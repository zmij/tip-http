/*
 * client_tests.cpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <tip/http/client/service.hpp>
#include <tip/http/common/response.hpp>
#include <tip/log.hpp>

LOCAL_LOGGING_FACILITY(CLIENTTEST, TRACE);

TEST(HttpClient, ServiceGet)
{
	boost::asio::io_service io_service;
	ASSERT_NO_THROW( boost::asio::use_service< tip::http::client::service >(io_service) );
}

TEST(HttpClient, GetRequest)
{
	typedef tip::http::client::service http_service;
	using tip::http::response_ptr;
	boost::asio::io_service io_service;
	http_service& svc = boost::asio::use_service< http_service >(io_service);
	svc.get("http://google.com/", [](response_ptr r){
		local_log() << "Received a response: Content-Length: "
				<< r->content_length() << " body size " << r->body_.size();
	});
	io_service.run();
}
