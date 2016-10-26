/*
 * request.cpp
 *
 *  Created on: Aug 17, 2015
 *      Author: zmij
 */

#include <tip/http/common/request.hpp>
#include <tip/util/misc_algorithm.hpp>

#include <boost/spirit/include/qi.hpp>

#include <tip/http/common/grammar/request_parse.hpp>
#include <tip/http/common/grammar/request_generate.hpp>

#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>

#include <atomic>

namespace tip {
namespace http {

namespace  {

const std::string EMPTY_STRING;

::std::atomic<::std::size_t>    request_counter;

}  // namespace

tip::iri::host
host_name(std::string const& s)
{
    namespace qi = boost::spirit::qi;
    using string_iterator = std::string::const_iterator;
    using iauthority_grammar = iri::grammar::parse::iauthority_grammar<string_iterator>;

    string_iterator f = s.begin();
    string_iterator l = s.end();
    tip::iri::authority auth;
    if (qi::parse(f, l, iauthority_grammar(), auth)) {
        return auth.host;
    }
    return tip::iri::host();
}

request::request()
    : method{GET}, version{1, 1},
      path{}, query{}, fragment{}, headers_{}, body_{},
      start_{boost::posix_time::microsec_clock::universal_time()},
      serial{get_number()}
{
}

request::request(request_method m,
        iri::path const& p, query_type const& q, iri::fragment const& f,
        headers const& h, body_type const& body)
    : method{m}, version{1, 1},
      path{p}, query{q}, fragment{f}, headers_{h}, body_{body},
      start_{boost::posix_time::microsec_clock::universal_time()},
      serial{get_number()}
{
}

request::request(request_method m,
        iri::path const& p, query_type const& q, iri::fragment const& f,
        headers const& h, body_type&& body)
    : method{m}, version{1, 1},
      path{p}, query{q}, fragment{f}, headers_{h}, body_{::std::move(body)},
      start_{boost::posix_time::microsec_clock::universal_time()},
      serial{get_number()}
{
}

request::request(request const& rhs)
    : method{rhs.method}, version{rhs.version},
      path{rhs.path}, query{rhs.query}, fragment{rhs.fragment},
      headers_{rhs.headers_}, body_{rhs.body_},
      start_{rhs.start_},
      serial{rhs.serial}
{
}

request::request(request&& rhs)
    : method{::std::move(rhs.method)}, version{::std::move(rhs.version)},
      path{::std::move(rhs.path)}, query{::std::move(rhs.query)}, fragment{::std::move(rhs.fragment)},
      headers_{::std::move(rhs.headers_)}, body_{::std::move(rhs.body_)},
      start_{::std::move(rhs.start_)},
      serial{rhs.serial}
{
}

size_t
request::content_length() const
{
    return http::content_length(headers_);
}

void
request::host(tip::iri::host const& h)
{
    headers::iterator f = headers_.find(Host);
    if (f == headers_.end()) {
        headers_.insert({ Host, static_cast< std::string const& >(h) });
    } else {
        f->second = static_cast< std::string const& >(h);
    }
}

tip::iri::host
request::host() const
{
    headers::iterator f = headers_.find(Host);
    if (f != headers_.end()) {
        return host_name(f->second);
    }
    return tip::iri::host();
}

bool
request::read_headers(std::istream& is)
{
    namespace spirit = boost::spirit;
    namespace qi = boost::spirit::qi;

    using istreambuf_iterator = std::istreambuf_iterator<char>;
    using stream_iterator = boost::spirit::multi_pass< istreambuf_iterator >;
    using request_grammar = grammar::parse::request_grammar< stream_iterator >;
    static request_grammar grammar_;

    stream_iterator f = spirit::make_default_multi_pass(istreambuf_iterator(is));
    stream_iterator l = spirit::make_default_multi_pass(istreambuf_iterator());

    return (qi::parse(f, l, grammar_, *this));
}

request::read_result_type
request::read_body(std::istream& is)
{
    size_t cl = content_length();
    if (cl > 0) {
        return read_body_content_length(is, cl);
    }
    return {true, read_callback()};
}

request::read_result_type
request::read_body_content_length(std::istream& is, size_t remain)
{
    if (remain == 0) {
        // Don't need more data
        return read_result_type{true, read_callback{}};
    }
    is.unsetf(std::ios_base::skipws);
    std::istream_iterator<char> f(is);
    std::istream_iterator<char> l;
    size_t consumed = util::copy_max(f, l, remain, std::back_inserter(body_));
    if (consumed == remain) {
        // Don't need more data
        return read_result_type{true, read_callback{}};
    }

    return read_result_type{boost::indeterminate,
            std::bind(&request::read_body_content_length,
                    this, std::placeholders::_1, remain - consumed)};
}

std::ostream&
operator << (std::ostream& os, request const& val)
{
    namespace karma = boost::spirit::karma;
    using output_iterator = std::ostream_iterator<char>;
    using request_grammar = grammar::gen::request_grammar<output_iterator>;
    using header_grammar = grammar::gen::header_grammar<output_iterator>;

    std::ostream::sentry s(os);
    if (s) {
        std::ostream_iterator<char> out(os);
        val.headers_.erase(ContentLength);
        header content_length{
            ContentLength,
            boost::lexical_cast< std::string >(val.body_.size())
        };
        karma::generate(out, request_grammar(), val);
        karma::generate(out, header_grammar(), content_length);
    }
    return os;
}


request_ptr
request::create(request_method method, iri_type const& iri, body_type const& body)
{
    return request_ptr(new request{
        method,
        iri.path, iri.query, iri.fragment,
        headers {
            { Host, iri.authority.host }
        },
        body
    });
}

request_ptr
request::create(request_method method, iri_type const& iri, body_type&& body)
{
    return request_ptr(new request{
        method,
        iri.path, iri.query, iri.fragment,
        headers {
            { Host, iri.authority.host }
        },
        std::move(body)
    });
}


request_ptr
request::create(request_method method, std::string const& iri_str)
{
    iri_type iri;
    if (parse_iri(iri_str, iri))
        return create(method, iri);
    return request_ptr(); // TODO throw an exception?
}

bool
request::parse_iri(std::string const& iri_str, iri_type& iri)
{
    namespace qi = boost::spirit::qi;
    using string_iterator = std::string::const_iterator;
    using iri_grammar = iri::grammar::parse::iri_grammar< string_iterator,
            grammar::parse::query_grammar< string_iterator > >;
    iri_grammar parser;
    string_iterator f = iri_str.begin();
    string_iterator l = iri_str.end();
    return qi::parse(f, l, parser, iri) && f == l;
}

::std::size_t
request::get_number()
{
    return ++request_counter;
}

std::ostream&
operator << (std::ostream& out, request_method val)
{
    std::ostream::sentry s(out);
    if (s) {
        switch (val) {
            case GET :
                out << "GET";
                break;
            case HEAD :
                out << "HEAD";
                break;
            case POST :
                out << "POST";
                break;
            case PUT :
                out << "PUT";
                break;
            case DELETE :
                out << "DELETE";
                break;
            case OPTIONS :
                out << "OPTIONS";
                break;
            case TRACE :
                out << "TRACE";
                break;
            case CONNECT :
                out << "CONNECT";
                break;
            case PATCH :
                out << "PATCH";
                break;
            default:
                break;
        }
    }
    return out;
}


} /* namespace http */
} /* namespace tip */
