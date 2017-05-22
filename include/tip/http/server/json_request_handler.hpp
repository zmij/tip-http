/*
 * json_request_handler.hpp
 *
 *  Created on: Oct 27, 2016
 *      Author: zmij
 */

#ifndef HTTP_SERVER_JSON_REQUEST_HANDLER_HPP_
#define HTTP_SERVER_JSON_REQUEST_HANDLER_HPP_

#include <tip/http/server/json_body_context.hpp>
#include <atomic>
#include <type_traits>

namespace tip {
namespace http {
namespace server {

struct json_reply {
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

    tip::iri::path const&
    path() const
    { return r_.path(); }

    reply::query_type const&
    query() const
    { return r_.query(); }

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
    void
    error(http::server::error const& e) const
    {
        e.log_error();
        done(e, e.status());
    }
    void
    operator()(http::server::error const& e) const
    { error(e); }

    template < typename Context >
    friend bool
    has_context(json_reply& r);
    template < typename Context >
    friend Context&
    use_context(json_reply& r);

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
private:
    void
    log_already_sent() const;
    using shared_flag = ::std::shared_ptr<::std::atomic_flag>;

    reply mutable   r_;
    shared_flag     sent_;
};

template < typename Context >
bool
has_context(json_reply& r)
{
    return has_context<Context>(r.r_);
}

template < typename Context >
Context&
use_context(json_reply& r)
{
    return use_context<Context>(r.r_);
}

template < typename T >
struct json_request_handler {
    using handler_type = T;
    using base_type = json_request_handler< T >;

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
    using base_type = json_request_handler< json_transform_handler<T, BodyType> >;
    using handler_type = T;
    using request_body_type     = BodyType;
    using request_pointer       = ::std::shared_ptr<BodyType>;

    using base_type::operator();
    void
    operator()(json_reply r)
    {
        try {
            request_pointer req{ ::std::make_shared<request_body_type>() };
            if (r.parse(*req)) {
                rebind()(r, req);
            }
        } catch ( ::tip::http::server::error const& e ) {
            r.error(e);
        } catch (::std::exception const& e) {
            r.error(::tip::http::server::error{"REQUEST", e.what(), response_status::ok});
        } catch (...) {
            r.error(::tip::http::server::error{"REQUEST", "Unexpected exception", response_status::ok});
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

}  /* namespace server */
}  /* namespace http */
}  /* namespace tip */



#endif /* HTTP_SERVER_JSON_REQUEST_HANDLER_HPP_ */
