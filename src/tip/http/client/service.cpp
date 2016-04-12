/*
 * service.cpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#include <tip/http/client/service.hpp>
#include <tip/log.hpp>

#include <set>
#include <map>
#include <mutex>

#include <tip/http/client/session.hpp>
#include <tip/http/client/errors.hpp>
#include <tip/http/common/response.hpp>

namespace tip {
namespace http {
namespace client {

LOCAL_LOGGING_FACILITY(HTTPCLIENT, TRACE);

service::io_service::id service::id;

namespace {

const iri::scheme HTTP_SCHEME = iri::scheme{"http"};
const iri::scheme HTTPS_SCHEME = iri::scheme{"https"};

const std::set< iri::scheme > SUPPORTED_SCHEMES { HTTP_SCHEME, HTTPS_SCHEME };

}  // namespace

struct service::impl : std::enable_shared_from_this<impl> {
    using connection_id     = session::connection_id;
    // TODO multiple sessions per host
    using session_container = ::std::map< connection_id, session_ptr >;
    using mutex_type        = ::std::mutex;
    using lock_type         = ::std::lock_guard< mutex_type >;

    io_service&             owner_;
    headers                 default_headers_;
    session_container       sessions_;
    mutex_type              mtx_;

    impl(io_service& owner, headers const& default_headers) :
        owner_(owner), default_headers_(default_headers)
    {
    }

    ~impl()
    {
        local_log() << "service::impl::~impl";
    }

    void
    get(std::string const& url, response_callback cb, error_callback ecb)
    {
        request::iri_type iri;
        if (!request::parse_iri(url, iri)) {
            try {
                throw errors::http_client_error("Invalid IRI");
            } catch (std::exception const& e) {
                if (ecb) {
                    ecb(std::current_exception());
                } else throw;
            }
        }
        send_request(GET, iri, session::body_type(), cb, ecb);
    }
    void
    post(std::string const& url, body_type const& body,
            response_callback cb, error_callback ecb)
    {
        request::iri_type iri;
        if (!request::parse_iri(url, iri)) {
            return;
        }
        send_request(POST, iri, body, cb, ecb);
    }
    void
    post(std::string const& url, body_type&& body,
            response_callback cb, error_callback ecb)
    {
        request::iri_type iri;
        if (!request::parse_iri(url, iri)) {
            return;
        }
        send_request(POST, iri, std::move(body), cb, ecb);
    }


    void
    send_request(request_method method, request::iri_type const& iri,
            session::body_type const& body,
            response_callback cb, error_callback ecb)
    {
        using std::placeholders::_1;
        using std::placeholders::_2;
        if (!cb) {
            local_log(logger::WARNING) << method << " " << iri.scheme << "://"
                    << iri.authority.host << ": No result callback ";
            return;
        }
        if (!ecb) {
            local_log(logger::WARNING) << method << " " << iri.scheme << "://"
                    << iri.authority.host << ": No error callback ";
        }
        try {
            session_ptr s = get_session(iri);
            s->send_request(method, iri, body,
                    std::bind(&impl::handle_response,
                            shared_from_this(), _1, _2, cb, ecb), ecb);
        } catch (...) {
            if (ecb) {
                ecb(std::current_exception());
            } else throw;
        }
    }
    void
    send_request(request_method method, request::iri_type const& iri,
            session::body_type&& body,
            response_callback cb, error_callback ecb)
    {
        using std::placeholders::_1;
        using std::placeholders::_2;
        if (!cb) {
            local_log(logger::WARNING) << method << " " << iri.scheme << "://"
                    << iri.authority.host << ": No result callback ";
            return;
        }
        if (!ecb) {
            local_log(logger::WARNING) << method << " " << iri.scheme << "://"
                    << iri.authority.host << ": No error callback ";
        }
        try {
            session_ptr s = get_session(iri);
            s->send_request(method, iri, std::move(body),
                    std::bind(&impl::handle_response,
                            shared_from_this(), _1, _2, cb, ecb), ecb);
        } catch (...) {
            if (ecb) {
                ecb(std::current_exception());
            } else throw;
        }
    }

    session_ptr
    get_session(request::iri_type const& iri)
    {
        if (SUPPORTED_SCHEMES.count(iri.scheme) == 0) {
            // TODO Throw an error - unsupported scheme
            local_log(logger::ERROR) << "IRI scheme "
                    << iri.scheme << " is not supported";
            throw std::runtime_error("Scheme not supported");
        }
        if (iri.authority.host.empty()) {
            // TODO Throw an error - empty host
            local_log(logger::ERROR) << "Host is empty";
            throw std::runtime_error("Host is empty");
        }
        connection_id cid = session::create_connection_id(iri);

        lock_type lock(mtx_);
        auto f = sessions_.find(cid);
        if (f == sessions_.end()) {
            session_ptr s = create_session(iri);
            f = sessions_.insert(std::make_pair(cid, s)).first;
        }
        return f->second;
    }

    session_ptr
    create_session(request::iri_type const& iri)
    {
        return session::create(owner_, iri,
                ::std::bind(&impl::session_closed,
                        shared_from_this(), std::placeholders::_1),
                default_headers_);
    }

    void
    handle_response(request_ptr req, response_ptr resp, response_callback cb,
            error_callback ecb)
    {
        using std::placeholders::_1;
        using std::placeholders::_2;
        local_log() << req->method << " " << "://" << req->host() << req->path
                << " " << resp->status << " " << resp->status_line;
        response_class r_class = status_class(resp->status);
        if (r_class == response_class::redirection) {
            // Handle redirect
            headers::const_iterator f = resp->headers_.find(Location);
            if (f != resp->headers_.end()) {
                local_log(logger::DEBUG) << "Request redirected to " << f->second;
                request::iri_type iri;
                if (!request::parse_iri(f->second, iri)) {
                    local_log(logger::WARNING) << "Invalid redirect location";
                    cb(resp);
                } else {
                    req->path = iri.path;
                    req->query = iri.query;
                    req->fragment = iri.fragment;
                    req->host( iri.authority.host );

                    session_ptr s = get_session(iri);
                    s->send_request(req, std::bind(
                            &impl::handle_response, this, _1, _2, cb, ecb), ecb);
                }
            } else {
                local_log(logger::WARNING) << "Request redirected, but no Location header set";
                cb(resp);
            }
        } else if (is_error(r_class)) {

            local_log(logger::WARNING) << "Received error response "
                    << static_cast<int>(resp->status)
                    << " " << resp->status_line << "\n" << *req;

            cb(resp);
        } else {
            cb(resp);
        }
    }

    void
    session_closed(session_ptr s)
    {
        lock_type lock(mtx_);
        local_log() << "Session closed";
        session_container::const_iterator p = sessions_.begin();
        for (; p != sessions_.end(); ++p) {
            if (p->second == s) break;
        }
        if (p != sessions_.end()) {
            sessions_.erase(p);
        }
    }

    void
    shutdown()
    {
        session_container save = sessions_;
        for (session_container::const_iterator p = save.begin();
                p != save.end(); ++p) {
            p->second->close();
        }
    }
};

service::service(io_service& owner) :
        base_type(owner),
        pimpl_(new impl {
            owner,
            {
                { Connection, "keep-alive" },
                { UserAgent, "tip-http-client" }
            }
        })
{
}

service::~service()
{
}

void
service::shutdown_service()
{
    local_log(logger::DEBUG) << "Shutdown HTTP client service";
    pimpl_->shutdown();
}

void
service::get(std::string const& url, response_callback cb, error_callback eb)
{
    pimpl_->get(url, cb, eb);
}
void
service::post(std::string const& url, body_type const& body,
        response_callback cb, error_callback eb)
{
    pimpl_->post(url, body, cb, eb);
}
void
service::post(std::string const& url, body_type&& body,
        response_callback cb, error_callback eb)
{
    pimpl_->post(url, std::move(body), cb, eb);
}

} /* namespace client */
} /* namespace http */
} /* namespace tip */
