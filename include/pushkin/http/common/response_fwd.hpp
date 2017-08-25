/*
 * response_fwd.hpp
 *
 *  Created on: Aug 25, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_COMMON_RESPONSE_FWD_HPP_
#define PUSHKIN_HTTP_COMMON_RESPONSE_FWD_HPP_

#include <memory>

namespace psst {
namespace http {

struct response;
typedef std::shared_ptr<response> response_ptr;
typedef std::shared_ptr<response const> response_const_ptr;


}  // namespace http
}  // namespace psst


#endif /* PUSHKIN_HTTP_COMMON_RESPONSE_FWD_HPP_ */
