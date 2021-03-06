/**
 * response_parse.hpp
 *
 *  Created on: 22 авг. 2015 г.
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_COMMON_GRAMMAR_RESPONSE_PARSE_HPP_
#define PUSHKIN_HTTP_COMMON_GRAMMAR_RESPONSE_PARSE_HPP_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/fusion/include/std_pair.hpp>
#include <boost/spirit/include/phoenix.hpp>
#pragma GCC diagnostic pop

#include <pushkin/http/common/grammar/header_parse.hpp>
#include <pushkin/http/common/response.hpp>

namespace psst {
namespace http {
namespace grammar {
namespace parse {

// http://tools.ietf.org/html/rfc2616
template < typename InputIterator >
struct response_grammar :
        boost::spirit::qi::grammar< InputIterator, response() > {
    typedef response value_type;
    response_grammar() : response_grammar::base_type(root)
    {
        namespace qi = boost::spirit::qi;
        namespace phx = boost::phoenix;
        using qi::space;
        using qi::lit;
        using qi::int_;
        using qi::char_;
        using qi::_val;
        using qi::_1;

        version %= lit("HTTP/") >> int_ >> '.' >> int_;
        crlf = lit("\r\n");

        status_line %= +~char_("\r");
        root = version[ phx::bind(&response::version, _val) = _1 ] >> +space >>
                int_[ phx::bind(&response::status, _val) =
                        phx::static_cast_< response_status >(_1)] >> +space >>
                status_line[ phx::bind(&response::status_line, _val) = _1 ] >> crlf >>
                -_headers[ phx::bind(&response::headers_, _val) = _1 ] >> crlf;
    }
    boost::spirit::qi::rule< InputIterator, response() > root;
    boost::spirit::qi::rule< InputIterator, response::version_type() > version;
    boost::spirit::qi::rule< InputIterator, std::string() > status_line;
    boost::spirit::qi::rule< InputIterator > crlf;
    headers_grammar< InputIterator > _headers;
};

}  // namespace parse
}  // namespace grammar
}  // namespace http
}  // namespace psst

#endif /* PUSHKIN_HTTP_COMMON_GRAMMAR_RESPONSE_PARSE_HPP_ */
