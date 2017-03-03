/*
 * transport.hpp
 *
 *  Created on: Mar 3, 2017
 *      Author: zmij
 */

#ifndef TIP_HTTP_CLIENT_TRANSPORT_HPP_
#define TIP_HTTP_CLIENT_TRANSPORT_HPP_

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <memory.h>

namespace tip {
namespace http {
namespace client {

struct tcp_transport {
    using io_service = boost::asio::io_service;
    using tcp = boost::asio::ip::tcp;
    using error_code = boost::system::error_code;
    using connect_callback = std::function<void(::std::exception_ptr)>;
    using socket_type = tcp::socket;

    tcp::resolver resolver_;
    socket_type socket_;

    tcp_transport(io_service& io_service,
            ::std::string const& host,
            ::std::string const& service,
            connect_callback cb);

    void
    handle_resolve(error_code const& ec,
            tcp::resolver::iterator endpoint_iterator,
            connect_callback cb);
    void
    handle_connect(error_code const& ec, connect_callback cb);
    void
    disconnect();
};

struct ssl_transport {
    using io_service = boost::asio::io_service;
    using tcp = boost::asio::ip::tcp;
    using error_code = boost::system::error_code;
    using connect_callback = std::function<void(::std::exception_ptr)>;
    using socket_type = boost::asio::ssl::stream< tcp::socket >;

    tcp::resolver resolver_;
    socket_type socket_;

    ssl_transport(io_service& io_service,
            ::std::string const& host,
            ::std::string const& service,
            connect_callback cb);

    void
    handle_resolve(error_code const& ec,
            tcp::resolver::iterator endpoint_iterator,
            connect_callback cb);

    bool
    verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx);

    void
    handle_connect(error_code const& ec, connect_callback cb);

    void
    handle_handshake(error_code const& ec, connect_callback cb);

    void
    disconnect();
};



} /* namespace client */
} /* namespace http */
} /* namespace tip */



#endif /* TIP_HTTP_CLIENT_TRANSPORT_HPP_ */
