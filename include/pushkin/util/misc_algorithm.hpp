/*
 * misc_algorithm.hpp
 *
 *  Created on: Aug 25, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_UTIL_MISC_ALGORITHM_HPP_
#define PUSHKIN_HTTP_UTIL_MISC_ALGORITHM_HPP_

namespace psst {
namespace util {

template < typename InputIterator, typename OutputIterator >
size_t
copy_max(InputIterator& f, InputIterator l, size_t max, OutputIterator o)
{
	size_t c = 0;
	while (f != l && c < max) {
		*o++ = *f++;
		++c;
	}
	return c;
}


}  // namespace util
}  // namespace psst

#endif /* PUSHKIN_HTTP_UTIL_MISC_ALGORITHM_HPP_ */
