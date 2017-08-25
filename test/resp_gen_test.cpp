/*
 * resp_gen_test.cpp
 *
 *  Created on: Aug 25, 2015
 *      Author: zmij
 */


#include <gtest/gtest.h>
#include <pushkin/http/common/grammar/response_generate.hpp>
#include "grammar/grammar_gen_test.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

namespace http = psst::http;
namespace gen = http::grammar::gen;
namespace iri_gen = tip::iri::grammar::gen;

class ResponseConstructTest : public ::testing::TestWithParam< ::std::pair< http::response, http::response_status > > {
public:
	typedef ::std::pair< http::response, http::response_status > param_type;
	static param_type
	make_test_data(http::response const& resp, http::response_status status)
	{
		return std::make_pair(resp, status);
	}
};

TEST_P(ResponseConstructTest, TestConstruct)
{
	param_type p = GetParam();
	EXPECT_EQ(p.first.status, p.second);
}

INSTANTIATE_TEST_CASE_P(ResponseTest, ResponseConstructTest,
	::testing::Values(
		ResponseConstructTest::make_test_data(
				{ http::response::version_type{1, 1}, http::response_status::ok, },
				http::response_status::ok
		),
		ResponseConstructTest::make_test_data(
				{ http::response::version_type{1, 1}, http::response_status::internal_server_error, },
				http::response_status::internal_server_error
		)
));

GRAMMAR_GEN_TEST(gen::response_status_grammar, ResponseStatus, http::response_status,
	::testing::Values(
		GenerateResponseStatus::make_test_data(http::response_status::continue_, "100"),
		GenerateResponseStatus::make_test_data(http::response_status::ok, "200"),
		GenerateResponseStatus::make_test_data(http::response_status::moved_permanently, "301"),
		GenerateResponseStatus::make_test_data(http::response_status::bad_request, "400"),
		GenerateResponseStatus::make_test_data(http::response_status::not_found, "404"),
		GenerateResponseStatus::make_test_data(http::response_status::method_not_allowed, "405"),
		GenerateResponseStatus::make_test_data(http::response_status::internal_server_error, "500")
	)
);

GRAMMAR_GEN_TEST(gen::response_grammar, Response, http::response,
	::testing::Values(
		GenerateResponse::make_test_data(
			{
				http::response::version_type{1, 1},
				http::response_status::ok,
				"OK",
				http::headers{
					{ http::Server, "tip-http-server" }
				}
			},
			"HTTP/1.1 200 OK\r\n"
			"Server: tip-http-server\r\n"
		),
		GenerateResponse::make_test_data(
			{
				http::response::version_type{1, 1},
				http::response_status::not_found,
				"Not Found",
				http::headers{
					{ http::Server, "tip-http-server" }
				}
			},
			"HTTP/1.1 404 Not Found\r\n"
			"Server: tip-http-server\r\n"
		)
	)
);

#pragma GCC diagnostic pop

