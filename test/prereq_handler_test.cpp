/*
 * prereq_handler_test.cpp
 *
 *  Created on: Oct 6, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <tip/http/server/prerequisite_handler.hpp>

namespace tip {
namespace http {
namespace server {
namespace test {

struct some_test_prerequisite {
	bool
	operator()(reply const& r) const
	{
		return false;
	}
};

class test_handler : public prerequisite_handler< some_test_prerequisite > {

};

}  // namespace test
}  // namespace server
}  // namespace http
}  // namespace tip

