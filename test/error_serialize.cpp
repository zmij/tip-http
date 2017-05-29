/*
 * error_serialize.hpp
 *
 *  Created on: May 29, 2017
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <tip/http/server/json_body_context.hpp>


namespace tip {
namespace http {
namespace server {

TEST(Error, Serialize)
{
    using ::boost::locale::translate;
    error err{"", error::format(translate("This is error {1}")) % "format"};

    ASSERT_TRUE(err.is_localized());

    ::std::ostringstream os;
    {
        cereal::JSONOutputArchive ar{os};
        ar(err);
    }
    ::std::cerr << os.str() << "\n";
}

} /* namespace server */
} /* namespace http */
} /* namespace tip */


