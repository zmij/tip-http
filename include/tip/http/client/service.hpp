/*
 * service.h
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_CLIENT_SERVICE_HPP_
#define TIP_HTTP_CLIENT_SERVICE_HPP_

#include <memory>
#include <functional>
#include <boost/asio/io_service.hpp>
#include <tip/http/common/request.hpp>

namespace tip {
namespace http {

class response;
using response_ptr      = std::shared_ptr< response >;
using response_callback = std::function< void(response_ptr) >;
using error_callback    = std::function< void(std::exception_ptr) >;

namespace client {

const ::std::size_t DEFAULT_MAX_SESSIONS = 4;

class service : public boost::asio::io_service::service {
public:
    using base_type     = boost::asio::io_service::service;
    using io_service    = boost::asio::io_service;
    using body_type     = std::vector<char>;
public:
    static io_service::id id;
public:
    service(io_service& owner);
    virtual ~service();

    void
    set_defaults(headers const& h);
    void
    set_max_concurrent_sessions(::std::size_t);

    void
    get(std::string const& url, response_callback, error_callback = nullptr);
    void
    post(std::string const& url, body_type const& body, response_callback,
            error_callback = nullptr);
    void
    post(std::string const& url, body_type&& body, response_callback,
            error_callback = nullptr);
private:
    virtual void
    shutdown_service();
private:
    struct impl;
    using pimpl = std::shared_ptr< impl >;
    pimpl pimpl_;
};

} /* namespace client */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_CLIENT_SERVICE_HPP_ */
