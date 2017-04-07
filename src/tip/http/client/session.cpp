/*
 * session.cpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#include <tip/http/client/session.hpp>
#include <tip/http/client/errors.hpp>

#include <tip/http/common/request.hpp>
#include <tip/http/common/response.hpp>

#include <tip/ssl_context_service.hpp>

#include <tip/util/misc_algorithm.hpp>
#include <pushkin/log.hpp>

#include <boost/asio.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/operator.hpp>

namespace tip {
namespace http {
namespace client {

LOCAL_LOGGING_FACILITY(HTTPSSN, TRACE);

struct tcp_transport {
    using io_service = boost::asio::io_service;
    using tcp = boost::asio::ip::tcp;
    using error_code = boost::system::error_code;
    using connect_callback = std::function<void(::std::exception_ptr)>;
    using socket_type = tcp::socket;

    tcp::resolver resolver_;
    socket_type socket_;

    tcp_transport(io_service& io_service, session::connection_id const& conn_id,
            connect_callback cb) :
            resolver_(io_service), socket_(io_service)
    {
        tcp::resolver::query
            qry(static_cast<std::string const&>(conn_id.first), conn_id.second);
        resolver_.async_resolve(qry, std::bind(
            &tcp_transport::handle_resolve, this,
                std::placeholders::_1, std::placeholders::_2, cb
        ));
    }

    void
    handle_resolve(error_code const& ec,
            tcp::resolver::iterator endpoint_iterator,
            connect_callback cb)
    {
        if (!ec) {
            boost::asio::async_connect(socket_, endpoint_iterator, std::bind(
                &tcp_transport::handle_connect, this,
                    std::placeholders::_1, cb
            ));
        } else if (cb) {
            local_log(logger::ERROR) << "Failed to resolve";
            cb(::std::make_exception_ptr( errors::resolve_failed(ec.message()) ));
        }
    }

    void
    handle_connect(error_code const& ec, connect_callback cb)
    {
        if (cb) {
            if (!ec) {
                cb(nullptr);
            } else {
                cb(::std::make_exception_ptr( errors::connection_refused(ec.message()) ));
            }
        }
    }
    void
    disconnect()
    {
        if (socket_.is_open()) {
            socket_.close();
        }
    }
};

struct ssl_transport {
    using io_service = boost::asio::io_service;
    using tcp = boost::asio::ip::tcp;
    using error_code = boost::system::error_code;
    using connect_callback = std::function<void(::std::exception_ptr)>;
    using socket_type = boost::asio::ssl::stream< tcp::socket >;

    tcp::resolver resolver_;
    socket_type socket_;

    ssl_transport(io_service& io_service, session::connection_id const& conn_id, connect_callback cb) :
        resolver_(io_service),
        socket_( io_service,
                boost::asio::use_service<tip::ssl::ssl_context_service>(io_service).context() )
    {
        tcp::resolver::query
            qry(static_cast<std::string const&>(conn_id.first), conn_id.second);
        resolver_.async_resolve(qry, std::bind(
            &ssl_transport::handle_resolve, this,
                std::placeholders::_1, std::placeholders::_2, cb
        ));
    }

    void
    handle_resolve(error_code const& ec,
            tcp::resolver::iterator endpoint_iterator,
            connect_callback cb)
    {
        if (!ec) {
            socket_.set_verify_mode(boost::asio::ssl::verify_peer);
            socket_.set_verify_callback(std::bind(
                &ssl_transport::verify_certificate,
                this, std::placeholders::_1, std::placeholders::_2
            ));
            boost::asio::async_connect(socket_.lowest_layer(),
                endpoint_iterator, std::bind(
                &ssl_transport::handle_connect, this,
                    std::placeholders::_1, cb
            ));
        } else if (cb) {
            cb(::std::make_exception_ptr( errors::resolve_failed(ec.message()) ));
        }
    }

    bool
    verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx)
    {
        // The verify callback can be used to check whether the certificate that is
        // being presented is valid for the peer. For example, RFC 2818 describes
        // the steps involved in doing this for HTTPS. Consult the OpenSSL
        // documentation for more details. Note that the callback is called once
        // for each certificate in the certificate chain, starting from the root
        // certificate authority.

        // In this example we will simply print the certificate's subject name.
        char subject_name[256];
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        local_log() << "Verifying " << subject_name;
        return preverified;
    }
    void
    handle_connect(error_code const& ec, connect_callback cb)
    {
        if (!ec) {
            socket_.async_handshake(boost::asio::ssl::stream_base::client,
                std::bind(&ssl_transport::handle_handshake, this,
                    std::placeholders::_1, cb));
        } else if (cb) {
            cb(::std::make_exception_ptr( errors::connection_refused(ec.message()) ));
        }
    }
    void
    handle_handshake(error_code const& ec, connect_callback cb)
    {
        if (cb) {
            if (!ec) {
                cb(nullptr);
            } else {
                cb(::std::make_exception_ptr( errors::ssl_handshake_failed(ec.message()) ));
            }
        }
    }
    void
    disconnect()
    {
        if (socket_.lowest_layer().is_open())
            socket_.lowest_layer().close();
    }
};

//-----------------------------------------------------------------------------
namespace events {
struct connected {};
struct disconnect {};
struct transport_error {
    std::exception_ptr error;
};
struct request {
    http::request_ptr request_;
    session::response_callback success_;
    session::error_callback    fail_;
};
struct response {
    http::response_ptr response_;
};
}  // namespace events

template < typename TransportType, typename SharedType >
struct session_fsm_ :
        public boost::msm::front::state_machine_def< session_fsm_<TransportType, SharedType >>,
        public std::enable_shared_from_this< SharedType > {
    using transport_type        = TransportType;
    using shared_type           = SharedType;
    using shared_base           = std::enable_shared_from_this< shared_type >;
    using session_fsm           = boost::msm::back::state_machine< session_fsm_< transport_type, shared_type > >;
    using io_service            = boost::asio::io_service;
    using buffer_type           = boost::asio::streambuf;
    using buffer_ptr            = std::shared_ptr< buffer_type >;
    using output_buffers_type   = std::vector< boost::asio::const_buffer >;
    using error_code            = boost::system::error_code;
    //@{
    /** @name Aliases for MSM types */
    template < typename ... T >
    using Row = boost::msm::front::Row< T ... >;
    template < typename ... T >
    using Internal = boost::msm::front::Internal< T ... >;
    using none                  = boost::msm::front::none;
    template < typename T >
    using Not = boost::msm::front::euml::Not_< T >;
    //@}

    //@{
    /** @name States */
    struct unplugged : public boost::msm::front::state<> {
        typedef boost::mpl::vector<
            events::request
        > deferred_events;
        void
        on_exit(events::transport_error const&, session_fsm& fsm)
        {
            local_log() << "exiting unplugged (error) deferred events: "
                    << fsm.get_deferred_queue().size();
        }
        template < typename Event >
        void
        on_exit(Event const&, session_fsm&)
        {
            local_log() << "exiting unplugged";
        }
    };
    struct online_ : public boost::msm::front::state_machine_def<online_> {
        using online = boost::msm::back::state_machine< online_ >;
        template < typename Event >
        void
        on_entry(Event const&, session_fsm& fsm)
        {
            local_log() << "entering online";
            session_ = &fsm;
        }
        template < typename Event, typename FSM >
        void
        on_exit(Event const&, FSM&)
        {
            local_log() << "exiting online";
        }
        //@{
        /** @name States */
        struct wait_request : public boost::msm::front::state<> {
            template < typename Event >
            void
            on_entry(Event const&, online& fsm)
            {
                local_log() << "entering wait_request";
                fsm.session_->idle();
            }
            template < typename Event, typename FSM >
            void
            on_exit(Event const&, FSM&)
            {
                local_log() << "exiting wait_request";
            }
        };
        struct wait_response : public boost::msm::front::state<> {
            template < typename Event, typename FSM >
            void
            on_entry(Event const&, FSM&)
            { local_log() << "entering wait_response"; }

            template < typename Event, typename FSM >
            void
            on_exit(Event const&, FSM&)
            { local_log() << "exiting wait_response"; }

            template < typename FSM >
            void
            on_exit(events::transport_error const& evt, FSM&)
            {
                local_log() << "exiting wait_response (on error)";
                if (req_.fail_) {
                    req_.fail_(evt.error);
                }
                req_ = events::request{};
            }
            typedef boost::mpl::vector<
                events::request
            > deferred_events;

            events::request req_;
        };
        using initial_state = wait_request;
        //@}
        //@{
        /** @name Actions */
        struct send_request {
            template < typename SourceState >
            void
            operator()(events::request const& req, online& fsm, SourceState&,
                    wait_response& resp_state)
            {
                fsm.session_->send_request(*req.request_.get());
                resp_state.req_ = req;
            }
        };
        struct process_reply {
            template < typename TargetState >
            void
            operator()(events::response const& resp, online& fsm, wait_response& resp_state,
                    TargetState&)
            {
                local_log() << "Reply to "
                        << resp_state.req_.request_->method
                        << " " << resp_state.req_.request_->path
                        << " " << resp.response_->status;
                if (resp_state.req_.success_) {
                    try {
                        resp_state.req_.success_(resp_state.req_.request_, resp.response_);
                    } catch (std::exception const& e) {
                        local_log(logger::ERROR)
                                << "Response handler throwed an exception: "
                                << e.what();
                    } catch (...) {
                        local_log(logger::ERROR)
                                << "Response handler throwed an unexpected exception";
                    }
                } else {
                    local_log(logger::WARNING) << "No response callback";
                }
                resp_state.req_ = events::request{};
                fsm.session_->request_handled();
            }
        };
        //@}
        struct transition_table : boost::mpl::vector <
            /*        Start            Event                Next            Action            Guard          */
            /*  +-----------------+-------------------+---------------+---------------+-------------+ */
            Row <    wait_request,    events::request,    wait_response,    send_request,    none        >,
            Row <    wait_response,   events::response,   wait_request,     process_reply,   none        >
        > {};

        online_() : session_(nullptr) {}
        session_fsm* session_;
    };
    using online = boost::msm::back::state_machine< online_ >;

    struct connection_failed : boost::msm::front::state<> {
        ::std::exception_ptr error;

        void
        on_entry(events::transport_error const& evt, session_fsm& fsm)
        {
            local_log() << "entering connection failed. deferred events "
                    << fsm.get_deferred_queue().size();
            error = evt.error;
        }
        template< typename Event >
        void
        on_exit(Event const&, session_fsm&)
        {
            local_log() << "exiting connection failed";
            error = nullptr;
        }

        struct notify_request_failure {
            void
            operator()(events::request const& req, session_fsm& fsm,
                    connection_failed& state, connection_failed&)
            {
                local_log() << "Notify that request failed";
                if (req.fail_) {
                    try {
                        req.fail_(state.error);
                    } catch (std::exception const& e) {
                        local_log(logger::ERROR)
                                << "Error handler throwed an exception: "
                                << e.what();
                    } catch (...) {
                        local_log(logger::ERROR)
                                << "Error handler throwed an unexpected exception";
                    }
                }
                if (fsm.get_deferred_queue().empty()) {
                    fsm.enqueue_event( events::disconnect{} );
                }
            }
        };
        struct internal_transition_table : ::boost::mpl::vector<
            Internal< events::request, notify_request_failure, none >
        > {};
    };

    struct terminated : boost::msm::front::terminate_state<> {
        void
        on_entry(events::transport_error const& evt, session_fsm& fsm)
        {
            local_log() << "entering terminated (error)";
            fsm.notify_closed(evt.error);
        }
        template < typename Event >
        void
        on_entry(Event const&, session_fsm& fsm)
        {
            local_log() << "entering terminated";
            fsm.notify_closed(nullptr);
        }
    };
    using initial_state = unplugged;
    //@}
    //@{
    /** @name Actions */
    struct disconnect_transport {
        template < typename Event, typename SourceState, typename TargetState >
        void
        operator()(Event const&, session_fsm& fsm, SourceState&, TargetState&)
        {
            fsm.disconnect();
        }
    };
    //@}
    //@{
    /** @name Guards */
    struct events_pending {
        template < class EVT, class SourceState, class TargetState>
        bool
        operator()(EVT const&, session_fsm& fsm, SourceState&, TargetState&)
        {
            return !fsm.get_deferred_queue().empty();
        }
    };
    //@}
    //@{
    struct transition_table : boost::mpl::vector<
        /*        Start            Event                    Next            Action                        Guard                  */
        /*  +-----------------+-----------------------+---------------+---------------------------+---------------------+ */
        Row <    unplugged,         events::connected,        online,            none,                        none                >,
        /* Transport errors */
        Row <    unplugged,         events::
                                    transport_error,    connection_failed,  none,                        events_pending      >,
        Row <    unplugged,         events::
                                    transport_error,    terminated,         disconnect_transport,        Not<events_pending> >,
        Row <    online,            events::
                                    transport_error,    connection_failed,  none,                        events_pending      >,
        Row <    online,            events::
                                    transport_error,    terminated,         disconnect_transport,        Not<events_pending> >,
        /* Disconnect */
        Row <    unplugged,         events::disconnect, terminated,         none,                        none                >,
        Row <    online,            events::disconnect, terminated,         disconnect_transport,        none                >,
        Row <    connection_failed, events::disconnect, terminated,         none,                        none                >
    >{};

    template < typename Event, typename FSM >
    void
    no_transition(Event const& e, FSM&, int state)
    {
        local_log(logger::DEBUG) << "No transition from state " << state
                << " on event " << typeid(e).name() << " (in transaction)";
    }
    template <class FSM,class Event>
    void exception_caught (Event const& evt, FSM& , std::exception& ex )
    {
        local_log(logger::DEBUG) << "Exception caught on event "
                << typeid(evt).name() << " " << typeid(ex).name()
                << ": " << ex.what();
    }
    //@}
    session_fsm_(io_service& io_service, request::iri_type const& iri,
            session::session_callback on_idle, session::session_error on_close,
            headers const& default_headers) :
        io_service_{io_service},
        strand_{io_service},
        transport_{io_service, session::create_connection_id(iri),
            strand_.wrap(
                std::bind( &session_fsm_::handle_connect,
                        this, std::placeholders::_1 ))},
        host_{iri.authority.host}, scheme_{iri.scheme},
        default_headers_{default_headers}, on_idle_(on_idle), on_close_{on_close},
        req_count_{0}
    {
    }

    virtual ~session_fsm_() {}

    void
    handle_connect(::std::exception_ptr ex)
    {
        if (!ex) {
            local_log(logger::DEBUG) << "Connected to "
                    << scheme_ << "://" << host_;
            fsm().process_event(events::connected{});
            start_read_headers();
        } else {
            try {
                ::std::rethrow_exception(ex);
            } catch (std::exception const& e) {
                local_log(logger::ERROR) << "Error connecting to "
                        << scheme_ << "://" << host_ << ": " << e.what();
                fsm().process_event(events::transport_error{
                    ::std::current_exception()
                });
            }
        }
    }

    void
    send_request(request const& req)
    {
        using std::placeholders::_1;
        using std::placeholders::_2;

        buffer_ptr outgoing = std::make_shared<buffer_type>();
        std::ostream os(outgoing.get());
        os << req;
        if (!default_headers_.empty()) {
            os << default_headers_;
        }
        os << "\r\n";
        output_buffers_type buffers;
        buffers.push_back(boost::asio::buffer(outgoing->data()));
        buffers.push_back(boost::asio::buffer(req.body_));
        boost::asio::async_write(transport_.socket_, buffers,
            strand_.wrap(::std::bind(&shared_type::handle_write,
                        shared_base::shared_from_this(), _1, _2, outgoing)));
    }

    void
    start_read_headers()
    {
        using std::placeholders::_1;
        using std::placeholders::_2;
        // Start read headers
        boost::asio::async_read_until(transport_.socket_, incoming_, "\r\n\r\n",
            strand_.wrap(::std::bind(&shared_type::handle_read_headers,
                    shared_base::shared_from_this(), _1, _2)));
    }

    void
    handle_write(error_code const& ec, size_t, buffer_ptr)
    {
        using std::placeholders::_1;
        using std::placeholders::_2;
        if (ec) {
            fsm().process_event(events::transport_error{
                ::std::make_exception_ptr(errors::connection_broken{ec.message()})
            });
        }
    }
    void
    handle_read_headers(error_code const& ec, size_t)
    {
        if (!ec) {
            // Parse response head
            std::istream is(&incoming_);
            response_ptr resp(std::make_shared< response >());
            if (resp->read_headers(is)) {
                read_body(resp, resp->read_body(is));
            } else {
                local_log(logger::ERROR) << "Failed to parse response headers";
            }
        } else {
            local_log(logger::DEBUG) << "Request to " << scheme_ << "://" << host_
                    << " connection dropped before headers read";
            fsm().process_event(events::transport_error{
                ::std::make_exception_ptr(errors::connection_broken{ec.message()})
            });
        }
    }

    void
    read_body(response_ptr resp, response::read_result_type res)
    {
        using std::placeholders::_1;
        using std::placeholders::_2;
        if (res.result) {
            // success read
            local_log() << "Request to " << scheme_ << "://" << host_
                    << " successfully read response body";
            fsm().process_event( events::response{ resp } );
        } else if (!res.result) {
            // failed to read body
            local_log() << "Request to " << scheme_ << "://" << host_
                    << " failed read response body";
            fsm().process_event( events::response{ resp } );
        } else if (res.callback) {
                // need more data
                local_log() << "Request to " << scheme_ << "://" << host_
                        << " need more data to read body";
                boost::asio::async_read(transport_.socket_,
                        incoming_,
                        boost::asio::transfer_at_least(1),
                        strand_.wrap(std::bind(&shared_type::handle_read_body,
                            shared_base::shared_from_this(), _1, _2,
                            resp, res.callback)));
        } else {
            // need more data but no callback
            local_log(logger::WARNING) << "Response read body returned "
                    "indeterminate, but provided no callback";
            fsm().process_event( events::response{ resp } );
        }
    }

    void
    handle_read_body(error_code const& ec, size_t, response_ptr resp,
            response::read_callback cb)
    {
        if (!ec) {
            if (cb) {
            std::istream is(&incoming_);
            read_body(resp, cb(is));
        } else {
                local_log(logger::WARNING) << "No callback for reading body";
            }
        } else {
            local_log() << "Request to " << scheme_ << "://" << host_
                    << " dropped connect before body read";
            fsm().process_event(events::transport_error{
                ::std::make_exception_ptr(errors::connection_broken{ec.message()})
            });
        }
    }

    void
    idle()
    {
        if (on_idle_) {
            on_idle_(shared_base::shared_from_this());
        }
    }

    void
    request_handled()
    {
        ++req_count_;
        start_read_headers();
    }

    void
    disconnect()
    {
        transport_.disconnect();
    }

    void
    notify_closed(::std::exception_ptr e)
    {
        if(on_close_) {
            on_close_(shared_base::shared_from_this(), e);
        }
    }
private:
    session_fsm&
    fsm()
    {
        return static_cast< session_fsm& >(*this);
    }
    session_fsm const&
    fsm() const
    {
        return static_cast< session_fsm const& >(*this);
    }
protected:
    io_service&                         io_service_;
private:
    boost::asio::io_service::strand     strand_;
    transport_type                      transport_;
    iri::host                           host_;
    iri::scheme                         scheme_;
    buffer_type                         incoming_;
    headers                             default_headers_;

    session::session_callback           on_idle_;
    session::session_error              on_close_;
protected:
    ::std::atomic<::std::size_t>        req_count_;
};
//-----------------------------------------------------------------------------
template < typename TransportType >
class session_impl : public session,
        public boost::msm::back::state_machine< session_fsm_< TransportType,
            session_impl< TransportType > > > {
public:
    typedef boost::msm::back::state_machine< session_fsm_< TransportType,
            session_impl< TransportType > > > base_type;
    using this_type = session_impl< TransportType >;

    session_impl(io_service& io_service, request::iri_type const& iri,
            session_callback on_idle, session_error on_close,
            headers const& default_headers) :
        base_type(std::ref(io_service), iri, on_idle, on_close, default_headers)
    {
    }

    virtual ~session_impl()
    {
        local_log() << "session_impl::~session_impl";
    }

    void
    do_send_request(request_ptr req, response_callback on_reply, error_callback on_error) override
    {
        local_log() << "enqueue request event";
        base_type::process_event( events::request{ req, on_reply, on_error });
    }
    void
    do_close() override
    {
        base_type::process_event( events::disconnect() );
    }
    ::std::size_t
    get_request_count() const override
    {
        return base_type::req_count_;
    }
};

//----------------------------------------------------------------------------
session::session()
{
}

session::~session()
{
}

void
session::send_request(request_method method, request::iri_type const& iri,
        body_type const& body, response_callback cb, error_callback ecb)
{
    do_send_request( request::create(method, iri, body), cb, ecb);
}

void
session::send_request(request_method method, request::iri_type const& iri,
        body_type&& body, response_callback cb, error_callback ecb)
{
    do_send_request( request::create(method, iri, std::move(body)), cb, ecb);
}

void
session::send_request(request_ptr req, response_callback cb, error_callback ecb)
{
    do_send_request(req, cb, ecb);
}

void
session::close()
{
    do_close();
}

::std::size_t
session::request_count() const
{
    return get_request_count();
}

session_ptr
session::create(io_service& svc, request::iri_type const& iri,
        session_callback on_idle, session_error on_close,
        headers const& default_headers)
{
    using http_session = session_impl< tcp_transport >;
    using https_session = session_impl< ssl_transport >;

    local_log() << "Create session " << iri.scheme << "://" << iri.authority.host
            << iri.path;
    if (iri.scheme == tip::iri::scheme{ "http" }) {
        return std::make_shared< http_session >( svc, iri, on_idle, on_close, default_headers );
    } else if (iri.scheme == tip::iri::scheme{ "https" }) {
        return std::make_shared< https_session >( svc, iri, on_idle, on_close, default_headers );
    }
    // TODO Throw an exception
    local_log(logger::ERROR) << "Unsupported scheme for HTTP client " << iri.scheme;
    throw ::std::runtime_error{ "Unsupported scheme" };
}

session::connection_id
session::create_connection_id(request::iri_type const& iri)
{
    return ::std::make_pair( iri.authority.host, (
            iri.authority.port.empty() ?
                    static_cast<std::string const&>(iri.scheme) :
                    static_cast<std::string const&>(iri.authority.port) ) );
}


} /* namespace client */
} /* namespace http */
} /* namespace tip */
