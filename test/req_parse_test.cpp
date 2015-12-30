/*
 * req_grammar_test.cpp
 *
 *  Created on: Aug 18, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <grammar/grammar_parse_test.hpp>
#include <tip/http/common/grammar/request_parse.hpp>

namespace http = tip::http;
namespace parse = http::grammar::parse;

GRAMMAR_TEST(parse::header_name_grammar, HeaderName,
	::testing::Values(
		"Accept",
		"Sec-WebSocket-Accept",
		"User-Agent",
		"Upgrade-Insecure-Requests"
	),
	::testing::Values("", "~InvalidHeader")
);

GRAMMAR_PARSE_TEST(parse::header_name_grammar, HeaderName, http::header_name,
	::testing::Values(
		ParseHeaderName::make_test_data( "Accept", http::Accept ),
		ParseHeaderName::make_test_data( "Content-Length", http::ContentLength ),
		ParseHeaderName::make_test_data( "If-Modified-Since", http::IfModifiedSince ),
		ParseHeaderName::make_test_data( "Last-Modified", http::LastModified ),
		ParseHeaderName::make_test_data( "X-Custom-Header", "X-Custom-Header" )
	)
);

GRAMMAR_TEST(parse::header_grammar, HeaderKeyValue,
	::testing::Values(
		"Accept: text/plain\r\n",
		"Allow: OPTIONS, GET, HEAD\r\n",
		"Content-Language: en, ase, ru\r\n",
		"Content-Length: 1348\r\n",
		"User-Agent: Mozilla/5.0 (X11; Linux i686; rv:2.0.1) Gecko/20100101 Firefox/4.0.1\r\n",
		"Host: localhost:8080\r\n",
		"Connection: keep-alive\r\n",
		"Cache-Control: max-age=0\r\n",
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n",
		"Upgrade-Insecure-Requests: 1\r\n",
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/44.0.2403.157 Safari/537.36\r\n",
		"Accept-Encoding: gzip, deflate, sdch\r\n",
		"Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n"
	),
	::testing::Values("", "~InvalidHeader", "IncompleteHeader: value")
);

GRAMMAR_PARSE_TEST(parse::header_grammar, Header, http::header,
	::testing::Values(
		ParseHeader::make_test_data("Accept: text/plain\r\n",
				{tip::http::Accept, "text/plain"}),
		ParseHeader::make_test_data("Content-Length: 1348\r\n",
				{tip::http::ContentLength, "1348"}),
		ParseHeader::make_test_data("X-Custom-Header: custom value\r\n",
				{"X-Custom-Header", "custom value"})
	)
);

TEST(Parse, HeaderStreamParse)
{
	namespace spirit = boost::spirit;
	namespace qi = boost::spirit::qi;

	typedef std::istreambuf_iterator<char> istreambuf_iterator;
	typedef spirit::multi_pass< istreambuf_iterator > multi_pass_iterator;

	typedef boost::spirit::istream_iterator stream_iterator;
	typedef parse::header_grammar< multi_pass_iterator > stream_header_grammar;

	std::istringstream is("Accept: text/plain\r\n");

	multi_pass_iterator f = multi_pass_iterator(istreambuf_iterator(is));
	multi_pass_iterator l = multi_pass_iterator(istreambuf_iterator());

	http::header res;
	http::header expected{ http::Accept, "text/plain" };
	EXPECT_TRUE(qi::parse(f, l, stream_header_grammar(), res));
	EXPECT_EQ(expected, res);
}

GRAMMAR_TEST(parse::headers_grammar, Headers,
	::testing::Values(
			"Accept: text/plain\r\n",
			"Allow: OPTIONS, GET, HEAD\r\n",
			"Content-Language: en, ase, ru\r\n",
			"Content-Length: 1348\r\n",
			"Accept: text/plain\r\n"
			"Allow: OPTIONS, GET, HEAD\r\n"
			"Content-Language: en, ase, ru\r\n"
			"Content-Length: 1348\r\n"
	),
	::testing::Values("", ": invalid\r\n")
);

GRAMMAR_PARSE_TEST(parse::headers_grammar, Headers, http::headers,
	::testing::Values(
		ParseHeaders::make_test_data(
			"Accept: text/plain\r\n"
			"Allow: OPTIONS, GET, HEAD\r\n"
			"Content-Language: en, ase, ru\r\n"
			"Content-Length: 1348\r\n",
			{
				{ tip::http::Accept, "text/plain" },
				{ tip::http::Allow, "OPTIONS, GET, HEAD" },
				{ tip::http::ContentLanguage, "en, ase, ru" },
				{ tip::http::ContentLength, "1348" }
			}
		)
	)
);

GRAMMAR_TEST(parse::query_param_grammar, QueryParam,
	::testing::Values(
		"a=b", "blabla=Foo"
	),
	::testing::Values(
		"", "&&&&"
	)
);

GRAMMAR_PARSE_TEST(parse::query_param_grammar, QueryParam,
		http::request::query_param_type,
	::testing::Values(
		ParseQueryParam::make_test_data("a=b", {"a", "b"}),
		ParseQueryParam::make_test_data("q=b%20Foo", {"q", "b Foo"})
	)
);

GRAMMAR_TEST(parse::query_grammar, Query,
	::testing::Values(
		"a=b&blabla=Foo", "t=%20foo&bar=baz"
	),
	::testing::Values(
		"", "&&&&"
	)
);

GRAMMAR_PARSE_TEST(parse::query_grammar, Query, http::request::query_type,
	::testing::Values(
		ParseQuery::make_test_data("a=b", {{"a", "b"}}),
		ParseQuery::make_test_data(
			"q=b%20Foo&foo=bar",
			{
				{"q", "b Foo"},
				{"foo", "bar"}
			})
	)
);

GRAMMAR_TEST(parse::request_grammar, Request,
	::testing::Values(
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n\r\n",
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Allow: OPTIONS, GET, HEAD\r\n"
		"Content-Language: en, ase, ru\r\n"
		"Content-Length: 1348\r\n"
		"\r\n",
		"GET / HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Connection: keep-alive\r\n"
		"Cache-Control: max-age=0\r\n"
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
		"Upgrade-Insecure-Requests: 1\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/44.0.2403.157 Safari/537.36\r\n"
		"Accept-Encoding: gzip, deflate, sdch\r\n"
		"Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n"
		"\r\n"
	),
	::testing::Values(
		"",
		"GET HTTP/1.1\r\n",
		"\r\n"
	)
);
