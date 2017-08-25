/*
 * basic_auth_context.cpp
 *
 *  Created on: Jun 27, 2017
 *      Author: zmij
 */

#include <pushkin/http/server/basic_auth_context.hpp>

#include <pushkin/http/common/grammar/header_fields_parse.hpp>
#include <pushkin/http/common/request.hpp>

namespace psst {
namespace http {
namespace server {

reply::id basic_auth_context::id;

basic_auth_context::basic_auth_context(reply r) : reply::context{r}
{
    namespace qi = ::boost::spirit::qi;
    using string_iterator = ::std::string::const_iterator;
    using basic_auth_grammar = grammar::parse::basic_auth_grammar<string_iterator>;
    static basic_auth_grammar parser;

    auto hdrs = r.request_headers().equal_range(Authorization);
    if (hdrs.first != hdrs.second) {
        string_iterator f = hdrs.first->second.begin();
        string_iterator l = hdrs.first->second.end();
        ::std::string auth_info;
        if (qi::parse(f, l, parser, auth_info)) {
            auto pos = auth_info.find(':');
            if (pos != ::std::string::npos) {
                user_       = auth_info.substr(0, pos);
                password_   = auth_info.substr(pos + 1);
            } else {
                user_ = auth_info;
            }
        }
    }
}

basic_auth_context::~basic_auth_context()
{
}

} /* namespace server */
} /* namespace http */
} /* namespace psst */
