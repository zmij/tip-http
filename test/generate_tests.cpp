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

BOOST_FUSION_ADAPT_ADT(
test_output_structure,
(std::string&, std::string const&, obj.str, obj.str = val)
(pair_type&, pair_type const&, obj.int_pair, obj.int_pair = val)
(double, double, obj.d, obj.d = val)
)

TEST(GenerateTest, StructEncode)
{
	namespace karma = boost::spirit::karma;
	namespace phx = boost::phoenix;
	typedef std::ostream_iterator< char > sink_type;

	karma::rule< sink_type, std::pair< int, int >() > pair =
			karma::lit('{') << karma::int_ << ':' << karma::int_ << '}';
	karma::rule< sink_type, test_output_structure() > test =
			karma::string << " " << pair << " " << karma::double_;
			//karma::string << " " << karma::double_;

	std::ostringstream os;
	sink_type out(os);
	EXPECT_TRUE(karma::generate(out, test, test_output_structure{ "foo", {1, 5}, 3.14 }));
	//EXPECT_TRUE(karma::generate(out, test, test_output_structure{ "foo", 3.14 }));
	EXPECT_EQ("foo {1:5} 3.14", os.str());
}
