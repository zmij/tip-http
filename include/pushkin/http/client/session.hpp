/*
 * session.hpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_CLIENT_SESSION_HPP_
#define PUSHKIN_HTTP_CLIENT_SESSION_HPP_

#include <memory>
#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>

#include <pushkin/http/common/request.hpp>

namespace psst {
namespace http {

class response;
using response_ptr      = ::std::shared_ptr< response >;

namespace client {

class session;
using session_ptr       = ::std::shared_ptr< session >;
using session_weak_ptr  = ::std::weak_ptr< session >;

class session {
public:
    using io_service        = boost::asio::io_service;
    using body_type         = ::std::vector<char>;
    using response_callback = ::std::function< void(request_ptr, response_ptr) >;
    using error_callback    = ::std::function< void(::std::exception_ptr) >;
    using session_callback  = ::std::function< void(session_ptr) >;
    using session_error     = ::std::function< void(session_ptr, ::std::exception_ptr) >;
    using connection_id     = ::std::pair< ::tip::iri::host, ::std::string >;
public:
    virtual ~session();

    session(session const&) = delete;
    session&
    operator = (session const&) = delete;

    void
    send_request(request_method method, request::iri_type const&, body_type const&,
            response_callback cb, error_callback ecb, headers const& = headers{});
    void
    send_request(request_method method, request::iri_type const&, body_type&&,
            response_callback cb, error_callback ecb, headers const& = headers{});
    void
    send_request(request_ptr req, response_callback cb, error_callback ecb);

    /**
     * Number of requests handled
     * @return
     */
    ::std::size_t
    request_count() const;

    void
    close();

    static session_ptr
    create(io_service& svc, request::iri_type const&,
            session_callback on_idle, session_error on_close,
            headers const& default_headers);

    static connection_id
    create_connection_id(request::iri_type const&);
protected:
    session();

    virtual void
    do_send_request(request_ptr req, response_callback cb, error_callback) = 0;

    virtual void
    do_close() = 0;

    virtual ::std::size_t
    get_request_count() const = 0;
};

} /* namespace client */
} /* namespace http */
} /* namespace psst */

#endif /* PUSHKIN_HTTP_CLIENT_SESSION_HPP_ */
