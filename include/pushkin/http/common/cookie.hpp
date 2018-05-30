/*
 * cookie.hpp
 *
 *  Created on: Aug 26, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_COMMON_COOKIE_HPP_
#define PUSHKIN_HTTP_COMMON_COOKIE_HPP_

#include <string>
#include <vector>

#include <boost/date_time.hpp>
#include <boost/optional.hpp>

#include <tip/iri.hpp>

namespace psst {
namespace http {

struct cookie {
	using host_opt      = ::boost::optional< ::tip::iri::host >;
	using path_opt      = ::boost::optional< ::tip::iri::path >;
	using int_opt       = ::boost::optional< ::std::int32_t >;
	using datetime_type = ::boost::posix_time::ptime;
	using datetime_opt  = ::boost::optional< datetime_type>;

	::std::string	name;
	::std::string	value;

	datetime_opt	expires;
	int_opt			max_age;
	host_opt		domain;
	path_opt		path;
	bool			secure;
	bool			http_only;
};

struct cookie_name_cmp {
	bool
	operator()(cookie const& lhs, cookie const& rhs) const
	{
		return lhs.name < rhs.name;
	}
};

// TODO multiset?
using request_cookies = ::std::vector<cookie>;

}  // namespace http
}  // namespace psst


#endif /* PUSHKIN_HTTP_COMMON_COOKIE_HPP_ */
