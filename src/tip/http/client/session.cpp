/*
 * session.cpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#include <tip/http/client/session.hpp>
#include <tip/http/client/errors.hpp>
#include <tip/http/client/transport.hpp>

#include <tip/http/common/request.hpp>
#include <tip/http/common/response.hpp>

#include <tip/ssl_context_service.hpp>

#include <tip/util/misc_algorithm.hpp>
#include <tip/log.hpp>

#include <boost/asio.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>

#include <afsm/fsm.hpp>

#include <mutex>

namespace tip {
namespace http {
namespace client {

LOCAL_LOGGING_FACILITY(HTTPSSN, TRACE);

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
struct session_fsm_ : afsm::def::state_machine< session_fsm_<TransportType, SharedType >>,
        public std::enable_shared_from_this< SharedType > {
    using transport_type        = TransportType;
    using shared_type           = SharedType;
    using shared_base           = std::enable_shared_from_this< shared_type >;
    using session_fsm           = ::afsm::state_machine< session_fsm_< transport_type, shared_type >, ::std::mutex >;
    using io_service            = boost::asio::io_service;
    using buffer_type           = boost::asio::streambuf;
    using buffer_ptr            = std::shared_ptr< buffer_type >;
    using output_buffers_type   = std::vector< boost::asio::const_buffer >;
    using error_code            = boost::system::error_code;
    //@{
    /** @name Aliases for AFSM types */
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
        using deferred_events = ::psst::meta::type_tuple <
            events::request
        >;
    };
    struct online : public state_machine< online > {
        template < typename Event >
        void
        on_enter(Event const&, session_fsm& fsm)
        {
            session_ = &fsm;
        }
        //@{
        /** @name States */
        struct wait_request : state< wait_request > {
            template < typename Event, typename FSM >
            void
            on_enter(Event const&, FSM& fsm)
            {
                fsm.session_->idle();
            }
        };
        struct wait_response : state< wait_response > {
            template < typename FSM >
            void
            on_exit(events::transport_error const& evt, FSM&)
            {
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
            template < typename FSM, typename SourceState >
            void
            operator()(events::request const& req, FSM& fsm, SourceState&,
                    wait_response& resp_state)
            {
                fsm.session_->send_request(*req.request_.get());
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
        using transitions = transition_table <
            /*  Start             Event               Next            Action          Guard   */
            /*+-----------------+-------------------+---------------+---------------+-------+ */
            tr< wait_request    , events::request   , wait_response , send_request  , none  >,
            tr< wait_response   , events::response  , wait_request  , process_reply , none  >
        >;

        online() : session_(nullptr) {}
        session_fsm* session_;
    };

    struct connection_failed : state< connection_failed > {
        ::std::exception_ptr error;

        void
        on_enter(events::transport_error const& evt, session_fsm& fsm)
        {
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
            }
        };
        using internal_transitions = transition_table<
            in< events::request, notify_request_failure, none >
        >;
    };

    struct terminated : ::afsm::def::terminal_state<terminated> {
        void
        on_enter(events::transport_error const& evt, session_fsm& fsm)
        {
            local_log() << "entering terminated (error)";
            fsm.notify_closed(evt.error);
        }
        template < typename Event >
        void
        on_enter(Event const&, session_fsm& fsm)
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
    using transitions = transition_table<
        /*  Start                 Event                       Next                Action                  Guard       */
        /*+---------------------+---------------------------+-------------------+-----------------------+-----------+ */
        tr< unplugged           , events::connected         , online            , none                  , none      >,
        /* Transport errors */
        tr< unplugged           , events::transport_error   , terminated        , disconnect_transport  , none      >,
        tr< online              , events::transport_error   , connection_failed , disconnect_transport  , none      >,
        /* Disconnect */
        tr< unplugged           , events::disconnect        , terminated        , none                  , none      >,
        tr< online              , events::disconnect        , terminated        , disconnect_transport  , none      >,
        tr< connection_failed   , events::disconnect        , terminated        , none                  , none      >
    >;

    //@}
    session_fsm_(io_service& io_service,
            iri::scheme const& scheme,
            ::std::string const& host,
            ::std::string const& service,
            session::session_callback on_idle,
            session::session_error on_close,
            headers const& default_headers) :
        io_service_{io_service},
        strand_{io_service},
        transport_{io_service, host, service,
            strand_.wrap(
                std::bind( &session_fsm_::handle_connect,
                        this, std::placeholders::_1 ))},
        host_{host},
        scheme_{scheme},
        default_headers_{default_headers},
        on_idle_(on_idle),
        on_close_{on_close},
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
    public ::afsm::state_machine< session_fsm_< TransportType,
            session_impl< TransportType > >, ::std::mutex > {
public:
    using base_type = ::afsm::state_machine< session_fsm_< TransportType,
            session_impl< TransportType > >, ::std::mutex >;
    using this_type = session_impl< TransportType >;

    session_impl(io_service& io_service,
            iri::scheme const& scheme,
            ::std::string const& host,
            ::std::string const& service,
            session_callback on_idle, session_error on_close,
            headers const& default_headers) :
        base_type(std::ref(io_service), scheme, host, service,
                on_idle, on_close, default_headers)
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
    auto id = create_connection_id(iri);
    if (iri.scheme == tip::iri::scheme{ "http" }) {
        return std::make_shared< http_session >( svc,
            iri.scheme, id.first, id.second, on_idle, on_close, default_headers );
    } else if (iri.scheme == tip::iri::scheme{ "https" }) {
        return std::make_shared< https_session >( svc,
                iri.scheme, id.first, id.second, on_idle, on_close, default_headers );
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
