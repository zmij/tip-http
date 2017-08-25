/*
 * language_context.cpp
 *
 *  Created on: Aug 28, 2015
 *      Author: zmij
 */

#include <pushkin/http/server/language_context.hpp>

#include <pushkin/http/common/grammar/header_fields_parse.hpp>
#include <pushkin/http/common/request.hpp>

namespace psst {
namespace http {
namespace server {

reply::id language_context::id;

language_context::language_context(reply r) : reply::context(r)
{
	namespace qi = boost::spirit::qi;
	using string_iterator = std::string::const_iterator;
	using accept_languages_grammar =
			grammar::parse::accept_languages_grammar<string_iterator>;
	static accept_languages_grammar parser;
	auto hdrs = r.request_headers().equal_range(AcceptLanguage);
	for (; hdrs.first != hdrs.second; ++hdrs.first) {
		string_iterator f = hdrs.first->second.begin();
		string_iterator l = hdrs.first->second.end();
		qi::parse(f, l, parser, accept_);
	}
}

language_context::~language_context()
{
}

accept_languages const&
language_context::languages() const
{
	return accept_;
}

bool
language_context::empty() const
{
	return accept_.empty();
}

} /* namespace server */
} /* namespace http */
} /* namespace psst */
