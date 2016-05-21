/*
 * request_dispatcher_fwd.hpp
 *
 *  Created on: May 19, 2016
 *      Author: zmij
 */

#ifndef HTTP_SERVER_REQUEST_DISPATCHER_FWD_HPP_
#define HTTP_SERVER_REQUEST_DISPATCHER_FWD_HPP_

namespace tip {
namespace http {
namespace server {

struct reply;
struct request;
class request_dispatcher;
using request_dispatcher_ptr = std::shared_ptr<request_dispatcher>;

} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* HTTP_SERVER_REQUEST_DISPATCHER_FWD_HPP_ */
