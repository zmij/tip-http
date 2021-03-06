/*
 * json_request_handler.hpp
 *
 *  Created on: Oct 27, 2016
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_SERVER_JSON_REQUEST_HANDLER_HPP_
#define PUSHKIN_HTTP_SERVER_JSON_REQUEST_HANDLER_HPP_

#include <atomic>
#include <type_traits>
#include <pushkin/http/server/json_body_context.hpp>
#include <pushkin/http/server/prerequisite_handler.hpp>

namespace psst {
namespace http {
namespace server {

struct json_reply {
public:
    using io_service_ptr    = reply::io_service_ptr;
public:
    explicit
    json_reply(reply r)
        : r_{r}, sent_{ ::std::make_shared< ::std::atomic_flag >(false) } {}
    json_reply(json_reply const&) = default;
    json_reply(json_reply&&) = default;

    json_reply&
    operator = (json_reply const&) = default;
    json_reply&
    operator = (json_reply&&) = default;

    io_service_ptr
    io_service() const
    { return r_.io_service(); }

    tip::iri::path const&
    path() const
    { return r_.path(); }

    //@{
    /** @name Query */
    reply::query_type const&
    query() const
    { return r_.query(); }

    bool
    has_query_parameter(::std::string const& name) const
    {
        return r_.has_query_parameter(name);
    }

    template <typename T>
    bool
    get_query_parameter(::std::string const& name, T& val) const
    {
        return r_.get_query_parameter(name, val);
    }
    template <typename T, typename ParseFunc>
    bool
    get_query_parameter(::std::string const& name, T& val, ParseFunc parse_func) const
    {
        return r_.get_query_parameter(name, val, parse_func);
    }
    //@}

    headers const&
    request_headers() const
    { return r_.request_headers(); }

    void
    add_header(header const& h) const
    {
        r_.add_header(h);
    }
    void
    add_header(header&& h) const
    {
        r_.add_header(::std::move(h));
    }

    void
    on_finish(reply::finished_func f)
    {
        r_.on_finish(f);
    }

    template < typename T >
    bool
    parse(T& val, bool allow_empty_body = false)
    {
        auto& json = use_context<json_body_context>(r_);
        if ((bool)json) {
            try {
                json.input(val);
                return true;
            } catch(::std::exception const& e) {
                throw json_error{e.what()};
            } catch(...) {
                throw json_error("Invalid JSON request");
            }
        }
        switch (json.status()) {
            case json_body_context::empty_body:
                if (!allow_empty_body)
                    throw json_error("Empty body (expected JSON object)");
                break;
            case json_body_context::invalid_json:
                throw json_error("Malformed JSON");
            default:
                throw json_error("Unknown JSON status");
        }
        return false;
    }

    template < typename T >
    void
    done(T&& body, response_status status = response_status::ok) const
    {
        if (!sent_->test_and_set()) {
            r_.add_header({ ContentType, "application/json; charset=utf8" });
            r_.response_body().clear();
            auto& json = use_context<json_body_context>(r_);
            json << ::std::forward<T>(body);
            r_.done(status);
        } else {
            log_already_sent();
        }
    }
    template < typename T >
    void
    operator()(T&& body, response_status status = response_status::ok) const
    { done(::std::forward<T>(body), status); }

    template < typename ... T >
    void
    operator()(response_status status, T&& ... items) const
    {
        if (!sent_->test_and_set()) {
            r_.add_header({ ContentType, "application/json; charset=utf8" });
            r_.response_body().clear();
            auto& json = use_context<json_body_context>(r_);
            serialize_response(json, ::std::forward<T>(items) ...);
            r_.done(status);
        } else {
            log_already_sent();
        }
    }
    void
    error(http::server::error const& e) const
    {
        e.log_error();
        done(e, e.status());
    }
    void
    operator()(http::server::error const& e) const
    { error(e); }

    void
    client_error(response_status s = response_status::bad_request)
    { r_.client_error(s); }
    void
    server_error(response_status s = response_status::internal_server_error)
    { r_.server_error(s); }

    template < typename Context >
    friend bool
    has_context(json_reply const& r);
    template < typename Context >
    friend Context&
    use_context(json_reply& r);
    template < typename Context >
    friend Context const&
    use_context(json_reply const& r);

    void
    raw_response(::std::string const& body,
            response_status status = response_status::ok) const
    {
        if (!sent_->test_and_set()) {
            r_.add_header({ ContentType, "application/json; charset=utf8" });
            r_.response_body().clear();
            ::std::copy(body.begin(), body.end(), ::std::back_inserter(r_.response_body()));
            r_.done(status);
        } else {
            log_already_sent();
        }
    }

    reply::body_type const&
    request_body() const
    {
        return r_.request_body();
    }
private:
    void
    log_already_sent() const;

    template < typename T >
    void
    serialize_response(json_body_context& json, T&& body ) const
    {
        json << ::std::forward<T>(body);
    }

    template < typename T, typename ... U >
    void
    serialize_response(json_body_context& json, T&& head, U&& ... tail) const
    {
        json << ::std::forward<T>(head);
        serialize_response(json, ::std::forward<U>(tail) ...);
    }

    using shared_flag = ::std::shared_ptr<::std::atomic_flag>;

    reply mutable   r_;
    shared_flag     sent_;
};

template < typename Context >
bool
has_context(json_reply const& r)
{
    return has_context<Context>(r.r_);
}

template < typename Context >
Context&
use_context(json_reply& r)
{
    return use_context<Context>(r.r_);
}

template < typename Context >
Context const&
use_context(json_reply const& r)
{
    return use_context<Context>(r.r_);
}


template < typename T >
struct json_request_handler {
    using handler_type  = T;
    using base_type     = json_request_handler< T >;
    // Pull the json_reply name into the class's name scope
    using json_reply    = http::server::json_reply;

    void
    operator()(reply r)
    {
        rebind()(json_reply{r});
    }
private:
    handler_type&
    rebind()
    {
        return static_cast<handler_type&>(*this);
    }
    handler_type&
    rebind() const
    {
        return static_cast<handler_type const&>(*this);
    }
};

template < typename T, typename BodyType >
struct json_transform_handler :
        json_request_handler< json_transform_handler<T, BodyType> >{
    using base_type         = json_request_handler< json_transform_handler<T, BodyType> >;
    using handler_type      = T;
    using request_body_type = BodyType;
    using request_pointer   = ::std::shared_ptr<BodyType>;

    using base_type::operator();
    void
    operator()(json_reply r)
    {
        try {
            request_pointer req{ ::std::make_shared<request_body_type>() };
            if (r.parse(*req)) {
                rebind()(r, req);
            }
        } catch ( ::psst::http::server::error const& e ) {
            r.error(e);
        } catch (::std::exception const& e) {
            r.error(::psst::http::server::error{"REQUEST", e.what(), response_status::ok});
        } catch (...) {
            r.error(::psst::http::server::error{"REQUEST", "Unexpected exception", response_status::ok});
        }
    }
private:
    handler_type&
    rebind()
    {
        return static_cast<handler_type&>(*this);
    }
    handler_type&
    rebind() const
    {
        return static_cast<handler_type const&>(*this);
    }
};

template < typename T, typename ... Prerequisite >
class json_prerequisite_handler : public json_request_handler< T > {
    using prerequisites_type = prerequisites<Prerequisite...>;
    using json_handler_type = json_request_handler<T>;
public:
    using this_type         = json_prerequisite_handler<T, Prerequisite...>;
    using base_type         = this_type;

    void
    operator()(reply r)
    {
        try {
            if (check_prerequisites(r)) {
                json_handler_type::operator ()(r);
            }
        } catch (...) {
            r.server_error();
        }
    }

    bool
    check_prerequisites( reply const& r ) const
    {
        return prerequisites_(r);
    }
private:
    prerequisites_type  prerequisites_;
};

template < typename T, typename BodyType, typename ... Prerequisite >
class json_transform_prerequisite_handler : public json_transform_handler<T, BodyType> {
    using prerequisites_type = prerequisites<Prerequisite...>;
    using json_handler_type = json_transform_handler<T, BodyType>;
public:
    using this_type         = json_transform_prerequisite_handler<T, BodyType, Prerequisite...>;
    using base_type         = this_type;

    void
    operator()(reply r)
    {
        try {
            if (check_prerequisites(r)) {
                json_handler_type::operator ()(r);
            }
        } catch (...) {
            r.server_error();
        }
    }

    bool
    check_prerequisites( reply const& r ) const
    {
        return prerequisites_(r);
    }
private:
    prerequisites_type  prerequisites_;
};

}  /* namespace server */
}  /* namespace http */
}  /* namespace psst */



#endif /* PUSHKIN_HTTP_SERVER_JSON_REQUEST_HANDLER_HPP_ */
