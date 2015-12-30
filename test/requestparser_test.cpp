/**
 * request_handler.cpp
 *
 *  Created on: Jul 17, 2015
 *      Author: zmij
 */

#include <tip/http/common/request.hpp>

#include <tip/log/log.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <tuple>

#include "http/config.hpp"

LOCAL_LOGGING_FACILITY(HTTPTEST, TRACE);

namespace {

const std::vector<std::string> GET_REQUESTS {
	"01.get.request",
	"02.get.request",
	"03.get.request"
};

const std::string URLENCODED_REQUEST = "04.post.request";
const std::string MULTIPART_REQUEST = "05.post.request";

}  // namespace

TEST(RequestParser, GetRequest)
{
	using namespace tip::http;
	ASSERT_TRUE(!test::DATA_DIR.empty());
	for (auto file_name : GET_REQUESTS) {
		std::string req_name = test::DATA_DIR + file_name;
		std::ifstream file(req_name, std::ios_base::binary);
		ASSERT_TRUE((bool)file);

		request req;

		EXPECT_TRUE(req.read_headers(file));

		EXPECT_EQ(GET, req.method);
		EXPECT_TRUE(!req.path.empty());
		EXPECT_TRUE(!req.query.empty());
		EXPECT_TRUE(!req.headers_.empty());
	}
}

TEST(RequestParser, PostUrlEncoded)
{
	using namespace tip::http;
	ASSERT_TRUE(!test::DATA_DIR.empty());
	std::string req_name = test::DATA_DIR + URLENCODED_REQUEST;
	std::ifstream file(req_name, std::ios_base::binary);
	ASSERT_TRUE((bool)file);

	request req;

	EXPECT_TRUE(req.read_headers(file));
	EXPECT_TRUE(req.read_body(file).result);

	EXPECT_EQ(POST, req.method);
	EXPECT_TRUE(!req.path.empty());
	EXPECT_TRUE(!req.headers_.empty());

	EXPECT_TRUE(!req.body_.empty());
}

TEST(RequestParser, ParsePostMultipart)
{
	using namespace tip::http;
	ASSERT_TRUE(!test::DATA_DIR.empty());
	std::string req_name = test::DATA_DIR + MULTIPART_REQUEST;
	std::ifstream file(req_name, std::ios_base::binary);
	ASSERT_TRUE((bool)file);

	request req;

	EXPECT_TRUE(req.read_headers(file));
	EXPECT_TRUE(req.read_body(file).result);

	EXPECT_EQ(POST, req.method);
	EXPECT_TRUE(!req.path.empty());
	EXPECT_TRUE(!req.headers_.empty());

	EXPECT_TRUE(!req.body_.empty());
}
