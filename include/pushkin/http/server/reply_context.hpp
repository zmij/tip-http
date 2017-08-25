/*
 * reply_context.hpp
 *
 *  Created on: Aug 28, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_SERVER_DETAIL_REPLY_CONTEXT_HPP_
#define PUSHKIN_HTTP_SERVER_DETAIL_REPLY_CONTEXT_HPP_

#include <pushkin/http/server/detail/context_registry.hpp>

namespace psst {
namespace http {
namespace server {

template < typename Context >
void
add_context(reply& r, Context* ctx)
{
	static_assert(std::is_base_of< reply::context, Context >::value,
			"Context must be derived from reply::context");
	(void)static_cast<reply::id*>(&Context::id);
	r.context_registry().template add_context(ctx);
}

template < typename Context >
bool
has_context(reply const& r)
{
	static_assert(std::is_base_of< reply::context, Context >::value,
			"Context must be derived from reply::context");
	(void)static_cast<reply::id*>(&Context::id);
	return r.context_registry().template has_context<Context>();
}

template < typename Context >
Context&
use_context(reply& r)
{
	static_assert(std::is_base_of< reply::context, Context >::value,
			"Context must be derived from reply::context");
	(void)static_cast<reply::id*>(&Context::id);
	return r.context_registry().template use_context<Context>();
}

template < typename Context >
Context const&
use_context(reply const& r)
{
    static_assert(std::is_base_of< reply::context, Context >::value,
            "Context must be derived from reply::context");
    (void)static_cast<reply::id*>(&Context::id);
    return r.context_registry().template use_context<Context>();
}


} /* namespace server */
} /* namespace http */
} /* namespace psst */


#endif /* PUSHKIN_HTTP_SERVER_DETAIL_REPLY_CONTEXT_HPP_ */
