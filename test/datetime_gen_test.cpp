/*
 * datetime_gen_test.cpp
 *
 *  Created on: Aug 27, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <grammar/grammar_gen_test.hpp>
#include <pushkin/http/common/grammar/datetime_generate.hpp>

namespace gen = psst::http::grammar::gen;
namespace pt = boost::posix_time;

TEST(GenerateTest, DateTime)
{
	namespace karma = boost::spirit::karma;
	typedef std::ostream_iterator<char> output_iterator;
	typedef gen::http_datetime_grammar< output_iterator > http_datetime_grammar;
	http_datetime_grammar gen;

	boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
	std::ostringstream os;
	output_iterator out(os);
	EXPECT_TRUE(karma::generate(out, gen, now));
}

GRAMMAR_GEN_TEST(gen::http_datetime_grammar, HttpDateTime, pt::ptime,
	::testing::Values(
		GenerateHttpDateTime::make_test_data(
			pt::ptime(
				boost::gregorian::date( 2015, boost::date_time::Aug, 27 ),
						pt::hours(17) + pt::minutes(45)
			),
			"Thu, 27 Aug 2015 17:45:00 GMT"
		),
		GenerateHttpDateTime::make_test_data(
			pt::ptime(
				boost::gregorian::date( 2014, boost::date_time::Feb, 1 ),
						pt::hours(0) + pt::minutes(30) + pt::seconds(13)
			),
			"Sat, 01 Feb 2014 00:30:13 GMT"
		)
	)
);
