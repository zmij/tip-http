/*
 * cookie_parse_test.cpp
 *
 *  Created on: Aug 26, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <grammar/grammar_parse_test.hpp>
#include <tip/http/common/grammar/cookie_parse.hpp>

namespace http = tip::http;
namespace parse = http::grammar::parse;

GRAMMAR_TEST(parse::ctl_grammar, Ctl,
	::testing::Values("\x01", "\x1f", "\x7f"),
	::testing::Values("a", "0", "~")
);

GRAMMAR_TEST(parse::cookie_grammar, Cookie,
	::testing::Values(
		"SID=31d4d96e407aad42",
		"lang=en-US"
	),
	::testing::Values(
		"", "1adf", "(bla)=adsfa"
	)
);

GRAMMAR_TEST(parse::cookies_grammar, Cookies,
	::testing::Values(
		"SID=31d4d96e407aad42; lang=en-US",
		"foo=bar"
	),
	::testing::Values(
		"", "asdf"
	)
);

GRAMMAR_TEST(parse::set_cookie_grammar, SetCookie,
	::testing::Values(
		"SID=31d4d96e407aad42",
		"foo=bar"
		"SID=31d4d96e407aad42; Path=/; Domain=example.com",
		"SID=31d4d96e407aad42; Path=/; Secure; HttpOnly",
		"SID=31d4d96e407aad42; Domain=example.com; Secure; HttpOnly",
		"lang=en-US; Path=/; Domain=example.com",
		"lang=en-US; Domain=example.com; Path=/",
		"lang=en-US; Domain=example.com; Expires=Wed, 09 Jun 2021 10:18:14 GMT"
	),
	::testing::Values(
		"", "1adf", "(bla)=adsfa"
	)
);
