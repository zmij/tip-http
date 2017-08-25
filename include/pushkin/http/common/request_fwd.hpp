/*
 * request_fwd.hpp
 *
 *  Created on: Aug 25, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_COMMON_REQUEST_FWD_HPP_
#define PUSHKIN_HTTP_COMMON_REQUEST_FWD_HPP_

#include <memory>
#include <iosfwd>

namespace psst {
namespace http {

class request;
typedef std::shared_ptr< request > request_ptr;
typedef std::shared_ptr< request const > request_const_ptr;
typedef std::weak_ptr< request const > request_weak_const_ptr;

enum request_method {
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	OPTIONS,
	TRACE,
	CONNECT,
	PATCH
};

std::ostream&
operator << (std::ostream&, request_method);


}  // namespace http
}  // namespace psst

#endif /* PUSHKIN_HTTP_COMMON_REQUEST_FWD_HPP_ */
