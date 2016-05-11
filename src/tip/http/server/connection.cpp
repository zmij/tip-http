/**
 * @file /tip-server/src/tip/http/server/connection.cpp
 * @brief
 * @date Jul 8, 2015
 * @author: zmij
 */

#include <tip/http/server/connection.hpp>
#include <tip/http/server/request_handler.hpp>
#include <tip/http/server/remote_address.hpp>
#include <tip/http/server/reply_context.hpp>

#include <tip/http/common/request.hpp>
#include <tip/http/common/response.hpp>
#include <tip/http/common/grammar/response_generate.hpp>

#include <tip/http/version.hpp>

#include <tip/log.hpp>

#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/lexical_cast.hpp>

#include <vector>
#include <functional>
#include <chrono>

namespace tip {
namespace http {
namespace server {

LOCAL_LOGGING_FACILITY(HTTPCONN, TRACE);

const ::std::size_t LOG_MAX_REQUEST_BYTES = 500;

connection::connection(io_service_ptr io_service,
                       request_handler_ptr handler) :
    io_service_(io_service),
    strand_(*io_service),
    socket_(*io_service),
    request_handler_(handler),
    default_headers_{
        { Server, "tip-http-server/" + tip::VERSION }
    }
{
}

connection::~connection()
{
}

boost::asio::ip::tcp::socket& connection::socket()
{
    return socket_;
}

void
connection::start(endpoint_ptr peer)
{
    local_log() << "Start new connection with " << peer->address();
    peer_ = peer;
    read_request_headers();
}

void
connection::read_request_headers()
{
    boost::asio::async_read_until(socket_, incoming_, "\r\n\r\n",
        strand_.wrap(std::bind(&connection::handle_read_headers,
            shared_from_this(),
            std::placeholders::_1, std::placeholders::_2)));
}

void
connection::handle_read_headers(const boost::system::error_code& e,
                                std::size_t)
{
    if (!e) {
        std::istream is(&incoming_);
        request_ptr req = std::make_shared< request >();
        if (req->read_headers(is)) {
            try {
            read_request_body(req, req->read_body(is));
            } catch (::std::exception const& e) {
                local_log(logger::ERROR) << "Error reading body: " << e.what();
                send_error(req, response_status::bad_request);
            }
        } else {
            // Bad request
            local_log(logger::ERROR) << "Error parsing headers";
            send_error(req, response_status::bad_request);
        }
    } else {
        local_log(logger::DEBUG) << "Error reading request headers: "
                << e.message();
    }
}

void
connection::read_request_body(request_ptr req, read_result_type res)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    if (res.result) {
        // success read
        io_service_ptr io = io_service_.lock();
        if (io) {
            req->start_ = boost::posix_time::microsec_clock::local_time();
            reply rep{
                io,
                req,
                strand_.wrap(std::bind(&connection::send_response,
                        shared_from_this(), req, std::placeholders::_1)),
                strand_.wrap(std::bind(&connection::send_error,
                        shared_from_this(), req, std::placeholders::_1))
            };
            add_context(rep, new remote_address(rep, peer_));
            try {
            request_handler_->handle_request(rep);
            } catch (::std::exception const& e) {
                local_log(logger::ERROR) << "Exception when dispatching request "
                        << req->path << ": " << e.what();
            } catch (...) {
                local_log(logger::ERROR) << "Unknown exception when dispatching request "
                        << req->path;
            }
        } else {
            local_log(logger::WARNING) << "IO service weak pointer is bad";
        }
    } else if (!res.result) {
        // fail read
        local_log(logger::WARNING) << "Failed to read request body";
        send_error(req, response_status::bad_request);
    } else if (res.callback) {
        boost::asio::async_read(socket_, incoming_,
            boost::asio::transfer_at_least(1),
            strand_.wrap( std::bind( &connection::handle_read_body,
                shared_from_this(), _1, _2, req, res.callback) ));
    } else {
        // need more data but no callback
        local_log(logger::WARNING) << "Request read body returned "
                "indeterminate, but provided no callback";
    }
}

void
connection::handle_read_body(const boost::system::error_code& e,
                                std::size_t,
                                request_ptr req,
                                read_callback cb)
{
    if (!e) {
        std::istream is(&incoming_);
        read_request_body(req, cb(is));
    } else {
        local_log(logger::DEBUG) << "Error reading request body: "
                << e.message();
    }

    // If an error occurs then no new asynchronous operations are started. This
    // means that all shared_ptr references to the connection object will
    // disappear and the object will be destroyed automatically after this
    // handler returns. The connection class's destructor closes the socket.
}

void
connection::send_response(request_ptr req, response_const_ptr resp)
{
    typedef std::vector< boost::asio::const_buffer > output_buffers_type;
    using std::placeholders::_1;
    using std::placeholders::_2;

    {
        auto proc_time = boost::posix_time::microsec_clock::local_time() - req->start_;
        if (proc_time.is_not_a_date_time()) {
            local_log(logger::DEBUG) << req->method << " " << req->path
                    << " " << resp->status << " '" << resp->status_line << "' process time: 00:00:00.0";
        } else {
    local_log(logger::DEBUG) << req->method << " " << req->path
                    << " " << resp->status << " '" << resp->status_line << "' process time: " << proc_time;
        }
    }
    if (is_error(resp->status) && (HTTPCONN_DEFAULT_SEVERITY != logger::OFF)) {
        local_log() << "Request headers:\n" << req->headers_;
        if (!req->body_.empty()) {
            ::std::size_t bs = req->body_.size() < LOG_MAX_REQUEST_BYTES ?
                    req->body_.size() : LOG_MAX_REQUEST_BYTES;
            ::std::string body(req->body_.begin(), req->body_.begin() + bs);
            local_log() << "Request body (max " << LOG_MAX_REQUEST_BYTES << " bytes):\n"
                    << body;
        }
    }

    std::ostream os(&outgoing_);
    os << *resp;
    if (!resp->headers_.count(ContentLength)) {
        header content_length{
            ContentLength,
            boost::lexical_cast<std::string>( resp->body_.size() )
        };
        os << content_length;
    }
    if (!default_headers_.empty()) {
        os << default_headers_;
    }
    os << "\r\n";

    output_buffers_type buffers;
    buffers.push_back(boost::asio::buffer(outgoing_.data()));
    buffers.push_back(boost::asio::buffer(resp->body_));
    boost::asio::async_write(socket_, buffers,
        strand_.wrap(std::bind(&connection::handle_write_response,
            shared_from_this(), _1, _2, resp)));
}

void
connection::send_error(request_ptr req, response_status status)
{
    response_const_ptr resp = response::stock_response(status);
    send_response(req, resp);
}

void
connection::handle_write(const boost::system::error_code& e)
{
    if (!e) {
        // Initiate graceful connection closure.
        boost::system::error_code ignored_ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                ignored_ec);
    }

    // TODO Keep-Alive and timer
    // No new asynchronous operations are started. This means that all shared_ptr
    // references to the connection object will disappear and the object will be
    // destroyed automatically after this handler returns. The connection class's
    // destructor closes the socket.
}

void
connection::handle_write_response(boost::system::error_code const& e,
        size_t, response_const_ptr)
{
    if (!e) {
        // Initiate graceful connection closure.
        boost::system::error_code ignored_ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                ignored_ec);
    }

    // TODO Keep-Alive and timer
    // No new asynchronous operations are started. This means that all shared_ptr
    // references to the connection object will disappear and the object will be
    // destroyed automatically after this handler returns. The connection class's
    // destructor closes the socket.
}

} // namespace server
} // namespace http
}  // namespace tip
