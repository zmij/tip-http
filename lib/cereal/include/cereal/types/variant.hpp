/*
 * variant.hpp
 *
 *  Created on: May 29, 2017
 *      Author: zmij
 */

#ifndef CEREAL_TYPES_VARIANT_HPP_
#define CEREAL_TYPES_VARIANT_HPP_

#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <boost/variant.hpp>

namespace cereal {

template < typename ... T >
void
prologue(JSONInputArchive&, ::boost::variant<T...> const&) {}
template < typename ... T >
void
epilogue(JSONInputArchive&, ::boost::variant<T...> const&) {}
template < typename ... T >
void
prologue(JSONOutputArchive&, ::boost::variant<T...> const&) {}
template < typename ... T >
void
epilogue(JSONOutputArchive&, ::boost::variant<T...> const&) {}

template < typename ... T >
void
CEREAL_LOAD_FUNCTION_NAME(JSONInputArchive&, ::boost::variant<T...>&) {}

namespace detail {

struct cerealize_visitor : ::boost::static_visitor<> {
    JSONOutputArchive& ar;

    cerealize_visitor(JSONOutputArchive& ar) : ar{ar} {}
    template< typename T >
    void
    operator()(T const& v) const
    {
        ar(v);
    }
};

} /* namespace detail */


template < typename ... T >
void
CEREAL_SAVE_FUNCTION_NAME(JSONOutputArchive& ar, ::boost::variant<T...> const& v)
{
    detail::cerealize_visitor viz{ar};
    ::boost::apply_visitor(viz, v);
}

} /* namespace cereal */



#endif /* CEREAL_TYPES_VARIANT_HPP_ */
