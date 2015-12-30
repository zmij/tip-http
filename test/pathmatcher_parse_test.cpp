/*
 * pathmatcher_parse_test.cpp
 *
 *  Created on: Aug 29, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <grammar/grammar_parse_test.hpp>
#include <tip/http/server/detail/pathmatcher_parse.hpp>

namespace parse = tip::http::server::grammar::parse;
namespace http = tip::http::server::detail;
typedef tip::util::wildcard< std::string > string_wildcard;

GRAMMAR_TEST(parse::match_path_segment_grammar, PathSegment,
	::testing::Values(
		"*", "?", ":param:", "foo", "bar", "foo-bar"
	),
	::testing::Values(
		"", "/"
	)
);

GRAMMAR_PARSE_TEST(parse::match_path_segment_grammar, PathSegment,
	http::path_segment,
	::testing::Values(
		ParsePathSegment::make_test_data("*",
				{ string_wildcard{ true } }),
		ParsePathSegment::make_test_data("?",
				{ string_wildcard{ false } }),
		ParsePathSegment::make_test_data(":param:",
				{ string_wildcard{ std::string("param") } }),
		ParsePathSegment::make_test_data("foo:bar",
				{ std::string("foo:bar") })
	)
);

GRAMMAR_TEST(parse::match_path_sequence_grammar, PathSequence,
	::testing::Values(
		"/api/register/*", "/api/register/?", "/api/register/:type:",
		"/:one:/:two:/:three:/here_we_go"
	),
	::testing::Values(
		"", "/", " ", "/  /"
	)
);

GRAMMAR_PARSE_TEST(parse::match_path_sequence_grammar, PathSequence,
	http::path_match_sequence,
	::testing::Values(
		ParsePathSequence::make_test_data(
			"/api/register/*",
			{
				{ std::string("api") },
				{ std::string("register") },
				{ string_wildcard{ true } }
			}
		),
		ParsePathSequence::make_test_data(
			"/api/register/?",
			{
				{ std::string("api") },
				{ std::string("register") },
				{ string_wildcard{ false } }
			}
		),
		ParsePathSequence::make_test_data(
			"/api/register/:param:",
			{
				{ std::string("api") },
				{ std::string("register") },
				{ string_wildcard{ std::string("param") } }
			}
		),
		ParsePathSequence::make_test_data(
			"/:one:/:two:/:three:/here_we_go",
			{
				{ string_wildcard{ std::string("one") } },
				{ string_wildcard{ std::string("two") } },
				{ string_wildcard{ std::string("three") } },
				{ std::string("here_we_go") }
			}
		)
	)
);
