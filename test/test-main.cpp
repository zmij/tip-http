/*
 * test-main.cpp
 *
 *  Created on: Aug 24, 2015
 *      Author: zmij
 */

#include <tip/log.hpp>

#include <gtest/gtest.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
LOCAL_LOGGING_FACILITY(HTTPTEST, TRACE);
#pragma GCC diagnostic pop

// Initialize the test suite
int
main( int argc, char* argv[] )
{
	logger::set_proc_name(argv[0]);
	logger::set_stream(std::clog);
	logger::min_severity(logger::TRACE);
	logger::use_colors(true);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
