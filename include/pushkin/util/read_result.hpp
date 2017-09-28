
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
    using read_callback_type = ::std::function< read_result( T... ) >;
    boost::tribool        result;
    read_callback_type    callback;

    read_result(boost::tribool r, read_callback_type c);
    read_result(read_result const&);
    read_result(read_result&&);
};

template < typename ... T >
read_result<T...>::read_result(boost::tribool r, read_callback_type c)
    : result{ r }, callback{ c }
{
}
template < typename ... T >
read_result<T...>::read_result(read_result const& rhs)
    : result{ rhs.result }, callback{ rhs.callback }
{
}

template < typename ... T >
read_result<T...>::read_result(read_result&& rhs)
    : result{ ::std::move(rhs.result) }, callback{ ::std::move(rhs.callback) }
{
}

}  // namespace util
}  // namespace psst

#endif /* PUSHKIN_HTTP_COMMON_READ_RESULT_HPP_ */
