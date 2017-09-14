/*
 * session.cpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#include <pushkin/http/client/session.hpp>

#include <tip/ssl_context_service.hpp>

#include <pushkin/log.hpp>

#include <boost/asio.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>

#include <afsm/fsm.hpp>
#include <pushkin/http/client/errors.hpp>
#include <pushkin/http/common/request.hpp>
#include <pushkin/http/common/response.hpp>
#include <pushkin/util/misc_algorithm.hpp>

#include <pushkin/http/client/connection_fsm_observer.hpp>

namespace psst {
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

    tcp_transport(io_service& io_service) :
            resolver_(io_service), socket_(io_service)
    {
    }

    void
    connect(session::connection_id const& conn_id, connect_callback cb)
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

    ssl_transport(io_service& io_service) :
        resolver_(io_service),
        socket_( io_service,
                boost::asio::use_service<tip::ssl::ssl_context_service>(io_service).context() )
    {
    }

    void
    connect(session::connection_id const& conn_id, connect_callback cb)
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
struct session_fsm_def :
        public ::afsm::def::state_machine< session_fsm_def<TransportType, SharedType >>,
        public std::enable_shared_from_this< SharedType > {
    using transport_type        = TransportType;
    using shared_type           = SharedType;
    using shared_base           = std::enable_shared_from_this< shared_type >;
    using io_service            = boost::asio::io_service;
    using buffer_type           = boost::asio::streambuf;
    using buffer_ptr            = std::shared_ptr< buffer_type >;
    using output_buffers_type   = std::vector< boost::asio::const_buffer >;
    using error_code            = boost::system::error_code;
    //@{
    /** @name Aliases for afsm types */
    using none = ::afsm::none;

    template < typename StateDef, typename ... Tags >
    using state = ::afsm::def::state<StateDef, Tags...>;
    template < typename MachineDef, typename ... Tags >
    using state_machine = ::afsm::def::state_machine<MachineDef, Tags...>;
    template < typename ... T >
    using transition_table = ::afsm::def::transition_table<T...>;
    template < typename Event, typename Action = none, typename Guard = none >
    using in = ::afsm::def::internal_transition< Event, Action, Guard>;
    template <typename SourceState, typename Event, typename TargetState,
            typename Action = none, typename Guard = none>
    using tr = ::afsm::def::transition<SourceState, Event, TargetState, Action, Guard>;

    template < typename Predicate >
    using not_ = ::psst::meta::not_<Predicate>;
    //@}

    //@{
    /** @name States */
    struct unplugged : state< unplugged > {
        using deferred_events = ::psst::meta::type_tuple<
                events::request
            >;
    };
    struct online : state_machine<online> {
        //@{
        /** @name States */
        struct wait_request : state< wait_request > {
            template < typename Event, typename FSM >
            void
            on_enter(Event const&, FSM& fsm)
            {
                root_machine(fsm)->idle();
            }
        };
        struct wait_response : state<wait_response> {
            template < typename FSM >
            void
            on_exit(events::transport_error const& evt, FSM&)
            {
                if (req_.fail_) {
                    req_.fail_(evt.error);
                }
                req_ = events::request{};
            }
            using deferred_events = ::psst::meta::type_tuple<
                events::request
            >;

            events::request req_;
        };
        using initial_state = wait_request;
        //@}
        //@{
        /** @name Actions */
        struct send_request {
            template < typename FSM, typename SourceState >
            void
            operator()(events::request const& req, FSM& fsm, SourceState&,
                    wait_response& resp_state)
            {
                root_machine(fsm)->write_request(*req.request_.get());
                resp_state.req_ = req;
            }
        };
        struct process_reply {
            template < typename FSM, typename TargetState >
            void
            operator()(events::response const& resp, FSM& fsm, wait_response& resp_state,
                    TargetState&)
            {
                local_log() << "Reply to "
                        << resp_state.req_.request_->method
                        << " " << resp_state.req_.request_->path
                        << " " << resp.response_->status;
                root_machine(fsm)->request_handled();
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
            }
        };
        //@}
        using transitions = transition_table <
            /*        Start            Event                Next            Action            Guard          */
            /*  +-----------------+-------------------+---------------+---------------+-------------+ */
            tr <    wait_request,    events::request,    wait_response,    send_request,    none        >,
            tr <    wait_response,   events::response,   wait_request,     process_reply,   none        >
        >;

        online() {}
    };

    struct connection_failed : state< connection_failed > {
        ::std::exception_ptr error;

        template < typename FSM >
        void
        on_enter(events::transport_error const& evt, FSM& fsm)
        {
            error = evt.error;
            fsm.notify_closed(evt.error);
        }
        template< typename Event, typename FSM >
        void
        on_exit(Event const&, FSM& fsm)
        {
            error = nullptr;
        }

        struct notify_request_failure {
            template< typename FSM >
            void
            operator()(events::request const& req, FSM& fsm,
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
                fsm.process_event( events::disconnect{} );
            }
        };
        using internal_transitions = transition_table<
            in< events::request, notify_request_failure, none >
        >;
    };

    struct terminated : ::afsm::def::terminal_state<terminated> {
        template< typename FSM >
        void
        on_enter(events::transport_error const& evt, FSM& fsm)
        {
            fsm.notify_closed(evt.error);
        }
        template < typename Event, typename FSM >
        void
        on_enter(Event const&, FSM& fsm)
        {
            fsm.notify_closed(nullptr);
        }
    };
    using initial_state = unplugged;
    //@}
    //@{
    /** @name Actions */
    struct disconnect_transport {
        template < typename Event, typename FSM, typename SourceState, typename TargetState >
        void
        operator()(Event const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.disconnect();
        }
    };
    //@}
    //@{
    using transitions = transition_table<
        /*  Start                 Event                       Next                Action                  Guard   */
        /*+---------------------+---------------------------+-------------------+-----------------------+-------+ */
        tr< unplugged           , events::connected         , online            , none                  , none  >,
        /* Transport errors */
        tr< unplugged           , events::transport_error   , connection_failed , none                  , none  >,
        tr< online              , events::transport_error   , connection_failed , none                  , none  >,
        /* Disconnect */
        tr< unplugged           , events::disconnect        , terminated        , none                  , none  >,
        tr< online              , events::disconnect        , terminated        , disconnect_transport  , none  >,
        tr< connection_failed   , events::disconnect        , terminated        , none                  , none  >
    >;
    //@}
    session_fsm_def(io_service& io_service, request::iri_type const& iri,
            session::session_callback on_idle, session::session_error on_close,
            headers const& default_headers) :
        io_service_{io_service},
        strand_{io_service},
        transport_{io_service},
        host_{iri.authority.host}, scheme_{iri.scheme},
        default_headers_{default_headers}, on_idle_(on_idle), on_close_{on_close},
        req_count_{0}
    {
    }

    virtual ~session_fsm_def() {}

    void
    connect(request::iri_type const& iri)
    {
        using std::placeholders::_1;
        transport_.connect(session::create_connection_id(iri),
                ::std::bind(&session_fsm_def::handle_connect,
                        shared_base::shared_from_this(), _1));
    }

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
                    ::std::make_exception_ptr(e)
                });
            }
        }
    }

    // TODO Delegate this to protocol
    void
    write_request(request const& req)
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

    // TODO Delegate this to protocol
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
    // TODO Delegate this to protocol
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
            fsm().process_event(events::transport_error{
                ::std::make_exception_ptr(errors::connection_broken{"Failed to read body"})
            });
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
        } else {
            local_log() << "No on close handler for session";
        }
    }

    shared_type*
    operator ->()
    {
        return static_cast<shared_type*>(this);
    }

    shared_type const*
    operator ->() const
    {
        return static_cast<shared_type const*>(this);
    }

    ::std::ostream&
    tag(::std::ostream& os) const
    {
        os << host_;
        return os;
    }
private:
    shared_type&
    fsm()
    {
        return static_cast< shared_type& >(*this);
    }
    shared_type const&
    fsm() const
    {
        return static_cast< shared_type const& >(*this);
    }
protected:
    io_service&                         io_service_;
private:
    boost::asio::io_service::strand     strand_;
    transport_type                      transport_;
    ::tip::iri::host                    host_;
    ::tip::iri::scheme                  scheme_;
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
    public ::afsm::state_machine< session_fsm_def< TransportType,
            session_impl< TransportType > >, ::afsm::none, observer::conection_fsm_observer > {
public:
    using base_type = ::afsm::state_machine< session_fsm_def< TransportType,
            session_impl< TransportType > >, ::afsm::none, observer::conection_fsm_observer >;
    using this_type = session_impl< TransportType >;

    session_impl(io_service& io_service, request::iri_type const& iri,
            session_callback on_idle, session_error on_close,
            headers const& default_headers) :
        base_type(std::ref(io_service), iri, on_idle, on_close, default_headers)
    {
        this->make_observer();
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
        local_log() << "close session";
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
        auto s = std::make_shared< http_session >( svc, iri, on_idle, on_close, default_headers );
        s->connect(iri);
        return s;
    } else if (iri.scheme == tip::iri::scheme{ "https" }) {
        auto s = std::make_shared< https_session >( svc, iri, on_idle, on_close, default_headers );
        s->connect(iri);
        return s;
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
} /* namespace psst */
