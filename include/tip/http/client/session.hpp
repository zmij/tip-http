/*
 * session.hpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_CLIENT_SESSION_HPP_
#define TIP_HTTP_CLIENT_SESSION_HPP_

#include <memory>
#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>

#include <tip/http/common/request.hpp>

namespace tip {
namespace http {

class response;
using response_ptr      = std::shared_ptr< response >;

namespace client {

class session;
using session_ptr       = std::shared_ptr< session >;
using session_weak_ptr  = std::weak_ptr< session >;

class session : boost::noncopyable {
public:
    using io_service        = boost::asio::io_service;
    using body_type         = std::vector<char>;
    using response_callback = std::function< void(request_ptr, response_ptr) >;
    using error_callback    = std::function< void(std::exception_ptr) >;
    using session_callback  = std::function< void(session_ptr) >;
public:
    virtual ~session();

    void
    send_request(request_method method, request::iri_type const&,
            body_type const&, response_callback cb, error_callback ecb);
    void
    send_request(request_method method, request::iri_type const&,
            body_type&&, response_callback cb, error_callback ecb);
    void
    send_request(request_ptr req, response_callback cb, error_callback ecb);

    void
    close();

    static session_ptr
    create(io_service& svc, request::iri_type const&, session_callback on_close,
            headers const& default_headers);
protected:
    session();

    virtual void
    do_send_request(request_method method, request::iri_type const&,
            body_type const&, response_callback cb, error_callback) = 0;
    virtual void
    do_send_request(request_method method, request::iri_type const&,
            body_type&&, response_callback cb, error_callback) = 0;
    virtual void
    do_send_request(request_ptr req, response_callback cb, error_callback) = 0;

    virtual void
    do_close() = 0;
};

} /* namespace client */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_CLIENT_SESSION_HPP_ */
