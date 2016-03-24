/*
 * generate_gests.cpp
 *
 *  Created on: Aug 20, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/support_adapt_adt_attributes.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/include/adapt_adt.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <string>
#include <sstream>
#include <iterator>

typedef std::pair< int, int > pair_type;
struct test_output_structure {
	std::string str;
	pair_type int_pair;
	double d;
};

TEST(GenerateTest, StructEncode)
{
	namespace karma = boost::spirit::karma;
	namespace phx = boost::phoenix;
	typedef std::ostream_iterator< char > sink_type;
	using karma::_1;
	using karma::_2;
	using karma::_3;
	using karma::_val;


	karma::rule< sink_type, std::pair< int, int >() > pair =
			karma::lit('{') << karma::int_ << ':' << karma::int_ << '}';
	karma::rule< sink_type, test_output_structure() > test =
			(karma::string << " " << pair << " " << karma::double_)
			[
			 	 _1 = phx::bind( &test_output_structure::str, _val ),
				 _2 = phx::bind( &test_output_structure::int_pair, _val ),
				 _3 = phx::bind( &test_output_structure::d, _val )
			];
			//karma::string << " " << karma::double_;

	std::ostringstream os;
	sink_type out(os);
	double d_val = 3.14;
	test_output_structure val{ "foo", {1, 5}, d_val };
	EXPECT_EQ(d_val, val.d);
	EXPECT_TRUE(karma::generate(out, test, val));
	EXPECT_EQ("foo {1:5} 3.14", os.str());
}
