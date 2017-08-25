/**
 * resp_parse_test.cpp
 *
 *  Created on: 23 авг. 2015 г.
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <grammar/grammar_parse_test.hpp>
#include <pushkin/http/common/grammar/response_parse.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

namespace http = psst::http;
namespace parse = http::grammar::parse;
GRAMMAR_TEST( parse::response_grammar, Response,
	::testing::Values(
		"HTTP/1.1 200 OK\r\n\r\n",
		"HTTP/1.1 404 Not found\r\n"
		"Server: BlaBla server\r\n"
		"\r\n",
		"HTTP/1.1 302 Found\r\n"
		"Cache-Control: private\r\n"
		"Content-Type: text/html; charset=UTF-8\r\n"
		"Location: http://www.google.ru/?gfe_rd=cr&ei=PNnaVZ7BMOOv8wfAy4fYAg\r\n"
		"Content-Length: 258\r\n"
		"Date: Mon, 24 Aug 2015 08:43:40 GMT\r\n"
		"Server: GFE/2.0\r\n\r\n"
	),
	::testing::Values(
		"HTTP 200 OK\r\n\r\n",
		"200 OK\r\n\r\n"
	)
);

GRAMMAR_PARSE_TEST( parse::response_grammar, Response, http::response,
	::testing::Values(
		ParseResponse::make_test_data(
				"HTTP/1.1 404 Not found\r\n"
				"Server: BlaBla server\r\n"
				"\r\n",
				{
					{1, 1},
					http::response_status::not_found,
					"Not found",
					{
						{ http::Server, "BlaBla server" }
					}
				}
		)
	)
);

#pragma GCC diagnostic pop

