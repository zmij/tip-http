/*
 * facet_test.cpp
 *
 *  Created on: Sep 25, 2015
 *      Author: zmij
 */

#include <tip/util/facet.hpp>
#include <gtest/gtest.h>
#include <string>

namespace test {

struct void_facet {
	virtual ~void_facet() {}
};

struct void_facet_child : void_facet {
	virtual ~void_facet_child() {}
};

struct int_facet {
	int_facet(int v) : val(v) {};
	virtual ~int_facet() {}

	int val;
};

struct string_int_facet {
	string_int_facet(std::string const& s, int i) : str(s), val(i) {}
	virtual ~string_int_facet() {}

	std::string str;
	int val;
};

typedef tip::util::detail::facet_factory<void_facet> void_facet_factory;
typedef tip::util::detail::facet_factory<int_facet, int> int_facet_factory;
typedef tip::util::detail::facet_factory<string_int_facet, std::string, int > string_int_facet_factory;

typedef tip::util::facet_registry<void_facet> void_facet_registry;
static_assert(std::is_same< void_facet_registry::factory_type, void_facet_factory >::value,
		"Correct factory type");

typedef tip::util::facet_registry<string_int_facet, std::string, int> string_facet_registry;
static_assert(std::is_same< string_facet_registry::factory_type,
		string_int_facet_factory >::value, "Correct factory type" );

}  // namespace test

TEST(Facet, FacetFactory)
{
	{
		test::void_facet_factory void_f;
		auto f = void_f.create<test::void_facet>();
		delete f;

		test::void_facet_registry reg;
		test::void_facet& vf1 = reg.use_facet< test::void_facet >();
		test::void_facet& vf2 = tip::util::use_facet< test::void_facet >(reg);
		EXPECT_EQ((test::void_facet*)&vf1, (test::void_facet*)&vf2);
		EXPECT_NO_THROW(reg.use_facet< test::void_facet_child >());
		test::void_facet_child* cf = new test::void_facet_child();
		EXPECT_THROW(reg.add_facet(cf), std::logic_error);
		delete cf;
		test::void_facet_child& vfc = reg.use_facet<test::void_facet_child>();
		EXPECT_NE((test::void_facet*)&vf1, (test::void_facet*)&vfc);
	}
	{
		test::int_facet_factory int_f(5);
		test::int_facet* f = int_f.create<test::int_facet>();
		EXPECT_EQ(5, f->val);
		delete f;
	}
	{
		test::string_int_facet_factory factory("bla", 42);
		auto f = factory.create<test::string_int_facet>();
		EXPECT_EQ("bla", f->str);
		EXPECT_EQ(42, f->val);
		delete f;
	}
}

