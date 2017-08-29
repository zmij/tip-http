/*
 * connection_pool.hpp
 *
 *  Created on: Apr 14, 2016
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_CLIENT_CONNECTION_POOL_HPP_
#define PUSHKIN_HTTP_CLIENT_CONNECTION_POOL_HPP_

#include <deque>
#include <unordered_set>
#include <tuple>
#include <mutex>
#include <pushkin/http/client/session.hpp>

namespace psst {
namespace http {
namespace client {

class session_pool : public session,
                     public ::std::enable_shared_from_this< session_pool > {
public:
    using session_container = ::std::unordered_set< session_ptr >;
    using session_queue     = ::std::deque< session_ptr >;
    using queued_request    = ::std::tuple<
                                        request_ptr,
                                        response_callback,
                                        error_callback
                              >;
    using request_queue     = ::std::deque< queued_request >;
    using mutex_type        = ::std::recursive_mutex;
    using lock_type         = ::std::lock_guard<mutex_type>;
    using pool_ptr          = ::std::shared_ptr< session_pool >;
public:
    virtual ~session_pool() {}

    static pool_ptr
    create(io_service& svc, request::iri_type const& iri,
            session_callback on_close, headers const& default_headers,
            ::std::size_t max_sessions);
private:
    session_pool(io_service& svc, request::iri_type const& iri,
            session_callback on_close, headers const& default_headers,
            ::std::size_t max_sessions);
    void
    do_send_request(request_ptr req, response_callback cb, error_callback) override;

    void
    send_with_retries(request_ptr req, int retry_count, response_callback cb, error_callback ecb);

    void
    do_close() override;

    ::std::size_t
    get_request_count() const override;
private:
    session_ptr
    start_session();
    void
    session_idle(session_ptr);
    void
    session_closed(session_ptr, ::std::exception_ptr);
private:
    io_service&             svc_;
    request::iri_type       iri_;
    headers                 default_headers_;
    ::std::size_t           max_sessions_;

    mutex_type              mtx_;

    ::std::atomic<bool >    closed_;
    session_callback        on_close_;

    session_container       sessions_;
    session_queue           idle_sessions_;

    request_queue           requests_;
};

}  // namespace client
}  // namespace http
}  // namespace psst


#endif /* PUSHKIN_HTTP_CLIENT_CONNECTION_POOL_HPP_ */
