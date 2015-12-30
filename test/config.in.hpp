/*
 * config.in.hpp
 *
 *  Created on: Jul 17, 2015
 *      Author: zmij
 */

#ifndef HTTP_CONFIG_IN_HPP_
#define HTTP_CONFIG_IN_HPP_

#include <string>

namespace tip {
namespace http {
namespace test {

const std::string DATA_DIR = "@CMAKE_CURRENT_BINARY_DIR@/";
const std::string L10N_ROOT = "@L10N_MO_DIRECTORY@";
const std::string LANGUAGES = "@L10N_LANGUAGES@";
const std::string DOMAINS = "@L10N_DOMAINS@";

}  // namespace test
}  // namespace http
}  // namespace tip

#endif /* HTTP_CONFIG_IN_HPP_ */
