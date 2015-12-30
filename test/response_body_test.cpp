/*
 * response_body_test.cpp
 *
 *  Created on: Aug 25, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <tip/http/common/response.hpp>
#include <string>
#include <sstream>

class ChunkedResponseTest :
		public ::testing::TestWithParam< std::vector< std::string > > {
};

TEST_P(ChunkedResponseTest, ReadChunked)
{
	namespace http = tip::http;
	ParamType param = GetParam();
	http::response r;
	ParamType::iterator b = param.begin();
	http::response::read_callback cb = [&r](std::istream& is) {
		return r.read_body(is);
	};
	while (b != param.end()) {
		std::istringstream is(*b);
		if (b == param.begin()) {
			EXPECT_TRUE(r.read_headers(is));
		}
		http::response::read_result_type res = cb(is);
		cb = res.callback;
		if (b + 1 == param.end()) {
			// last buffer
			EXPECT_TRUE(res.result);
		} else {
			EXPECT_TRUE(boost::indeterminate(res.result));
		}
		++b;
	}
}
INSTANTIATE_TEST_CASE_P(ResponseTest, ChunkedResponseTest,
	::testing::Values(
		std::vector<std::string>{
			"HTTP/1.1 200 OK\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"b\r\n"
			"Lorem ipsum\r\n"
			"10\r\n"
			"  dolor sit amet\r\n"
			"0\r\n"
			"\r\n"
		},
		std::vector<std::string>{
			"HTTP/1.1 200 OK\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"b\r\n"
			"Lorem ipsum\r\n",
			"10\r\n"
			"  dolor sit amet\r\n"
			"0\r\n"
			"\r\n"
		}
	)
);

TEST(Response, ReadChunked)
{
	namespace http = tip::http;
	std::string response_str =
		"HTTP/1.1 200 OK\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"b\r\n"
		"Lorem ipsum\r\n"
		"10\r\n"
		"  dolor sit amet\r\n"
		"0\r\n"
		"\r\n"
	;
	std::istringstream is(response_str);
	http::response r;

	ASSERT_TRUE(r.read_headers(is));
	EXPECT_EQ(true, r.read_body(is).result);
}
