/*
 * request_dispatcher.cpp
 *
 *  Created on: Jul 17, 2015
 *      Author: zmij
 */

#include <tip/http/server/request_dispatcher.hpp>
#include <tip/http/server/detail/path_matcher.hpp>
#include <tip/log.hpp>
#include <map>
#include <unordered_set>
#include <mutex>

namespace tip {
namespace http {
namespace server {

LOCAL_LOGGING_FACILITY(REQDISP, TRACE);

struct request_dispatcher::impl {
    using iri_handlers      = ::std::map< request_method, handler_closure >;
    using handler_map_type  = ::std::map< detail::path_matcher, iri_handlers >;
    using silent_path_type  = ::std::unordered_set< std::string >;
    using mutex_type        = ::std::mutex;
    using lock_type         = ::std::lock_guard<mutex_type>;

    handler_map_type    handlers_;
    silent_path_type    silent_;
    mutex_type          mutex_;

    void
    add_handler(request_method method, std::string const& path,
        request_handler_ptr handler)
    {
        add_handler(method, path,
                ::std::bind( &request_handler::handle_request, handler,
                        ::std::placeholders::_1 ) );
    }

    void
    add_handler(request_method method, ::std::string const& path,
            handler_closure handler)
    {
        lock_type lock{mutex_};
        detail::path_matcher matcher{ path };

        auto p = handlers_.find(matcher);
        if (p == handlers_.end()) {
            p = handlers_.insert(std::make_pair(matcher, iri_handlers{})).first;
        }
        local_log(logger::DEBUG) << "Add " << method << "\thandler: " << matcher;
        p->second.insert(std::make_pair(method, handler));
    }

    handler_closure
    get_handler(reply& r)
    {
        lock_type lock{mutex_};
        auto p = std::find_if(handlers_.begin(), handlers_.end(),
        [&]( handler_map_type::value_type const& h ) {
            return h.first.matches(r.path());
        });
        if (p == handlers_.end()) {
            local_log(logger::WARNING) << "Failed to find handler for " << r.path();
            r.client_error(response_status::not_found);
            return {};
        }
        // local_log() << "Handler found " << p->first;
        auto h = p->second.find(r.method());
        if (h == p->second.end()) {
            local_log(logger::WARNING) << "Handler found, but not for " << r.method();
            r.client_error(response_status::method_not_allowed);
            return {};
        }
        return h->second;
    }
    void
    dispatch_request(reply& r)
    {
        handler_closure handler = get_handler(r);
        if (handler) {
            handler(r);
        }
    }

    void
    make_silent(std::string const& path)
    {
        silent_.insert(path);
    }

    bool
    is_silent(tip::iri::path const& path)
    {
        std::stringstream ss;
        ss << path;
        return silent_.count(ss.str()) > 0;
    }
};

request_dispatcher::request_dispatcher() : pimpl_(new impl())
{
}

request_dispatcher::~request_dispatcher()
{
}

void
request_dispatcher::add_handler( request_method method, std::string const& path,
        request_handler_ptr handler )
{
    pimpl_->add_handler(method, path, handler);
}

void
request_dispatcher::add_handler(
        request_method_set const& methods,
        std::string const& path, request_handler_ptr handler)
{
    for (auto method : methods) {
        pimpl_->add_handler(method, path, handler);
    }
}

void
request_dispatcher::add_handler(request_method method, std::string const& path,
        handler_closure func)
{
    pimpl_->add_handler(method, path, func);
}

void
request_dispatcher::add_handler(request_method_set methods, std::string const& path,
        handler_closure func)
{
    for (auto method : methods) {
        pimpl_->add_handler(method, path, func);
    }
}

request_dispatcher::add_handlers_helper
request_dispatcher::add_handlers()
{
    return add_handlers_helper{ shared_this< request_dispatcher >() };
}

void
request_dispatcher::make_silent(std::string const& path)
{
    pimpl_->make_silent(path);
}

bool
request_dispatcher::is_silent(tip::iri::path const& path)
{
    return pimpl_->is_silent(path);
}

void
request_dispatcher::get(std::string const& path, request_handler_ptr handler)
{
    pimpl_->add_handler(GET, path, handler);
}

void
request_dispatcher::post(std::string const& path, request_handler_ptr handler)
{
    pimpl_->add_handler(POST, path, handler);
}

void
request_dispatcher::do_handle_request(reply r)
{
    // local_log() << "Dispatch request for " << r.path();
    pimpl_->dispatch_request(r);
}

//-----------------------------------------------------------------------------
request_dispatcher::add_handlers_helper::add_handlers_helper(request_dispatcher_ptr disp)
    : owner_(disp)
{
}

request_dispatcher::add_handlers_helper&
request_dispatcher::add_handlers_helper::operator ()
    (request_method method, std::string const& path, handler_closure func)
{
    owner_->add_handler(method, path, func);
    return *this;
}

request_dispatcher::add_handlers_helper&
request_dispatcher::add_handlers_helper::operator ()
    (request_method_set methods, std::string const& path, handler_closure func)
{
    owner_->add_handler(methods, path, func);
    return *this;
}

} /* namespace server */
} /* namespace http */
} /* namespace tip */
