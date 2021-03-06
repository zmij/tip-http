/*
 * request_transformer.hpp
 *
 *  Created on: Oct 6, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_SERVER_REQUEST_TRANSFORMER_HPP_
#define PUSHKIN_HTTP_SERVER_REQUEST_TRANSFORMER_HPP_

#include <pushkin/http/server/error.hpp>
#include <pushkin/http/server/prerequisite_handler.hpp>
#include <pushkin/http/server/request_handler.hpp>

namespace psst {
namespace http {
namespace server {

/**
 * Template class to transform request from generic type to a concrete structure
 * and handle a typed request.
 * Transformer type must have an operator() that takes a reply object as a
 * parameter and returns a concrete structure. It also must have a member function
 * error(reply, error).
 * @tparam Transformer Class of request transformer.
 */
template < typename Transformer, typename ... Prerequisite >
class request_transformer : public request_handler {
public:
    using transformer_type = Transformer;
    using request_type = typename transformer_type::request_type;
    using request_pointer = typename transformer_type::pointer;
    using prerequisites_type = prerequisites< Prerequisite ... >;
public:
    request_transformer() : transform_() {}
    virtual ~request_transformer() {}
protected:
    void
    send_error(reply r, error const& e) const
    {
        transform_.error(r, e);
    }
private:
    virtual void
    do_handle_request(reply r)
    {
        try {
            if (prerequisites_(r)) {
                request_pointer req(transform_(r));
                do_handle_request(r, req);
            }
        } catch ( error const& e) {
            transform_.error(r, e);
        }
    }
    virtual void
    do_handle_request(reply, request_pointer) = 0;
private:
    transformer_type transform_;
    prerequisites_type prerequisites_;
};

template < typename Handler, typename Transformer, typename ... Prerequisite >
struct request_transformer_func {
    using base_type = request_transformer_func< Handler, Transformer, Prerequisite ... >;
    using static_type = Handler;
    using transformer_type = Transformer;
    using request_type = typename transformer_type::request_type;
    using request_pointer = typename transformer_type::pointer;
    using prerequisites_type = prerequisites< Prerequisite ... >;

    void
    operator()(reply r) const
    {
        try {
            if (prerequisites_type{}(r)) {
                request_pointer req(transformer_type{}(r));
                handler_type()(r, req);
            }
        } catch ( error const& e ) {
            send_error(r, e);
        }
    }
protected:
    static void
    send_error(reply r, error const& e)
    {
        transformer_type::error(r, e);
    }
private:
    static_type&
    hanlder_type()
    {
        return static_cast<static_type&>(*this);
    }
    static_type const&
    handler_type() const
    {
        return static_cast<static_type const&>(*this);
    }
};

}  // namespace server
}  // namespace http
}  // namespace psst

#endif /* PUSHKIN_HTTP_SERVER_REQUEST_TRANSFORMER_HPP_ */
