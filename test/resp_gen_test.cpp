/*
 * resp_gen_test.cpp
 *
 *  Created on: Aug 25, 2015
 *      Author: zmij
 */


#include <tip/http/common/grammar/response_generate.hpp>
#include "grammar/grammar_gen_test.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

namespace gen = tip::http::grammar::gen;
namespace iri_gen = tip::iri::grammar::gen;
namespace http = tip::http;

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
		)
	)
);

#pragma GCC diagnostic pop

