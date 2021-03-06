/*
 * errors.hpp
 *
 *  Created on: Feb 1, 2016
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_CLIENT_ERRORS_HPP_
#define PUSHKIN_HTTP_CLIENT_ERRORS_HPP_

#include <exception>

namespace psst {
namespace http {
namespace client {
namespace errors {

class http_client_error : public ::std::runtime_error {
public:
    http_client_error(std::string const& msg) : ::std::runtime_error{msg} {}
    http_client_error(char const* msg) : ::std::runtime_error{msg} {}
};

class resolve_failed : public http_client_error {
public:
    resolve_failed(std::string const& msg) : http_client_error{msg} {}
    resolve_failed(char const* msg) : http_client_error{msg} {}
};

class connection_refused : public http_client_error {
public:
    connection_refused(std::string const& msg) : http_client_error{msg} {}
    connection_refused(char const* msg) : http_client_error{msg} {}
};

class ssl_handshake_failed : public http_client_error {
public:
    ssl_handshake_failed(std::string const& msg) : http_client_error{msg} {}
    ssl_handshake_failed(char const* msg) : http_client_error{msg} {}
};

class connection_broken : public http_client_error {
public:
    connection_broken(std::string const& msg) : http_client_error{msg} {}
    connection_broken(char const* msg) : http_client_error{msg} {}
};

}  // namespace errors
}  // namespace client
}  // namespace http
}  // namespace psst



#endif /* PUSHKIN_HTTP_CLIENT_ERRORS_HPP_ */
