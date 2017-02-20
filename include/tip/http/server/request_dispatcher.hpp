/*
 * request_dispatcher.hpp
 *
 *  Created on: Jul 17, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_REQUEST_DISPATCHER_HPP_
#define TIP_HTTP_SERVER_REQUEST_DISPATCHER_HPP_

#include <tip/http/server/request_handler.hpp>
#include <set>

namespace tip {
namespace http {
namespace server {

struct reply;
struct request;

class request_dispatcher: public tip::http::server::request_handler {
public:
    using request_method_set = std::set<request_method>;
    using handler_closure = ::std::function< void ( reply ) >;
public:
    class add_handlers_helper;
public:
    request_dispatcher();
    virtual ~request_dispatcher();

    void
    add_handler( request_method, std::string const&, request_handler_ptr );
    void
    add_handler( request_method_set const&, std::string const&, request_handler_ptr );

    template < typename T >
    void
    add_handler( request_method method, std::string const& path)
    {
        add_handler( method, path, std::make_shared< T >() );
    }
    template < typename T >
    void
    add_handler( request_method_set const& methods, std::string const& path)
    {
        add_handler( methods, path, std::make_shared< T >() );
    }

    template < typename T, typename ... U >
    void
    add_handler( request_method method, std::string const& path, U&& ... args )
    {
        add_handler( method, path, std::make_shared< T >( std::forward<U>(args) ... ) );
    }
    template < typename T, typename ... U >
    void
    add_handler( request_method_set const& methods, std::string const& path, U&& ... args )
    {
        add_handler( methods, path, std::make_shared< T >( std::forward<U>(args) ... ) );
    }

    void
    add_handler(request_method method, std::string const& path, handler_closure func);
    void
    add_handler(request_method_set methods, std::string const& path, handler_closure func);

    void
    make_silent(std::string const& path);
    bool
    is_silent(tip::iri::path const& path) override;

    add_handlers_helper
    add_handlers();

    void
    get(std::string const&, request_handler_ptr);
    void
    post(std::string const&, request_handler_ptr);
private:
    virtual void
    do_handle_request(reply) override;
private:
    struct impl;
    using pimpl = std::shared_ptr<impl>;
    pimpl pimpl_;
};

using request_dispatcher_ptr = std::shared_ptr<request_dispatcher>;

class request_dispatcher::add_handlers_helper {
public:
    add_handlers_helper(add_handlers_helper const&) = default;

    add_handlers_helper&
    operator()(request_method method, std::string const& path, handler_closure func);
    add_handlers_helper&
    operator()(request_method_set methods, std::string const& path, handler_closure func);
private:
    add_handlers_helper(request_dispatcher_ptr);
private:
    friend class request_dispatcher;
    request_dispatcher_ptr owner_;
};


} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_SERVER_REQUEST_DISPATCHER_HPP_ */
