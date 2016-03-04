/*
 * cookie_gen_test.cpp
 *
 *  Created on: Aug 27, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <grammar/grammar_gen_test.hpp>
#include <tip/http/common/grammar/cookie_generate.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

namespace http = tip::http;
namespace gen = http::grammar::gen;
namespace pt = boost::posix_time;

GRAMMAR_GEN_TEST(gen::cookie_grammar, Cookie, http::cookie,
	::testing::Values(
		GenerateCookie::make_test_data(
			{ "foo", "bar" },
			"foo=bar"
		),
		GenerateCookie::make_test_data(
			{ "SID", "31d4d96e407aad42" },
			"SID=31d4d96e407aad42"
		)
	)
);

GRAMMAR_GEN_TEST(gen::set_cookie_grammar, SetCookie, http::cookie,
	::testing::Values(
		GenerateCookie::make_test_data(
			{ "foo", "bar" },
			"foo=bar"
		),
		GenerateCookie::make_test_data(
			{ "SID", "31d4d96e407aad42" },
			"SID=31d4d96e407aad42"
		),
		GenerateCookie::make_test_data(
			{
				"SID", "31d4d96e407aad42",
				pt::ptime(
					boost::gregorian::date( 2015, boost::date_time::Aug, 27 ),
							pt::hours(17) + pt::minutes(45)
				)
			},
			"SID=31d4d96e407aad42; Expires=Thu, 27 Aug 2015 17:45:00 GMT"
		),
		GenerateCookie::make_test_data(
			{
				"SID", "31d4d96e407aad42",
				{}, 100500
			},
			"SID=31d4d96e407aad42; Max-Age=100500"
		),
		GenerateCookie::make_test_data(
			{
				"SID", "31d4d96e407aad42",
				{}, 100500,
				tip::iri::host{ "example.com" },
				tip::iri::path{ true },
				true
			},
			"SID=31d4d96e407aad42; Max-Age=100500; Domain=example.com; Path=/; Secure"
		)
	)
);

#pragma GCC diagnostic pop
