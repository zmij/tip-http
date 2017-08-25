/*
 * uuid.hpp
 *
 *  Created on: May 23, 2017
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_SERVER_IO_UUID_HPP_
#define PUSHKIN_HTTP_SERVER_IO_UUID_HPP_

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/string_generator.hpp>

#include <pushkin/http/server/reply.hpp>

namespace psst {
namespace http {
namespace server {
namespace detail {

template <>
struct query_param_extractor<::boost::uuids::uuid>
    : query_param_extractor_base<::boost::uuids::uuid,
                  query_param_extractor<::boost::uuids::uuid>> {

    using base_type     = query_param_extractor_base<::boost::uuids::uuid,
                            query_param_extractor<::boost::uuids::uuid>>;
    using value_type    = base_type::value_type;
    using base_type::operator();

    bool
    operator()(::std::string const& param, value_type& val) const
    {
        static ::boost::uuids::string_generator _gen;
        try {
            val = _gen(param);
            return true;
        } catch (...) {
            return false;
        }
    }
};

} /* namespace detail */
} /* namespace server */
} /* namespace http */
} /* namespace psst */




#endif /* PUSHKIN_HTTP_SERVER_IO_UUID_HPP_ */
