/*
 * language_context.hpp
 *
 *  Created on: Aug 28, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_SERVER_LANGUAGE_CONTEXT_HPP_
#define PUSHKIN_HTTP_SERVER_LANGUAGE_CONTEXT_HPP_

#include <pushkin/http/server/reply.hpp>
#include <pushkin/http/server/reply_context.hpp>

namespace psst {
namespace http {
namespace server {

class language_context : public reply::context {
public:
	static reply::id id;
public:
	language_context(reply r);
	virtual ~language_context();

	accept_languages const&
	languages() const;
	bool
	empty() const;
private:
	accept_languages accept_;
};

} /* namespace server */
} /* namespace http */
} /* namespace psst */

#endif /* PUSHKIN_HTTP_SERVER_LANGUAGE_CONTEXT_HPP_ */
