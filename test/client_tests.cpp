/*
 * client_tests.cpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <tip/http/client/service.hpp>
#include <tip/http/client/session.hpp>
#include <tip/http/common/response.hpp>
#include <tip/ssl_context_service.hpp>
#include <tip/log.hpp>

namespace tip {
namespace http {
namespace client {
namespace test {

LOCAL_LOGGING_FACILITY(CLIENTTEST, TRACE);

TEST(HttpClient, ServiceGet)
{
    boost::asio::io_service io_service;
    ASSERT_NO_THROW( boost::asio::use_service< service >(io_service) );
}

TEST(HttpClient, GetRequest)
{
    using http_service = service;
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
    using http_service = service;
    boost::asio::io_service io_service;
    http_service& svc = boost::asio::use_service< http_service >(io_service);
    response_ptr resp;
    bool error = false;
    for (int i = 0; i < 10; ++i) {
        svc.get("http://127.0.0.1:65535/",
        [&](response_ptr r){
            local_log() << "Received a response: Content-Length: "
                    << r->content_length() << " body size " << r->body_.size();
            resp = r;
        },
        [&](std::exception_ptr) {
                error = true;
        });
    }
    io_service.run();
    EXPECT_FALSE(resp.get());
    EXPECT_TRUE(error);
}

TEST(HttpClient, NoEventQueue)
{
    boost::asio::io_service io_service;
    request::iri_type iri;
    ASSERT_TRUE(request::parse_iri("http://127.0.0.1:65535", iri));

    bool session_closed = false;

    session_ptr session = session::create(io_service, iri,
            [&session_closed](session_ptr s){ session_closed = true; }, {});

    io_service.run();
    EXPECT_TRUE(session_closed);
}

TEST(HttpsClient, FailVerifyCert)
{
    using http_service = tip::http::client::service;
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
    using http_service = service;
    using ssl_service = tip::ssl::ssl_context_service;

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

TEST(HttpsClient, GetAppleCert)
{
    using http_service = service;
    using ssl_service = tip::ssl::ssl_context_service;

    boost::asio::io_service io_service;
    ssl_service& ssl_svc = boost::asio::use_service< ssl_service >(io_service);
    ssl_svc.context().set_default_verify_paths();
    http_service& svc = boost::asio::use_service< http_service >(io_service);

    response_ptr resp;
    bool error = false;
    svc.get("https://static.gc.apple.com/public-key/ggc-prod-2.cer",
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

}  // namespace test
}  // namespace client
}  // namespace http
}  // namespace tip

