/**
 * @file /tip-server/src/tip/http/server/request_handler.cpp
 * @brief
 * @date Jul 8, 2015
 * @author: zmij
 */

#include <pushkin/http/server/request_handler.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>

#include <pushkin/log.hpp>
#include <pushkin/http/common/request.hpp>
#include <pushkin/http/server/mime_types.hpp>

namespace psst {
namespace http {
namespace server {

// LOCAL_LOGGING_FACILITY(REQHND, TRACE);

void
request_handler::handle_request(reply req)
{
	// local_log() << "Handle request for " << req.request()->path;

	do_handle_request(req);
}

}  // namespace server
}  // namespace http
}  // namespace psst
