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

    connection_id
    id() const override;

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
    do_close() override;

    ::std::size_t
    get_request_count() const override;
private:
    struct impl;
    using pimpl = ::std::shared_ptr<impl>;
    pimpl pimpl_;
};

}  // namespace client
}  // namespace http
}  // namespace psst


#endif /* PUSHKIN_HTTP_CLIENT_CONNECTION_POOL_HPP_ */
