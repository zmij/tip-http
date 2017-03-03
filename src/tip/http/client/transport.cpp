/*
 * transport.cpp
 *
 *  Created on: Mar 3, 2017
 *      Author: zmij
 */

#include <tip/http/client/transport.hpp>
#include <tip/http/client/errors.hpp>

#include <tip/log.hpp>

#include <tip/ssl_context_service.hpp>

#include <boost/asio.hpp>

namespace tip {
namespace http {
namespace client {

namespace {
LOCAL_LOGGING_FACILITY(HTTPSSN, TRACE);
} /* namespace  */


tcp_transport::tcp_transport(io_service& io_service,
        ::std::string const& host,
        ::std::string const& service,
        connect_callback cb) :
        resolver_(io_service), socket_(io_service)
{
    tcp::resolver::query qry(host, service);
    resolver_.async_resolve(qry, std::bind(
        &tcp_transport::handle_resolve, this,
            std::placeholders::_1, std::placeholders::_2, cb
    ));
}

void
tcp_transport::handle_resolve(error_code const& ec,
        tcp::resolver::iterator endpoint_iterator,
        connect_callback cb)
{
    if (!ec) {
        boost::asio::async_connect(socket_, endpoint_iterator, std::bind(
            &tcp_transport::handle_connect, this,
                std::placeholders::_1, cb
        ));
    } else if (cb) {
        local_log(logger::ERROR) << "Failed to resolve";
        cb(::std::make_exception_ptr( errors::resolve_failed(ec.message()) ));
    }
}

void
tcp_transport::handle_connect(error_code const& ec, connect_callback cb)
{
    if (cb) {
        if (!ec) {
            cb(nullptr);
        } else {
            cb(::std::make_exception_ptr( errors::connection_refused(ec.message()) ));
        }
    }
}
void
tcp_transport::disconnect()
{
    if (socket_.is_open()) {
        socket_.close();
    }
}

//----------------------------------------------------------------------------
ssl_transport::ssl_transport(io_service& io_service,
        ::std::string const& host,
        ::std::string const& service,
        connect_callback cb) :
    resolver_(io_service),
    socket_( io_service,
            boost::asio::use_service<
                tip::ssl::ssl_context_service>(io_service).context() )
{
    tcp::resolver::query qry(host, service);
    resolver_.async_resolve(qry, std::bind(
        &ssl_transport::handle_resolve, this,
            std::placeholders::_1, std::placeholders::_2, cb
    ));
}

void
ssl_transport::handle_resolve(error_code const& ec,
        tcp::resolver::iterator endpoint_iterator,
        connect_callback cb)
{
    if (!ec) {
        socket_.set_verify_mode(boost::asio::ssl::verify_peer);
        socket_.set_verify_callback(std::bind(
            &ssl_transport::verify_certificate,
            this, std::placeholders::_1, std::placeholders::_2
        ));
        boost::asio::async_connect(socket_.lowest_layer(),
            endpoint_iterator, std::bind(
            &ssl_transport::handle_connect, this,
                std::placeholders::_1, cb
        ));
    } else if (cb) {
        cb(::std::make_exception_ptr( errors::resolve_failed(ec.message()) ));
    }
}

bool
ssl_transport::verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx)
{
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    local_log() << "Verifying " << subject_name;
    return preverified;
}

void
ssl_transport::handle_connect(error_code const& ec, connect_callback cb)
{
    if (!ec) {
        socket_.async_handshake(boost::asio::ssl::stream_base::client,
            std::bind(&ssl_transport::handle_handshake, this,
                std::placeholders::_1, cb));
    } else if (cb) {
        cb(::std::make_exception_ptr( errors::connection_refused(ec.message()) ));
    }
}

void
ssl_transport::handle_handshake(error_code const& ec, connect_callback cb)
{
    if (cb) {
        if (!ec) {
            cb(nullptr);
        } else {
            cb(::std::make_exception_ptr( errors::ssl_handshake_failed(ec.message()) ));
        }
    }
}

void
ssl_transport::disconnect()
{
    if (socket_.lowest_layer().is_open())
        socket_.lowest_layer().close();
}

} /* namespace client */
} /* namespace http */
} /* namespace tip */
