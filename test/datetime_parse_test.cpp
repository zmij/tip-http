/*
 * datetime_parse.cpp
 *
 *  Created on: Aug 26, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <grammar/grammar_parse_test.hpp>
#include <tip/http/common/grammar/datetime_parse.hpp>

namespace http = tip::http;
namespace parse = http::grammar::parse;

GRAMMAR_TEST(parse::http_time_grammar, HttpTime,
	::testing::Values(
		"00:00:00", "23:59:59", "12:00:00"
	),
	::testing::Values(
		"24:00:00", "00:00", "12:60:00"
	)
);

GRAMMAR_TEST(parse::date_rfc1123_grammar, DateRFC1123,
	::testing::Values(
		"Mon, 03 Jun 1994", "Tue, 01 May 2001"
	),
	::testing::Values(
		"Mon 32 Jan 2014", "Mon 3 Jun 1994", "Tue 1 May 2001"
	)
);

GRAMMAR_TEST(parse::datetime_rfc1123_grammar, DateTimeRFC1123,
	::testing::Values(
		"Mon, 03 Jun 1994 00:00:00 GMT",
		"Tue, 01 May 2001 23:59:59 GMT"
	),
	::testing::Values(
		"Mon 32 Jan 2014", "Mon 3 Jun 1994", "Tue 1 May 2001",
		"Tue 01 May 2001 23:59:59"
	)
);

GRAMMAR_TEST(parse::date_rfc850_grammar, DateRFC850,
	::testing::Values(
		"Monday, 02-Jul-82", "Monday, 23-Feb-15"
	),
	::testing::Values(
		"Mon, 03 Jun 1994 00:00:00 GMT",
		"Tue, 01 May 2001 23:59:59 GMT"
	)
);

GRAMMAR_TEST(parse::datetime_rfc850_grammar, DateTimeRFC850,
	::testing::Values(
		"Monday, 02-Jul-82 00:00:00 GMT",
		"Monday, 23-Feb-15 23:59:45 GMT"
	),
	::testing::Values(
		"Mon, 03 Jun 1994 00:00:00 GMT",
		"Tue, 01 May 2001 23:59:59 GMT",
		"Monday, 23-Feb-15 23:59:45"
	)
);

GRAMMAR_TEST(parse::datetime_asctime_grammar, ANSIDateTime,
	::testing::Values(
		"Mon Jun  2 08:15:23 1996",
		"Sun Nov  6 08:49:37 1994",
		"Sun Nov 13 08:49:37 1994"
	),
	::testing::Values(
		"Mon, 03 Jun 1994 00:00:00 GMT",
		"Tue, 01 May 2001 23:59:59 GMT",
		"Monday, 23-Feb-15 23:59:45",
		"Sun Nov 6 08:49:37 1994"
	)
);

GRAMMAR_TEST(parse::http_datetime_grammar, HttpDateTime,
	::testing::Values(
		"Mon, 03 Jun 1994 00:00:00 GMT",
		"Tue, 01 May 2001 23:59:59 GMT",
		"Monday, 02-Jul-82 00:00:00 GMT",
		"Monday, 23-Feb-15 23:59:45 GMT",
		"Mon Jun  2 08:15:23 1996",
		"Sun Nov  6 08:49:37 1994",
		"Sun Nov 13 08:49:37 1994"
	),
	::testing::Values(
			"Monday, 23-Feb-15 23:59:45",
			"Sun Nov 6 08:49:37 1994"
	)
);
