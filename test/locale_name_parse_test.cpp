/**
 * locale_name_parse_test.cpp
 *
 *  Created on: 10 окт. 2015 г.
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <grammar/grammar_parse_test.hpp>
#include <grammar/grammar_gen_test.hpp>
#include <tip/http/server/grammar/locale_name.hpp>
#include <set>

namespace http = tip::http;
namespace parse = http::server::grammar::parse;
namespace gen = http::server::grammar::gen;

template <typename InputIterator>
using locale_names_grammar =
		parse::locale_names_grammar<
			InputIterator, std::set< http::server::locale_name > >;

GRAMMAR_TEST(parse::locale_name_grammar, LocaleName,
	::testing::Values(
		"en", "de", "fr-FR", "ru_RU.UTF-8"
	),
	::testing::Values(
		"", "-/bla"
	)
);
GRAMMAR_TEST(locale_names_grammar, LocaleNames,
	::testing::Values(
		"en;de;fr-FR;ru_RU.UTF-8",
		"en de fr-FR ru_RU.UTF-8"
	),
	::testing::Values(
		"", "-a"
	)
);
GRAMMAR_PARSE_TEST(parse::locale_name_grammar, LocaleName, http::server::locale_name,
	::testing::Values(
		ParseLocaleName::make_test_data( "ru", { "ru", "", "" } ),
		ParseLocaleName::make_test_data( "ru_RU", { "ru", "RU", "" } ),
		ParseLocaleName::make_test_data( "ru-RU", { "ru", "RU", "" } ),
		ParseLocaleName::make_test_data( "ru_RU.UTF-8", { "ru", "RU", "UTF-8" } )
	)
);

GRAMMAR_GEN_TEST(gen::locale_name_grammar, LocaleName, http::server::locale_name,
	::testing::Values(
		GenerateLocaleName::make_test_data( { "ru", "" }, "ru" ),
		GenerateLocaleName::make_test_data( { "ru", "RU" }, "ru_RU" ),
		GenerateLocaleName::make_test_data( { "ru", "RU", "UTF-8" }, "ru_RU.UTF-8" )
	)
);

TEST(LocaleName, Comparison)
{
	using http::server::locale_name;

	EXPECT_EQ((locale_name{ "ru" }), (locale_name{ "ru", "" }));
	EXPECT_EQ((locale_name{ "ru_RU" }), (locale_name{ "ru", "RU" }));
	EXPECT_EQ((locale_name{ "ru-RU" }), (locale_name{ "ru", "RU" }));
	EXPECT_EQ((locale_name{ "ru_RU.UTF-8" }), (locale_name{ "ru", "RU", "UTF-8" }));

	EXPECT_NE((locale_name{"ru"}), (locale_name{"ru_RU"}));
	EXPECT_NE((locale_name{"ru.UTF-8"}), (locale_name{"ru_RU"}));
}

TEST(LocaleName, CIComparison)
{
	using http::server::locale_name;

	EXPECT_EQ((locale_name{ "ru" }), (locale_name{ "RU", "" }));
	EXPECT_EQ((locale_name{ "ru_RU" }), (locale_name{ "RU", "ru" }));
	EXPECT_EQ((locale_name{ "ru_RU.UTF-8" }), (locale_name{ "ru", "ru", "utf-8" }));
}

TEST(LocaleName, IgnoreEncoding)
{
	using http::server::locale_name;

	EXPECT_EQ(0, locale_name{"ru"}.compare(locale_name{"ru.UTF-8"}, locale_name::ignore_encoding));
	EXPECT_EQ(0, locale_name{"ru_RU"}.compare(locale_name{"ru_RU.UTF-8"}, locale_name::ignore_encoding));
	EXPECT_EQ(0, locale_name{"ru_RU.CP-1251"}.compare(locale_name{"ru_RU.UTF-8"}, locale_name::ignore_encoding));
	EXPECT_NE(0, locale_name{"ru.UTF-8"}.compare(locale_name{"ru_RU.UTF-8"}, locale_name::ignore_encoding));
}

TEST(LocaleName, IgnoreCulture)
{
	using http::server::locale_name;

	EXPECT_EQ(0, locale_name{"ru"}.compare(locale_name{"ru_RU"}, locale_name::ignore_culture));
	EXPECT_EQ(0, locale_name{"ru.UTF-8"}.compare(locale_name{"ru_RU.UTF-8"}, locale_name::ignore_culture));
	EXPECT_NE(0, locale_name{"ru_RU"}.compare(locale_name{"ru_RU.UTF-8"}, locale_name::ignore_culture));
}

TEST(LocaleName, LanguageOnly)
{
	using http::server::locale_name;

	EXPECT_EQ(0, locale_name{"ru"}.compare(locale_name{"ru_RU"}, locale_name::language_only));
	EXPECT_EQ(0, locale_name{"ru.UTF-8"}.compare(locale_name{"ru_RU.UTF-8"}, locale_name::language_only));
	EXPECT_EQ(0, locale_name{"ru_RU"}.compare(locale_name{"ru_RU.UTF-8"}, locale_name::language_only));
	EXPECT_EQ(0, locale_name{"ru.CP-1251"}.compare(locale_name{"ru_RU.UTF-8"}, locale_name::language_only));
}

