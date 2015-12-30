/*
 * pathmatcher_tests.cpp
 *
 *  Created on: Aug 29, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <tip/http/server/detail/path_matcher.hpp>

using tip::http::server::detail::path_matcher;
TEST(PathMatcher, Construction)
{

	EXPECT_NO_THROW( path_matcher() );
	EXPECT_NO_THROW( path_matcher("/*") );

	EXPECT_THROW( path_matcher(""), std::logic_error );
	EXPECT_THROW( path_matcher("/"), std::logic_error );
	EXPECT_THROW( path_matcher("//"), std::logic_error );
}

TEST(PathMatcher, EmptyMatchTest)
{
	using tip::iri::path;

	path_matcher empty;
	path empty_path{"/"};
	EXPECT_TRUE(empty_path.is_rooted());
	EXPECT_TRUE(empty_path.empty());
	EXPECT_TRUE(empty.empty());
	EXPECT_TRUE(empty.matches(empty_path));

	path_matcher kleene("/*");
	EXPECT_FALSE(kleene.empty());
	EXPECT_TRUE(kleene.matches(empty_path));

	path_matcher matcher("/foo/bar");
	EXPECT_FALSE(matcher.empty());
	EXPECT_FALSE(matcher.matches(empty_path));
}

TEST(PathMatcher, SegmentSortOrder)
{
	typedef tip::util::wildcard< std::string > wildcard;
	typedef tip::http::server::detail::path_segment path_segment;

	{
		wildcard kleene_w{true};
		wildcard single_w{false};
		wildcard named_w{std::string("name")};
		EXPECT_GT(kleene_w, single_w);
		EXPECT_GT(single_w, named_w);
		EXPECT_GT(kleene_w, named_w);
	}
	{
		path_segment foo("foo");
		path_segment bar("bar");
		path_segment kleene(wildcard{true});
		path_segment single(wildcard{false});
		path_segment named(wildcard{std::string("param")});

		EXPECT_LT(bar, foo);
		EXPECT_LT(bar, kleene);
		EXPECT_LT(bar, single);
		EXPECT_LT(bar, named);
		EXPECT_LT(foo, kleene);
		EXPECT_LT(foo, single);
		EXPECT_LT(foo, named);

		EXPECT_GT(kleene, single);
		EXPECT_GT(single, named);
		EXPECT_GT(kleene, named);
	}
}

TEST(PathMatcher, SortOrder)
{
	path_matcher empty;
	path_matcher kleene("/*");

	EXPECT_LT(kleene, empty);
	EXPECT_LT(path_matcher("/foo"), kleene);
	EXPECT_LT(path_matcher("/foo/bar"), path_matcher("/foo"));
	EXPECT_LT(path_matcher("/foo/bar"), path_matcher("/foo/*"));
	EXPECT_LT(path_matcher("/?"), kleene);
	EXPECT_LT(path_matcher("/:param:"), path_matcher("/?"));
}

class BasePathMatcherTest : public ::testing::TestWithParam< tip::iri::path > {
public:
	BasePathMatcherTest() : empty_matcher(), kleene_matcher("/*"){}
protected:
	path_matcher empty_matcher;
	path_matcher kleene_matcher;
};

TEST_P(BasePathMatcherTest, NonEmptyPath)
{
	tip::iri::path p = GetParam();
	ASSERT_FALSE(p.empty());
	EXPECT_FALSE(empty_matcher.matches(p));
	EXPECT_TRUE(kleene_matcher.matches(p));
}

INSTANTIATE_TEST_CASE_P(PathMatcher, BasePathMatcherTest,
	::testing::Values(
		tip::iri::path::parse("/foo"),
		tip::iri::path::parse("/foo/bar")
	)
);

class PathMatcherTest : public ::testing::TestWithParam< std::pair< path_matcher, tip::iri::path > > {
public:
	static ParamType
	make_test_data(path_matcher const& m, tip::iri::path const& p)
	{
		return std::make_pair(m, p);
	}
};

TEST_P(PathMatcherTest, Matches)
{
	ParamType param = GetParam();
	ASSERT_FALSE(param.first.empty());

	EXPECT_TRUE(param.first.matches(param.second));
}

INSTANTIATE_TEST_CASE_P(PathMatcher, PathMatcherTest,
	::testing::Values(
		PathMatcherTest::make_test_data(
			path_matcher{"/*"},
			tip::iri::path::parse("/")),
		PathMatcherTest::make_test_data(
			path_matcher{"/foo"},
			tip::iri::path::parse("/foo")),
		PathMatcherTest::make_test_data(
			path_matcher{"/foo"},
			tip::iri::path::parse("foo")),
		PathMatcherTest::make_test_data(
			path_matcher{"/foo/bar"},
			tip::iri::path::parse("/foo/bar")),
		PathMatcherTest::make_test_data(
			path_matcher{"/foo/?"},
			tip::iri::path::parse("/foo/bar")),
		PathMatcherTest::make_test_data(
			path_matcher{"/foo/*"},
			tip::iri::path::parse("/foo/bar")),
		PathMatcherTest::make_test_data(
			path_matcher{"/foo/*"},
			tip::iri::path::parse("/foo/bar/baz")),
		PathMatcherTest::make_test_data(
			path_matcher{"/foo/:bar:"},
			tip::iri::path::parse("/foo/bar")),
		PathMatcherTest::make_test_data(
			path_matcher{"/foo/bar/baz"},
			tip::iri::path::parse("/foo/bar/baz")),
		PathMatcherTest::make_test_data(
			path_matcher{"/foo/?/baz"},
			tip::iri::path::parse("/foo/bar/baz")),
		PathMatcherTest::make_test_data(
			path_matcher{"/foo/bar/*"},
			tip::iri::path::parse("/foo/bar/baz")),
		PathMatcherTest::make_test_data(
			path_matcher{"/foo/:bar:/baz"},
			tip::iri::path::parse("/foo/bar/baz"))
	)
);
