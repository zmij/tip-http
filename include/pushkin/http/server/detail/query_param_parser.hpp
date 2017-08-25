/*
 * query_param_parser.hpp
 *
 *  Created on: May 23, 2017
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_SERVER_DETAIL_QUERY_PARAM_PARSER_HPP_
#define PUSHKIN_HTTP_SERVER_DETAIL_QUERY_PARAM_PARSER_HPP_

#include <map>
#include <string>

namespace psst {
namespace http {
namespace server {
namespace detail {

template < typename T >
struct query_param_extractor;

template < typename T, typename Extractor >
struct query_param_extractor_base {
    using query_type            = std::multimap< std::string, std::string >;
    using value_type            = T;

    /**
     * Extract value from parsed query
     * @param query
     * @param name
     * @param val
     * @return
     */
    bool
    operator()(query_type const& query, ::std::string const& name, value_type& val) const
    {
        auto f = query.equal_range(name);
        if (f.first != f.second) {
            // Call extractor operator from derived type
            return rebind()( f.first->second, val );
        }
        return false;
    }
private:
    Extractor&
    rebind()
    { return static_cast<Extractor&>(*this); }
    Extractor const&
    rebind() const
    { return static_cast<Extractor const&>(*this); }
};

template < typename T, typename A, typename Extractor >
struct query_param_extractor_base<::std::vector<T, A>, Extractor > {
    using query_type            = std::multimap< std::string, std::string >;
    using value_type            = ::std::vector<T, A>;
    using element_type          = T;

    /**
     * Extract vector of values from parsed query
     * @param query
     * @param name
     * @param val
     * @return
     */
    bool
    operator()(query_type const& query, ::std::string const& name, value_type& val) const
    {
        auto f = query.equal_range(name);
        if (f.first != f.second) {
            value_type tmp;
            query_param_extractor<element_type> extract;
            for (auto c = f.first; c != f.second; ++c) {
                element_type e;
                if (!extract(c->second, e))
                    return false;
                tmp.emplace_back(::std::move(e));
            }
            tmp.swap(val);
            return true;
        }
        return false;
    }
};

template < typename T >
struct query_param_extractor : query_param_extractor_base<T, query_param_extractor<T>> {
    using base_type     = query_param_extractor_base<T, query_param_extractor<T>>;
    using value_type    = typename base_type::value_type;
    using base_type::operator();

    bool
    operator()(::std::string const& param, value_type& val) const
    {
        ::std::istringstream is{param};
        return (bool)(is >> val);
    }
};

template <>
struct query_param_extractor<::std::string>
            : query_param_extractor_base<::std::string, query_param_extractor<::std::string>>{
    using base_type     = query_param_extractor_base<::std::string, query_param_extractor<::std::string>>;
    using value_type    = base_type::value_type;
    using base_type::operator();

    bool
    operator()(::std::string const& param, value_type& val) const
    {
        val = param;
        return true;
    }
};

template <typename T>
struct query_param_parser : query_param_extractor_base<T, query_param_parser<T>> {
    using base_type     = query_param_extractor_base<T, query_param_parser<T>>;
    using value_type    = typename base_type::value_type;
    using parse_func    = ::std::function<bool(::std::string const&, value_type&)>;

    using base_type::operator();

    query_param_parser(parse_func f)
        : parser_{f} {}
    template < typename F >
    query_param_parser(F f)
        : parser_{f} {}

    bool
    operator()(::std::string const& param, value_type& val) const
    {
        if (parser_)
            return parser_(param, val);
        return false;
    }
private:
    parse_func parser_;
};

} /* namespace detail */
} /* namespace server */
} /* namespace http */
} /* namespace psst */

#endif /* PUSHKIN_HTTP_SERVER_DETAIL_QUERY_PARAM_PARSER_HPP_ */
