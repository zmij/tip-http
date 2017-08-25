/*
 * connection_pool.cpp
 *
 *  Created on: Apr 14, 2016
 *      Author: zmij
 */

#include <pushkin/http/client/session_pool.hpp>

#include <pushkin/log.hpp>

namespace psst {
namespace http {
namespace client {

LOCAL_LOGGING_FACILITY(HTTPPOOL, DEBUG);

enum request_fields {
    the_request,
    on_success,
    on_error
};

session_pool::session_pool(io_service& svc, request::iri_type const& iri,
        session_callback on_close, headers const& default_headers,
        ::std::size_t max_sessions)
    : svc_(svc), iri_(iri), default_headers_(default_headers),
      // max_sessions_(max_sessions != 0 ? max_sessions : 1), AWM-11130
      closed_(false), on_close_(on_close)
{
}

void
session_pool::do_send_request(request_ptr req, response_callback cb, error_callback ecb)
{
    local_log() << "Send request";
    // FIXME Magic number in retry_count
    send_with_retries(req, 5, cb, ecb);
}

void
session_pool::send_with_retries(request_ptr req, int retry_count, response_callback cb, error_callback ecb)
{
//    if (retry_count > 0) {
//        auto _this = shared_from_this();
//
//        auto retry = [_this, req, cb, ecb, retry_count](::std::exception_ptr ex) {
//            local_log() << "Retry handler fired, retry count " << retry_count;
//            _this->send_with_retries(req, retry_count - 1, cb, ecb);
//        };
//        ecb = retry;
//    }

    lock_type lock{mtx_};
//    if (!idle_sessions_.empty()) {
//        session_ptr s = idle_sessions_.front();
//        idle_sessions_.pop_front();
//        s->send_request(req, cb, ecb);
//    } else {
//        if (sessions_.size() < max_sessions_) {
            sessions_.insert(start_session());
//        }
        local_log() << "No idle connections, enqueue request";
        requests_.push_back( ::std::make_tuple(req, cb, ecb) );
//    }
}


void
session_pool::do_close()
{
    lock_type lock{mtx_};
    auto sessions = sessions_;
    for (auto s : sessions) {
        s->close();
    }
}

::std::size_t
session_pool::get_request_count() const
{
    return 0U;
}

session_ptr
session_pool::start_session()
{
    local_log() << "Session pool: start session";
    auto shared_this = shared_from_this();
    return session::create(svc_, iri_,
        ::std::bind(&session_pool::session_idle, shared_this,
                ::std::placeholders::_1),
        ::std::bind(&session_pool::session_closed, shared_this,
             ::std::placeholders::_1, ::std::placeholders::_2),
        default_headers_
    );
}

void
session_pool::session_idle(session_ptr s)
{
    local_log() << "Session idle";
    lock_type lock{mtx_};
    if (requests_.empty()) {
        local_log() << "No pending requests";
        if (s->request_count()) {
            session_closed(s, nullptr);
        } else {
            idle_sessions_.push_back(s);
        }
    } else {
        local_log() << "Pass request to session";
        auto req = requests_.front();
        requests_.pop_front();
        s->send_request(
            ::std::get<the_request>(req),
            ::std::get<on_success>(req),
            ::std::get<on_error>(req));
    }
}

void
session_pool::session_closed(session_ptr s, ::std::exception_ptr e)
{
    local_log() << "Session closed";
    lock_type lock{mtx_};
    sessions_.erase(s);
    for (auto p = idle_sessions_.begin(); p != idle_sessions_.end(); ++p) {
        if (*p == s) {
            idle_sessions_.erase(p);
            break;
        }
    }
    if (e != nullptr) {
        local_log() << "Connection closed on an error";
        if (!requests_.empty()) {
            request_queue tmp;
            tmp.swap(requests_);
            for (auto& r : tmp) {
                auto ecb = ::std::get<on_error>(r);
                if (ecb) {
                    ecb(e);
                }
            }
        }
    }
    if (sessions_.empty()) {
        local_log() << "All sessions closed";
        if (on_close_)
            on_close_(shared_from_this());
        else
            local_log(logger::WARNING) << "No on close callback";
    } else {
        local_log() << "Sessions size " << sessions_.size();
    }
}

session_pool::pool_ptr
session_pool::create(io_service& svc, request::iri_type const& iri,
        session_callback on_close, headers const& default_headers,
        ::std::size_t max_sessions)
{
    pool_ptr p {new session_pool{svc, iri, on_close,
        default_headers, max_sessions}};
    return p;
}

}  // namespace client
}  // namespace http
}  // namespace psst
