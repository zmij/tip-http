/*
 * wildcard_test.cpp
 *
 *  Created on: Aug 29, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <pushkin/util/wildcard_grammar.hpp>
#include <grammar/grammar_parse_test.hpp>

template < typename InputIterator >
using wildcard_grammar =
		psst::util::grammar::parse::wildcard_grammar<InputIterator, std::string >;

GRAMMAR_TEST(wildcard_grammar, Wildcard,
	::testing::Values(
		"*", "?", ":param:"
	),
	::testing::Values(
		"foo", "bar", "::", ":param/pam/pam:"
	)
);
typedef psst::util::wildcard< std::string > wildcard;
GRAMMAR_PARSE_TEST(wildcard_grammar, Wildcard, psst::util::wildcard< std::string >,
	::testing::Values(
		ParseWildcard::make_test_data( "*", wildcard{ true } ),
		ParseWildcard::make_test_data( "?", wildcard{ false } ),
		ParseWildcard::make_test_data( ":param:", wildcard{ std::string("param") } )
	)
);
