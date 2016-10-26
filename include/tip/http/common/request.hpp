/*
 * request.hpp
 *
 *  Created on: Aug 17, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_REQUEST_HPP_
#define TIP_HTTP_COMMON_REQUEST_HPP_

#include <tip/http/common/header.hpp>
#include <tip/http/common/request_fwd.hpp>
#include <tip/iri.hpp>
#include <tip/util/read_result.hpp>

#include <memory>
#include <boost/asio/buffer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace tip {
namespace http {

class request {
public:
    using version_type      = std::pair< std::int32_t, std::int32_t >;
    using query_param_type  = std::pair< std::string, std::string >;
    using query_type        = std::multimap< std::string, std::string >;
    using body_type         = std::vector< char >;
    using timestamp_type    = boost::posix_time::ptime;
    using iri_type          = tip::iri::basic_iri<query_type>;
    using read_result_type  = util::read_result< std::istream& >;
    using read_callback     = read_result_type::read_callback_type;


    request_method      method;
    version_type        version;
    iri::path           path;
    query_type          query;
    iri::fragment       fragment;
    headers mutable     headers_;

    body_type           body_;

    timestamp_type      start_;

    ::std::size_t       serial;

    request();
    request(request_method m,
        iri::path const&, query_type const&, iri::fragment const&,
        headers const&, body_type const& body = body_type{});
    request(request_method m,
        iri::path const&, query_type const&, iri::fragment const&,
        headers const&, body_type&& body);

    request(request const& rhs);
    request(request&& rhs);

    void
    swap(request& rhs) noexcept
    {
        using ::std::swap;
        swap(method,    rhs.method);
        swap(version,   rhs.version);
        swap(path,      rhs.path);
        swap(query,     rhs.query);
        swap(fragment,  rhs.fragment);
        swap(headers_,  rhs.headers_);
        swap(body_,     rhs.body_);
        swap(start_,    rhs.start_);
        swap(serial,    rhs.serial);
    }

    request&
    operator = (request const& rhs)
    {
        request tmp{rhs};
        swap(tmp);
        return *this;
    }
    request&
    operator = (request&& rhs)
    {
        swap(rhs);
        return *this;
    }

    size_t
    content_length() const;

    void
    host(tip::iri::host const&);
    tip::iri::host
    host() const;

    bool
    read_headers(std::istream&);

    read_result_type
    read_body(std::istream&);
public:
    static request_ptr
    create(request_method method, iri_type const& iri,
            body_type const& body = body_type());

    static request_ptr
    create(request_method method, iri_type const& iri,
            body_type&& body);

    static request_ptr
    create(request_method method, std::string const& iri_str);

    static bool
    parse_iri(std::string const&, iri_type&);
private:
    static ::std::size_t
    get_number();

    read_result_type
    read_body_content_length(std::istream&, size_t remain);
};

std::ostream&
operator << (std::ostream&, request const&);

} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_COMMON_REQUEST_HPP_ */
