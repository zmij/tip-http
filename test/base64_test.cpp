/*
 * base64_tests.cpp
 *
 *  Created on: Aug 20, 2015
 *      Author: zmij
 */

#include <tip/http/common/grammar/base64_generate.hpp>
#include <tip/http/common/grammar/base64_parse.hpp>
#include "grammar/grammar_gen_test.hpp"
#include "grammar/grammar_parse_test.hpp"

namespace gen = tip::http::grammar::gen;
namespace parse = tip::http::grammar::parse;

GRAMMAR_GEN_TEST(gen::base64_grammar, Base64, std::string,
	::testing::Values(
		GenerateBase64::make_test_data( "asdgqerqlwkzxcoi", "YXNkZ3FlcnFsd2t6eGNvaQ==" ),
		GenerateBase64::make_test_data(
			"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
			"eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
			"enim ad minim veniam, quis nostrud exercitation ullamco laboris "
			"nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
			"reprehenderit in voluptate velit esse cillum dolore eu fugiat "
			"nulla pariatur. Excepteur sint occaecat cupidatat non proident, "
			"sunt in culpa qui officia deserunt mollit anim id est laborum.",
			"TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdCwg"
			"c2VkIGRvIGVpdXNtb2QgdGVtcG9yIGluY2lkaWR1bnQgdXQgbGFib3JlIGV0IGRvbG9yZSBtYWdu"
			"YSBhbGlxdWEuIFV0IGVuaW0gYWQgbWluaW0gdmVuaWFtLCBxdWlzIG5vc3RydWQgZXhlcmNpdGF0"
			"aW9uIHVsbGFtY28gbGFib3JpcyBuaXNpIHV0IGFsaXF1aXAgZXggZWEgY29tbW9kbyBjb25zZXF1"
			"YXQuIER1aXMgYXV0ZSBpcnVyZSBkb2xvciBpbiByZXByZWhlbmRlcml0IGluIHZvbHVwdGF0ZSB2"
			"ZWxpdCBlc3NlIGNpbGx1bSBkb2xvcmUgZXUgZnVnaWF0IG51bGxhIHBhcmlhdHVyLiBFeGNlcHRl"
			"dXIgc2ludCBvY2NhZWNhdCBjdXBpZGF0YXQgbm9uIHByb2lkZW50LCBzdW50IGluIGN1bHBhIHF1"
			"aSBvZmZpY2lhIGRlc2VydW50IG1vbGxpdCBhbmltIGlkIGVzdCBsYWJvcnVtLg=="
		)
	)
);

GRAMMAR_PARSE_TEST(parse::base64_grammar, Base64, std::string,
	::testing::Values(
		ParseBase64::make_test_data( "YXNkZ3FlcnFsd2t6eGNvaQ==", "asdgqerqlwkzxcoi" ),
		ParseBase64::make_test_data(
			"TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdCwg\n"
			"c2VkIGRvIGVpdXNtb2QgdGVtcG9yIGluY2lkaWR1bnQgdXQgbGFib3JlIGV0IGRvbG9yZSBtYWdu\n"
			"YSBhbGlxdWEuIFV0IGVuaW0gYWQgbWluaW0gdmVuaWFtLCBxdWlzIG5vc3RydWQgZXhlcmNpdGF0\n"
			"aW9uIHVsbGFtY28gbGFib3JpcyBuaXNpIHV0IGFsaXF1aXAgZXggZWEgY29tbW9kbyBjb25zZXF1\n"
			"YXQuIER1aXMgYXV0ZSBpcnVyZSBkb2xvciBpbiByZXByZWhlbmRlcml0IGluIHZvbHVwdGF0ZSB2\n"
			"ZWxpdCBlc3NlIGNpbGx1bSBkb2xvcmUgZXUgZnVnaWF0IG51bGxhIHBhcmlhdHVyLiBFeGNlcHRl\n"
			"dXIgc2ludCBvY2NhZWNhdCBjdXBpZGF0YXQgbm9uIHByb2lkZW50LCBzdW50IGluIGN1bHBhIHF1\n"
			"aSBvZmZpY2lhIGRlc2VydW50IG1vbGxpdCBhbmltIGlkIGVzdCBsYWJvcnVtLg==",
			"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
			"eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
			"enim ad minim veniam, quis nostrud exercitation ullamco laboris "
			"nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
			"reprehenderit in voluptate velit esse cillum dolore eu fugiat "
			"nulla pariatur. Excepteur sint occaecat cupidatat non proident, "
			"sunt in culpa qui officia deserunt mollit anim id est laborum."
		),
		ParseBase64::make_test_data(
			"TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdCwg"
			"c2VkIGRvIGVpdXNtb2QgdGVtcG9yIGluY2lkaWR1bnQgdXQgbGFib3JlIGV0IGRvbG9yZSBtYWdu"
			"YSBhbGlxdWEuIFV0IGVuaW0gYWQgbWluaW0gdmVuaWFtLCBxdWlzIG5vc3RydWQgZXhlcmNpdGF0"
			"aW9uIHVsbGFtY28gbGFib3JpcyBuaXNpIHV0IGFsaXF1aXAgZXggZWEgY29tbW9kbyBjb25zZXF1"
			"YXQuIER1aXMgYXV0ZSBpcnVyZSBkb2xvciBpbiByZXByZWhlbmRlcml0IGluIHZvbHVwdGF0ZSB2"
			"ZWxpdCBlc3NlIGNpbGx1bSBkb2xvcmUgZXUgZnVnaWF0IG51bGxhIHBhcmlhdHVyLiBFeGNlcHRl"
			"dXIgc2ludCBvY2NhZWNhdCBjdXBpZGF0YXQgbm9uIHByb2lkZW50LCBzdW50IGluIGN1bHBhIHF1"
			"aSBvZmZpY2lhIGRlc2VydW50IG1vbGxpdCBhbmltIGlkIGVzdCBsYWJvcnVtLg==",
			"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
			"eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
			"enim ad minim veniam, quis nostrud exercitation ullamco laboris "
			"nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
			"reprehenderit in voluptate velit esse cillum dolore eu fugiat "
			"nulla pariatur. Excepteur sint occaecat cupidatat non proident, "
			"sunt in culpa qui officia deserunt mollit anim id est laborum."
		)
	)
);
