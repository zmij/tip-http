/*
 * misc_tests.cpp
 *
 *  Created on: Apr 12, 2016
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <tip/http/common/response_status.hpp>

namespace tip {
namespace http {
namespace common {
namespace test {

TEST(ResponseTest, StatusClass)
{
    EXPECT_EQ(response_class::informational, status_class(response_status::continue_));

    EXPECT_EQ(response_class::successful, status_class(response_status::ok));
    EXPECT_EQ(response_class::successful, status_class(response_status::accepted));
    EXPECT_EQ(response_class::successful, status_class(response_status::partial_content));

    EXPECT_EQ(response_class::redirection, status_class(response_status::multiple_choices));
    EXPECT_EQ(response_class::redirection, status_class(response_status::see_other));
    EXPECT_EQ(response_class::redirection, status_class(response_status::temporary_redirect));

    EXPECT_EQ(response_class::client_error, status_class(response_status::bad_request));
    EXPECT_EQ(response_class::client_error, status_class(response_status::not_found));

    EXPECT_EQ(response_class::server_error, status_class(response_status::internal_server_error));
    EXPECT_EQ(response_class::server_error, status_class(response_status::bad_gateway));
}

}  // namespace test
}  // namespace common
}  // namespace http
}  // namespace tip

