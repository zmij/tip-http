/**
 * locale_manager_test.cpp
 *
 *  Created on: 10 окт. 2015 г.
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <pushkin/http/server/locale_manager.hpp>
#include "config.hpp"
#include <boost/locale.hpp>

namespace psst {
namespace http {
namespace server {
namespace test {

namespace l10n = boost::locale;

TEST(LocaleManager, Service)
{
	locale_manager::io_service io_service;

	ASSERT_NO_THROW(locale_manager& mgr = boost::asio::use_service< locale_manager >(io_service));
}

TEST(LocaleManager, Support)
{
	locale_manager::io_service io_service;
	locale_manager& loc_mgr = boost::asio::use_service< locale_manager >(io_service);

	loc_mgr.add_languages("en;ru;de");
	EXPECT_EQ("en.UTF-8", loc_mgr.default_locale_name());
	EXPECT_TRUE(loc_mgr.is_locale_supported("en"));
	EXPECT_TRUE(loc_mgr.is_locale_supported("en_US"));
	EXPECT_TRUE(loc_mgr.is_locale_supported("en-US"));
	EXPECT_TRUE(loc_mgr.is_locale_supported("en_GB"));
	EXPECT_TRUE(loc_mgr.is_locale_supported("en_GB.UTF-8"));

	EXPECT_TRUE(loc_mgr.is_locale_supported("ru"));
	EXPECT_TRUE(loc_mgr.is_locale_supported("ru_RU"));
	EXPECT_TRUE(loc_mgr.is_locale_supported("ru-RU"));
	EXPECT_TRUE(loc_mgr.is_locale_supported("ru_RU.UTF-8"));
	EXPECT_TRUE(loc_mgr.is_locale_supported("ru.UTF-8"));
	EXPECT_TRUE(loc_mgr.is_locale_supported("ru.CP-1251"));
}

TEST(LocaleManager, DISABLED_LocaleCheck)
{
	locale_manager::io_service io_service;
	locale_manager& loc_mgr = boost::asio::use_service< locale_manager >(io_service);

	loc_mgr.add_languages(http::test::LANGUAGES);
	loc_mgr.add_messages_path(http::test::L10N_ROOT);
	loc_mgr.add_messages_domain("httptest");

	ASSERT_TRUE(loc_mgr.is_locale_supported("en"));
	ASSERT_TRUE(loc_mgr.is_locale_supported("ru"));

	locale_manager::language_set const& langs = loc_mgr.supported_languages();
	for (auto&& lang : langs) {
		std::locale loc = loc_mgr.get_locale(lang);
		std::ostringstream os;
		os.imbue(loc);
		os << l10n::translate("*** LANGUAGE NAME ***");
		std::cerr << "Language " << lang << " national name is " << os.str() << "\n";
		EXPECT_NE("*** LANGUAGE NAME ***", os.str());
	}
}

}  // namespace test
}  // namespace server
}  // namespace http
}  // namespace tip
