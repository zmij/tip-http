/*
 * pathmatcher_parse.hpp
 *
 *  Created on: Aug 29, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_SERVER_DETAIL_PATHMATCHER_PARSE_HPP_
#define PUSHKIN_HTTP_SERVER_DETAIL_PATHMATCHER_PARSE_HPP_

#include <tip/iri/grammar/iri_parse.hpp>
#include <pushkin/util/wildcard_grammar.hpp>
#include <pushkin/http/server/detail/path_matcher.hpp>

namespace psst {
namespace http {
namespace server {
namespace grammar {
namespace parse {

template < typename InputIterator >
struct match_path_segment_grammar :
		boost::spirit::qi::grammar< InputIterator, detail::path_segment()> {
	typedef detail::path_segment value_type;
	match_path_segment_grammar() : match_path_segment_grammar::base_type(root)
	{
		namespace qi = boost::spirit::qi;
		root = wildcard | literal_segment;
	}
	boost::spirit::qi::rule< InputIterator, value_type()> root;
	util::grammar::parse::wildcard_grammar< InputIterator, std::string > wildcard;
	::tip::iri::grammar::parse::isegment_nz_grammar< InputIterator > literal_segment;
};

template < typename InputIterator >
struct match_path_sequence_grammar :
		boost::spirit::qi::grammar< InputIterator, detail::path_match_sequence()> {
	typedef detail::path_match_sequence value_type;
	match_path_sequence_grammar() : match_path_sequence_grammar::base_type(root)
	{
		namespace qi = boost::spirit::qi;
		root %= '/' >> segment >> *('/' >> segment);
	}
	boost::spirit::qi::rule< InputIterator, value_type()> root;
	match_path_segment_grammar< InputIterator > segment;
};

}  // namespace parse
}  // namespace grammar
}  // namespace server
}  // namespace http
}  // namespace psst

#endif /* PUSHKIN_HTTP_SERVER_DETAIL_PATHMATCHER_PARSE_HPP_ */
