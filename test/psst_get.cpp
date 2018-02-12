/*
 * psst_get.cpp
 *
 *  Created on: Feb 12, 2018
 *      Author: zmij
 */

#include <iostream>

#include <boost/program_options.hpp>

#include <pushkin/http/client/service.hpp>
#include <pushkin/http/common/response.hpp>
#include <pushkin/log.hpp>

LOCAL_LOGGING_FACILITY(PSSTGET, TRACE);

int
main(int argc, char* argv[])
try {
    namespace po = ::boost::program_options;
    using http_service = ::psst::http::client::service;
    using http_response_ptr = ::psst::http::response_ptr;

    logger::set_proc_name(argv[0]);
    logger::set_stream(::std::clog);
    logger::flush_stream(false);
    logger::min_severity(logger::TRACE);

    po::options_description desc("Program options");
    ::std::string url;

    desc.add_options()
        ("help,h", "show options description")
        ("url,u", po::value<std::string>(&url)->required(), "URL")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }
    po::notify(vm);

    ::boost::asio::io_service svc;
    auto& http = ::boost::asio::use_service< http_service >(svc);

    http.get_async(url,
    [&]( http_response_ptr r )
    {
        ::std::ostream_iterator<char> out{::std::cout};
        ::std::copy(r->body_.begin(), r->body_.end(), out);
        svc.stop();
    },
    [&](::std::exception_ptr ex)
    {
        svc.stop();
    });

    svc.run();

    return 0;
} catch (::std::exception const& e) {
    ::std::cerr << "Exception: " << e.what() << "\n";
    return 1;
} catch (...) {
    ::std::cerr << "Unexpected exception\n";
    return 2;
}
