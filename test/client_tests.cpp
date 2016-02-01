/*
 * client_tests.cpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <tip/http/client/service.hpp>
#include <tip/http/common/response.hpp>
#include <tip/ssl_context_service.hpp>
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
	response_ptr resp;
	bool error = false;
	svc.get("http://google.com/",
	[&](response_ptr r){
		local_log() << "Received a response: Content-Length: "
				<< r->content_length() << " body size " << r->body_.size();
		resp = r;
	},
	[&](std::exception_ptr) {
		error = true;
	});
	io_service.run();
	EXPECT_FALSE(error);
	ASSERT_TRUE(resp.get());
	EXPECT_LT(0, resp->body_.size());
}

TEST(HttpClient, FailConnecting)
{
	typedef tip::http::client::service http_service;
	using tip::http::response_ptr;
	boost::asio::io_service io_service;
	http_service& svc = boost::asio::use_service< http_service >(io_service);
	response_ptr resp;
	bool error = false;
	svc.get("http://127.0.0.1:65535/",
	[&](response_ptr r){
		local_log() << "Received a response: Content-Length: "
				<< r->content_length() << " body size " << r->body_.size();
		resp = r;
	},
	[&](std::exception_ptr) {
			error = true;
	});
	io_service.run();
	EXPECT_FALSE(resp.get());
	EXPECT_TRUE(error);
}


TEST(HttpsClient, FailVerifyCert)
{
	typedef tip::http::client::service http_service;
	using tip::http::response_ptr;
	boost::asio::io_service io_service;
	http_service& svc = boost::asio::use_service< http_service >(io_service);
	response_ptr resp;
	bool error = false;
	svc.get("https://mail.ru/",
	[&](response_ptr r){
		local_log() << "Received a response: Content-Length: "
				<< r->content_length() << " body size " << r->body_.size();
		resp = r;
	},
	[&](std::exception_ptr) {
			error = true;
	});
	io_service.run();
	EXPECT_FALSE(resp.get());
	EXPECT_TRUE(error);
}

TEST(HttpsClient, VerifyCertOK)
{
	typedef tip::http::client::service http_service;
	typedef tip::ssl::ssl_context_service ssl_service;

	using tip::http::response_ptr;
	boost::asio::io_service io_service;
	ssl_service& ssl_svc = boost::asio::use_service< ssl_service >(io_service);
	ssl_svc.context().set_default_verify_paths();
	http_service& svc = boost::asio::use_service< http_service >(io_service);
	response_ptr resp;
	bool error = false;
	svc.get("https://mail.ru/",
	[&](response_ptr r){
		local_log() << "Received a response: Content-Length: "
				<< r->content_length() << " body size " << r->body_.size();
		resp = r;
	},
	[&](std::exception_ptr) {
			error = true;
	});
	io_service.run();
	EXPECT_TRUE(resp.get());
	EXPECT_FALSE(error);
}
