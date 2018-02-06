/*
 * connection_pool.cpp
 *
 *  Created on: Apr 14, 2016
 *      Author: zmij
 */

#include <pushkin/http/client/session_pool.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_set.h>

#include <pushkin/log.hpp>

namespace std {

template < typename T >
::std::size_t
tbb_hasher(std::shared_ptr<T> v )
{
    return reinterpret_cast<::std::size_t>(v.get());
}

} // namespace std

namespace psst {
namespace http {
namespace client {

LOCAL_LOGGING_FACILITY(HTTPPOOL, DEBUG);

enum request_fields {
    the_request,
    on_success,
    on_error
};

struct session_pool::impl : ::std::enable_shared_from_this<impl> {
    using session_weak_ptr  = ::std::weak_ptr< session >;
    using request_queue     = ::tbb::concurrent_queue< queued_request >;
    using session_queue     = ::tbb::concurrent_queue< session_weak_ptr >;
    using session_container = ::tbb::concurrent_unordered_set< session_ptr >;

    io_service&             svc_;
    request::iri_type const iri_;
    headers                 default_headers_;
    ::std::size_t           max_sessions_;

    session_container       sessions_;
    session_queue           idle_sessions_;

    request_queue           requests_;

    impl(io_service& svc, request::iri_type const& iri,
            session_callback on_close, headers const& default_headers,
                    ::std::size_t max_sessions)
        : svc_{svc}, iri_{iri},
          default_headers_{default_headers},
          max_sessions_{max_sessions != 0 ? max_sessions : 1}
    {
    }

    void
    send_with_retries(request_ptr req, int retry_count,
            response_callback resp, error_callback err)
    {
        if (retry_count > 0) {
            auto _this = shared_from_this();

            auto retry = [ _this, req, resp, err, retry_count ](::std::exception_ptr ex)
            {
                local_log() << "Retry handler fired, retry count " << retry_count;
                _this->send_with_retries(req, retry_count - 1, resp, err);
            };
            err = retry;
        }

        auto s = get_session();
        if (s) {
            s->send_request(req, resp, err);
        } else {
            local_log() << "No idle connections, enqueue request";
            requests_.emplace(::std::make_tuple( req, resp, err ));
        }
    }

    session_ptr
    get_session()
    {
        session_weak_ptr res;
        while (idle_sessions_.try_pop(res)) {
            auto s = res.lock();
            if (s)
                return s;
        }
        if (sessions_.size() < max_sessions_) {
            auto s = start_session();
            sessions_.insert(s);
            return s;
        }
        local_log() << "Max sessions exceeded";
        return session_ptr{};
    }

    session_ptr
    start_session()
    {
        local_log() << "Session pool: start session";
        auto shared_this = shared_from_this();
        return session::create(svc_, iri_,
            ::std::bind(&impl::session_idle, shared_this,
                    ::std::placeholders::_1),
            ::std::bind(&impl::session_closed, shared_this,
                 ::std::placeholders::_1, ::std::placeholders::_2),
            default_headers_
        );
    }
    void
    session_idle(session_ptr s)
    {
        queued_request req;
        if (requests_.try_pop(req)) {
            s->send_request(
                ::std::get<the_request>(req),
                ::std::get<on_success>(req),
                ::std::get<on_error>(req)
            );
        } else {
            idle_sessions_.push(s);
        }
    }
    void
    session_closed(session_ptr s, ::std::exception_ptr e)
    {
        sessions_.unsafe_erase(s);
        if (e) {
            queued_request req;
            while (requests_.try_pop(req)) {
                auto err = ::std::get<on_error>(req);
                err(e);
            }
        }
    }

    void
    close()
    {
        sessions_.clear();
    }
};

session_pool::session_pool(io_service& svc, request::iri_type const& iri,
        session_callback on_close, headers const& default_headers,
        ::std::size_t max_sessions)
    : pimpl_{ ::std::make_shared<impl>(
            svc, iri, on_close, default_headers, max_sessions ) }
{
}

session_pool::connection_id
session_pool::id() const
{
    return create_connection_id(pimpl_->iri_);
}

void
session_pool::do_send_request(request_ptr req, response_callback cb, error_callback ecb)
{
    local_log() << "Send request";
    pimpl_->send_with_retries(req, 5, cb, ecb);
}


void
session_pool::do_close()
{
    pimpl_->close();
}

::std::size_t
session_pool::get_request_count() const
{
    return 0U;
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
