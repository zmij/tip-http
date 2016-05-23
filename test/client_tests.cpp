/*
 * client_tests.cpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <tip/http/client/service.hpp>
#include <tip/http/client/session.hpp>
#include <tip/http/client/session_pool.hpp>
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

TEST(HttpClient, NoEventQueue)
{
    boost::asio::io_service io_service;
    request::iri_type iri;
    ASSERT_TRUE(request::parse_iri("http://127.0.0.1:65535", iri));

    bool session_closed = false;

    session_ptr session = session::create(io_service, iri,
            [](session_ptr s){ local_log() << "Session idle"; },
            [&session_closed](session_ptr s, ::std::exception_ptr e)
                { session_closed = true; }, {});

    io_service.run();
    EXPECT_TRUE(session_closed);
}

TEST(HttpClient, Session)
{
    boost::asio::io_service io_service;
    request::iri_type iri;
    ASSERT_TRUE(request::parse_iri("http://google.com/", iri));

    bool session_closed = false;
    response_ptr resp;

    session_ptr session = session::create(io_service, iri,
            [](session_ptr s){ local_log() << "Session idle"; },
            [&session_closed](session_ptr s, ::std::exception_ptr e)
                { session_closed = true; }, {});

    session->send_request(request::create(GET, iri, {}),
    [&](request_ptr, response_ptr r){
        local_log() << "Received a response: Content-Length: "
                << r->content_length() << " body size " << r->body_.size()
                << "\nHeaders\n"
                << r->headers_;
        resp = r;
        session->close();
        io_service.stop();
    },
    [&](std::exception_ptr) {io_service.stop();});

    io_service.run();
    EXPECT_TRUE(session_closed);
    ASSERT_TRUE(resp.get());
    EXPECT_LT(0, resp->body_.size());
}

TEST(HttpClient, SessionPool)
{
    boost::asio::io_service io_service;
    request::iri_type iri;
    ASSERT_TRUE(request::parse_iri("http://google.com/", iri));

    auto pool = session_pool::create(io_service, iri,
    [](session_ptr s){
        local_log() << "Connection pool closed";
    }, {}, 1);
    pool->send_request(request::create(GET, iri, {}),
    [&](request_ptr, response_ptr){io_service.stop();},
    [&](std::exception_ptr) {io_service.stop();});

    EXPECT_TRUE(pool.get());
    local_log(logger::DEBUG) << "Start io_service";
    io_service.run();
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
        io_service.stop();
    },
    [&](std::exception_ptr) {
        error = true;
        io_service.stop();
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
            io_service.stop();
        },
        [&](std::exception_ptr) {
            error = true;
            io_service.stop();
        });
    }
    io_service.run();
    EXPECT_FALSE(resp.get());
    EXPECT_TRUE(error);
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
        io_service.stop();
    },
    [&](std::exception_ptr) {
        error = true;
        io_service.stop();
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
                << r->content_length() << " body size " << r->body_.size()
                << "\nHeaders\n"
                << r->headers_;
        resp = r;
        io_service.stop();
    },
    [&](std::exception_ptr) {
        error = true;
        io_service.stop();
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
                << r->content_length() << " body size " << r->body_.size()
                << "\nHeaders\n"
                << r->headers_;
        resp = r;
        io_service.stop();
    },
    [&](std::exception_ptr) {
        error = true;
        io_service.stop();
    });
    io_service.run();
    EXPECT_TRUE(resp.get());
    EXPECT_FALSE(error);
}

}  // namespace test
}  // namespace client
}  // namespace http
}  // namespace tip

