/*
 * service.h
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_CLIENT_SERVICE_HPP_
#define PUSHKIN_HTTP_CLIENT_SERVICE_HPP_

#include <memory>
#include <functional>
#include <future>

#include <boost/asio/io_service.hpp>

#include <pushkin/http/common/request.hpp>

namespace psst {
namespace http {

class response;
using response_ptr      = ::std::shared_ptr< response >;
using response_callback = ::std::function< void(response_ptr) >;
using error_callback    = ::std::function< void(::std::exception_ptr) >;

namespace client {

const ::std::size_t DEFAULT_MAX_SESSIONS = 4;

class service : public boost::asio::io_service::service {
public:
    using base_type     = boost::asio::io_service::service;
    using io_service    = boost::asio::io_service;
    using body_type     = ::std::vector<char>;
public:
    static io_service::id id;
public:
    service(io_service& owner);
    virtual ~service();

    void
    set_defaults(headers const& h);
    void
    set_max_concurrent_sessions(::std::size_t);

    template < template < typename > class _Promise = ::std::promise >
    response_ptr
    get(::std::string const& url, headers const& hdrs = headers{})
    {
        auto future = get_async<_Promise>(url, hdrs, true);
        return future.get();
    }

    template < template < typename > class _Promise = ::std::promise >
    auto
    get_async(::std::string const& url, headers const& hdrs, bool run_sync)
        -> decltype( ::std::declval< _Promise< response_ptr > >().get_future() )
    {
        auto promise = ::std::make_shared< _Promise< response_ptr > >();
        get_async(url,
        [promise](response_ptr resp)
        {
            promise->set_value(resp);
        },
        [promise](::std::exception_ptr ex)
        {
            promise->set_exception(::std::move(ex));
        }, hdrs, run_sync);

        return promise->get_future();
    }
    void
    get_async(::std::string const& url, response_callback,
            error_callback = nullptr, headers const& = headers{}, bool run_sync = false);

    void
    post_async(::std::string const& url, body_type const& body,
            response_callback, error_callback = nullptr,
            headers const& = headers{}, bool run_sync = false);

    void
    post_async(::std::string const& url, body_type&& body,
            response_callback, error_callback = nullptr,
            headers const& = headers{}, bool run_sync = false);


    void
    post_async(::std::string const& url, ::std::string const& body,
            response_callback, error_callback = nullptr,
            headers const& = headers{}, bool run_sync = false);


    template < typename Body, template < typename > class _Promise = ::std::promise >
    auto
    post_async(::std::string const& url, Body&& body, headers const& hdrs, bool run_sync)
        -> decltype( ::std::declval< _Promise< response_ptr > >().get_future() )
    {
        auto promise = ::std::make_shared< _Promise< response_ptr > >();
        post_async(url, ::std::forward<Body>(body),
        [promise](response_ptr resp)
        {
            promise->set_value(resp);
        },
        [promise](::std::exception_ptr ex)
        {
            promise->set_exception(::std::move(ex));
        }, hdrs, run_sync);

        return promise->get_future();
    }
    template < typename Body, template < typename > class _Promise = ::std::promise >
    response_ptr
    post(::std::string const& url, Body&& body, headers const& hdrs = headers{} )
    {
        auto future = post_async<Body, _Promise>(url, ::std::forward<Body>(body), hdrs, true);
        return future.get();
    }
private:
    virtual void
    shutdown_service();
private:
    struct impl;
    using pimpl = ::std::shared_ptr< impl >;
    pimpl pimpl_;
};

} /* namespace client */
} /* namespace http */
} /* namespace psst */

#endif /* PUSHKIN_HTTP_CLIENT_SERVICE_HPP_ */
