/*
 * prereq_handler_test.cpp
 *
 *  Created on: Oct 6, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <pushkin/http/server/prerequisite_handler.hpp>

namespace psst {
namespace http {
namespace server {
namespace test {

struct some_test_prerequisite {
	bool
	operator()(reply const&) const
	{
		return false;
	}
};

class test_handler : public prerequisite_handler< some_test_prerequisite > {

};

}  // namespace test
}  // namespace server
}  // namespace http
}  // namespace psst

