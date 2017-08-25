/*
 * request_dispatcher_fwd.hpp
 *
 *  Created on: May 19, 2016
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_SERVER_REQUEST_DISPATCHER_FWD_HPP_
#define PUSHKIN_HTTP_SERVER_REQUEST_DISPATCHER_FWD_HPP_

namespace psst {
namespace http {
namespace server {

struct reply;
struct request;
class request_dispatcher;
using request_dispatcher_ptr = std::shared_ptr<request_dispatcher>;

} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* PUSHKIN_HTTP_SERVER_REQUEST_DISPATCHER_FWD_HPP_ */
