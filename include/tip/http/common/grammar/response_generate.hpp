/*
 * response_generate.hpp
 *
 *  Created on: Aug 25, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_RESPONSE_GENERATE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_RESPONSE_GENERATE_HPP_

#include <boost/spirit/include/support_adapt_adt_attributes.hpp>
#include <tip/http/common/grammar/header_generate.hpp>
#include <tip/iri/grammar/iri_generate.hpp>
#include <tip/http/common/response.hpp>

namespace tip {
namespace http {
namespace grammar {
namespace gen {

template < typename OutputIterator >
struct response_status_grammar :
		boost::spirit::karma::grammar< OutputIterator, response_status()> {
	typedef ::std::underlying_type< response_status >::type integral_type;
	response_status_grammar() : response_status_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		namespace phx = boost::phoenix;
		using karma::_val;
		using karma::_1;
		root = karma::int_[ _1 = phx::static_cast_< integral_type >(_val) ];
	}
	boost::spirit::karma::rule< OutputIterator, response_status()> root;
};

template < typename OutputIterator >
struct response_grammar :
		boost::spirit::karma::grammar< OutputIterator, response()> {
	response_grammar() : response_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		namespace phx = boost::phoenix;
		using karma::_val;
		using karma::_1;
		using karma::_2;
		using karma::_3;
		using karma::_4;

		version = "HTTP/" << karma::int_ << "." << karma::int_;
		crlf = karma::lit("\r\n");

		root = (version << ' ' << status << ' ' << karma::string << crlf
				<< headers)
			[
			 	 _1 = phx::bind(&response::version, _val),
				 _2 = phx::bind(&response::status, _val),
				 _3 = phx::bind(&response::status_line, _val),
				 _4 = phx::bind(&response::headers_, _val)
			];
	}
	boost::spirit::karma::rule< OutputIterator, response()> root;
	boost::spirit::karma::rule< OutputIterator, response::version_type()> version;
	boost::spirit::karma::rule< OutputIterator> crlf;
	response_status_grammar< OutputIterator >	status;
	headers_grammar< OutputIterator > headers;
};

template < typename OutputIterator >
struct stock_response_grammar :
		boost::spirit::karma::grammar< OutputIterator, response()> {
	typedef response value_type;
	stock_response_grammar() : stock_response_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		namespace phx = boost::phoenix;
		using karma::_val;
		using karma::_1;
		using karma::_2;
		using karma::_3;

		root =
		(
			"<html>"
			"<head><title>" << karma::string << "</title></head>"
			"<body><h1>" << status << " " << karma::string << "</h1></body>"
			"</html>"
		)[
		  _1 = (phx::bind(&response::status_line, _val)),
		  _2 = (phx::bind(&response::status, _val)),
		  _3 = (phx::bind(&response::status_line, _val))
		];
	}
	boost::spirit::karma::rule< OutputIterator, value_type()>	root;
	response_status_grammar< OutputIterator >					status;
};


}  // namespace gen
}  // namespace grammar
}  // namespace http
}  // namespace tip

#endif /* TIP_HTTP_COMMON_GRAMMAR_RESPONSE_GENERATE_HPP_ */
