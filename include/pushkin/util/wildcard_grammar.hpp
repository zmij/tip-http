/*
 * wildcard_grammar.hpp
 *
 *  Created on: Aug 29, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_UTIL_WILDCARD_GRAMMAR_HPP_
#define PUSHKIN_HTTP_UTIL_WILDCARD_GRAMMAR_HPP_

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <pushkin/util/wildcard.hpp>

namespace psst {
namespace util {
namespace grammar {
namespace parse {

template < typename InputIterator, typename T >
struct wildcard_grammar :
		boost::spirit::qi::grammar< InputIterator, wildcard<T>()> {
	typedef wildcard<T> value_type;
	wildcard_grammar() : wildcard_grammar::base_type(root)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_val;
		using qi::_1;
		named_wildchar %= ':' >> (qi::alpha | qi::char_("_")) >>
				*(qi::alnum | qi::char_("_"))>> ':';
		root =  qi::char_('?')	[ _val = phx::construct<value_type>(false) ] |
				qi::char_('*')	[ _val = phx::construct<value_type>(true)  ] |
				named_wildchar	[ _val = phx::construct<value_type>(_1)    ];
	}
	boost::spirit::qi::rule< InputIterator, value_type()> root;
	boost::spirit::qi::rule< InputIterator, std::string()> named_wildchar;
};

}  // namespace parse
}  // namespace grammar
}  // namespace util
}  // namespace psst



#endif /* PUSHKIN_HTTP_UTIL_WILDCARD_GRAMMAR_HPP_ */
