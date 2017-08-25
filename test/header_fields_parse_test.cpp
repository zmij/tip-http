/*
 * header_fields_parse_test.cpp
 *
 *  Created on: Aug 28, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <grammar/grammar_parse_test.hpp>
#include <pushkin/http/common/grammar/header_fields_parse.hpp>

namespace http = psst::http;
namespace parse = http::grammar::parse;

GRAMMAR_TEST(parse::accept_language_grammar, AcceptLanguage,
	::testing::Values(
		"ru", "en-GB", "en;q=0.7"
	),
	::testing::Values(
		"", "1ab", "ru;f=8"
	)
);

GRAMMAR_PARSE_TEST(parse::accept_language_grammar, AcceptLanguage, http::accept_language,
	::testing::Values(
		ParseAcceptLanguage::make_test_data( "ru", { "ru", 1.0f } ),
		ParseAcceptLanguage::make_test_data( "en-GB", { "en-GB", 1.0f } ),
		ParseAcceptLanguage::make_test_data( "en;q=0.7", { "en", 0.7f } )
	)
);

GRAMMAR_TEST(parse::accept_languages_grammar, AcceptLanguages,
	::testing::Values(
		"ru, fr;q=0.9, en-GB;q=0.7, en;q=0.4",
		"de-de, fr;q=0.9, en-GB;q=0.7, en-us;q=0.6, en;q=0.4"
	),
	::testing::Values(
		"", "ru;fr"
	)
);

GRAMMAR_PARSE_TEST(parse::accept_languages_grammar, AcceptLanguages,
		http::accept_languages,
	::testing::Values(
		ParseAcceptLanguages::make_test_data(
			"ru, fr;q=0.9, en-GB;q=0.7, en;q=0.4",
			{
				{ "ru", 1.0f },
				{ "fr", 0.9f },
				{ "en-GB", 0.7f },
				{ "en", 0.4f }
			}
		),
		ParseAcceptLanguages::make_test_data(
			"de-de, fr;q=0.9, en-GB;q=0.7, en-us;q=0.6, en;q=0.4",
			{
				{ "de-de", 1.0f },
				{ "fr", 0.9f },
				{ "en-GB", 0.7f },
				{ "en-us", 0.6f },
				{ "en", 0.4f }
			}
		)
	)
);
