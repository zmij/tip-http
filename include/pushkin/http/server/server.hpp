/**
 * @file /tip-server/include/tip/http/server/server.hpp
 * @brief
 * @date Jul 8, 2015
 * @author: zmij
 */

#ifndef PUSHKIN_HTTP_SERVER_SERVER_HPP
#define PUSHKIN_HTTP_SERVER_SERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <memory>
#include <atomic>

#include <pushkin/http/server/connection.hpp>
#include <pushkin/http/server/request_handler.hpp>

namespace psst {
namespace http {
namespace server {

/**
 * The top-level class of the HTTP server.
 */
class server: private boost::noncopyable {
public:
    typedef std::function< void () > stop_function;
    typedef std::shared_ptr< boost::asio::io_service > io_service_ptr;
    typedef std::shared_ptr< boost::asio::ip::tcp::endpoint> endpoint_ptr;
    using endpoint = boost::asio::ip::tcp::endpoint;
public:
    /**
     * Construct the server to listen on the specified TCP address and port, and
     * serve up files from the given directory.
     * @param address TCP address to listen to
     * @param port TCP port to bind
     * @param thread_pool_size Size of the thread pool
     * @param handler
     */
    explicit
    server(io_service_ptr io_svc,
            std::string const& address,
            std::string const& port,
            std::size_t thread_pool_size,
            request_handler_ptr handler,
            bool start_accept_,
            stop_function = stop_function(),
            bool reg_signals = true);

    /**
     * Run the server's io_service loop.
     */
    void
    run();

    /**
     * Initiate an asynchronous accept operation.
     */
    void
    start_accept();
    /**
     * Cancel accepting new connections.
     */
    void
    stop_accept();

    endpoint
    local_endpoint() const;
private:
    /**
     * Handle completion of an asynchronous accept operation.
     * @param e
     */
    void
    handle_accept(boost::system::error_code const& e, endpoint_ptr);

    /**
     * Initiate an asynchronous accept operation.
     */
    void
    start_accept_silent();

    /**
     * Handle a request to stop the server.
     */
    void
    handle_stop();

    /**
     * Handle a hard exception
     * @param e
     * @param signo
     */
    void
    handle_error(boost::system::error_code const& e, int signo);

    void
    register_signals();
private:
    /** The io_service used to perform asynchronous operations. */
    io_service_ptr io_service_;

    /** The number of threads that will call io_service::run(). */
    std::size_t thread_pool_size_;

    /** The signal_set is used to register for process termination notifications. */
    boost::asio::signal_set signals_;

    /** Acceptor used to listen for incoming connections. */
    boost::asio::ip::tcp::acceptor acceptor_;

    /** The next connection to be accepted. */
    connection_ptr      new_connection_;

    /** The handler for all incoming requests. */
    request_handler_ptr request_handler_;

    stop_function       stop_;
    ::std::atomic_flag  stopped_;
    ::std::atomic<bool> accepting_;
};

} // namespace server
} // namespace http
} // namespace psst

#endif /* PUSHKIN_HTTP_SERVER_SERVER_HPP */
