/*
 * read_result.hpp
 *
 *  Created on: Aug 25, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_COMMON_READ_RESULT_HPP_
#define PUSHKIN_HTTP_COMMON_READ_RESULT_HPP_

#include <functional>
#include <boost/logic/tribool.hpp>

namespace psst {
namespace util {

template < typename ... T >
struct read_result {
	typedef std::function< read_result( T ... ) > read_callback_type;
	boost::tribool		result;
	read_callback_type	callback;
};

}  // namespace util
}  // namespace psst

#endif /* PUSHKIN_HTTP_COMMON_READ_RESULT_HPP_ */
