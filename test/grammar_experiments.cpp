/**
 * grammar_experiments.cpp
 *
 *  Created on: 27 авг. 2015 г.
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <grammar/grammar_parse_test.hpp>
#include <boost/date_time.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <tip/iri/grammar/iri_parse.hpp>

namespace test {
template < typename InputIterator >
struct ptime_grammar :
		boost::spirit::qi::grammar< InputIterator, boost::posix_time::ptime::time_duration_type() > {
	typedef boost::posix_time::ptime::time_duration_type value_type;
	ptime_grammar() : ptime_grammar::base_type(time)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;

		using qi::_val;
		using qi::_1;
		using qi::_2;
		using qi::_3;
		using qi::_a;
		using qi::_b;
		using qi::_c;
		using qi::_pass;

		_2digits = qi::uint_parser< std::uint32_t, 10, 2, 2 >();
		time = (_2digits >> ':' >> _2digits >> ':' >> _2digits)
			[ _pass = (_1 < 24) && (_2 < 60) && (_3 < 60),
			  _val = phx::construct< value_type >(_1, _2, _3)];
	}
	boost::spirit::qi::rule< InputIterator, value_type() > time;
	boost::spirit::qi::rule< InputIterator, std::uint32_t() > _2digits;
};

template < typename InputIterator >
struct exclude_grammar :
		boost::spirit::qi::grammar< InputIterator, std::string()> {
	typedef std::string value_type;

	exclude_grammar() : exclude_grammar::base_type(root)
	{
		namespace qi = boost::spirit::qi;
		//root =
	}
	boost::spirit::qi::rule< InputIterator, value_type()> root;

	tip::iri::grammar::parse::iunreserved_grammar< InputIterator > iunreserved;
	tip::iri::grammar::parse::pct_encoded_grammar< InputIterator > pct_encoded;
	tip::iri::grammar::parse::sub_delims_grammar< InputIterator > sub_delims;
};
}  // namespace test

GRAMMAR_TEST(test::ptime_grammar, TestTime,
		::testing::Values(
			"00:00:00", "23:59:59", "01:30:00", "14:48:58"
		),
		::testing::Values(
			"00:00", "24:00:00", "15:13:60"
		)
);

TEST(GrammarTest, AcceptLanguage)
{
	namespace qi = boost::spirit::qi;
	namespace phx = boost::phoenix;
	typedef std::string::const_iterator string_iterator;
	qi::rule< string_iterator, float() > qvalue =
			 (qi::lit(";q=") >> qi::float_[ qi::_val = qi::_1 ])
			 | qi::eps[qi::_val = 1.0f];

	std::string t = ";q=0.4";
	string_iterator f = t.begin();
	string_iterator l = t.end();
	float q = 0.0f;
	EXPECT_TRUE(qi::parse(f, l, qvalue, q));
	EXPECT_EQ(0.4f, q);
	t = "";
	f = t.begin();
	l = t.end();
	EXPECT_TRUE(qi::parse(f, l, qvalue, q));
	EXPECT_EQ(1.0f, q);
}

