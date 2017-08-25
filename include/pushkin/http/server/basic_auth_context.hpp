/*
 * basic_auth_context.hpp
 *
 *  Created on: Jun 27, 2017
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_SERVER_BASIC_AUTH_CONTEXT_HPP_
#define PUSHKIN_HTTP_SERVER_BASIC_AUTH_CONTEXT_HPP_

#include <pushkin/http/server/reply.hpp>
#include <pushkin/http/server/reply_context.hpp>

namespace psst {
namespace http {
namespace server {

class basic_auth_context: public reply::context {
public:
    static reply::id id;
public:
    basic_auth_context(reply r);
    virtual ~basic_auth_context();

    ::std::string const&
    user() const
    { return user_; }
    ::std::string const&
    password() const
    { return password_; }

    bool
    empty() const
    { return user_.empty(); }
private:
    ::std::string user_;
    ::std::string password_;
};

} /* namespace server */
} /* namespace http */
} /* namespace psst */

#endif /* PUSHKIN_HTTP_SERVER_BASIC_AUTH_CONTEXT_HPP_ */
