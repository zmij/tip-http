/*
 * req_gen_test.cpp
 *
 *  Created on: Aug 19, 2015
 *      Author: zmij
 */

#include <pushkin/http/common/grammar/request_generate.hpp>
#include "grammar/grammar_gen_test.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

namespace http = ::psst::http;
namespace gen = http::grammar::gen;
namespace iri_gen = ::tip::iri::grammar::gen;

GRAMMAR_GEN_TEST(gen::header_name_grammar, HeaderName, http::header_name,
	::testing::Values(
        GenerateHeaderName::make_test_data( http::Accept, "Accept" ),
        GenerateHeaderName::make_test_data( http::UserAgent, "User-Agent" ),
		GenerateHeaderName::make_test_data( "X-Custom-Header", "X-Custom-Header" )
	)
);

GRAMMAR_GEN_TEST(gen::header_grammar, Header, http::header,
	::testing::Values(
		GenerateHeader::make_test_data(
            { psst::http::Accept, "text/plain" },
			"Accept: text/plain\r\n"
		),
		GenerateHeader::make_test_data(
            {psst::http::ContentLength, "1348"},
			"Content-Length: 1348\r\n"
		)
	)
);

GRAMMAR_GEN_TEST(gen::headers_grammar, Headers, ::psst::http::headers,
	::testing::Values(
		GenerateHeaders::make_test_data(
			{
                { psst::http::Accept, "text/plain" },
                { psst::http::ContentLength, "100500" },
				{ "X-Foo-Bar", "FooBar" }
			},
			"Accept: text/plain\r\n"
			"Content-Length: 100500\r\n"
			"X-Foo-Bar: FooBar\r\n"
		)
	)
);

TEST(GenerateTest, Tmp)
{
	namespace karma = boost::spirit::karma;
	typedef std::ostream_iterator<char> sink_type;
	std::ostringstream os;
	sink_type out(os);
	iri_gen::unreserved_grammar<sink_type> unreserved;
	iri_gen::pct_encoded_grammar<sink_type> pct_encoded;
	karma::rule< sink_type, std::string() > query_char =
			+(unreserved | karma::char_("!$'()*+,;:@/?") | pct_encoded);
	//karma::rule< sink_type, std::string() > query_name = +query_char;

	EXPECT_TRUE(karma::generate(out, query_char, std::string("fuu bar blabla~bla?@@+;:")));
	EXPECT_EQ("fuu%20bar%20blabla~bla?@@+;:", os.str());
}

GRAMMAR_GEN_TEST(gen::query_param_grammar, QueryParam, http::request::query_param_type,
	::testing::Values(
		GenerateQueryParam::make_test_data( { "foo", "bar" }, "foo=bar"),
		GenerateQueryParam::make_test_data( { "foo baz", "bar~bar" }, "foo%20baz=bar~bar")
	)
);

GRAMMAR_GEN_TEST(gen::query_grammar, Query, http::request::query_type,
	::testing::Values(
		GenerateQuery::make_test_data(
			{{"foo", "bar"}, {"page", "100500"}},
			"foo=bar&page=100500"
		),
		GenerateQuery::make_test_data( {}, "" )
	)
);

GRAMMAR_GEN_TEST(gen::request_grammar, Request, http::request,
	::testing::Values(
		GenerateRequest::make_test_data(
            http::request(
				http::GET,
				tip::iri::path{ true, { "api" } },
				http::request::query_type{ {"foo", "bar"} },
				tip::iri::fragment{ "anchor" },
				http::headers{
					{ http::Host, "facebook.com" }
				}
            ),
			"GET /api?foo=bar#anchor HTTP/1.1\r\n"
			"Host: facebook.com\r\n"
		),
		GenerateRequest::make_test_data(
            http::request(
				http::GET,
				tip::iri::path{ true, {} },
				http::request::query_type{},
				tip::iri::fragment{},
				http::headers{
					{ http::Host, "mail.ru" }
				}
            ),
			"GET / HTTP/1.1\r\n"
			"Host: mail.ru\r\n"
		)
	)
);

#pragma GCC diagnostic pop
