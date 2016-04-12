/**
 * response_status.hpp
 *
 *  Created on: 28 авг. 2015 г.
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_RESPONSE_STATUS_HPP_
#define TIP_HTTP_COMMON_RESPONSE_STATUS_HPP_

#include <iostream>

namespace tip {
namespace http {

/** The status of the reply.
 * [RFC 2616 Fielding, et al.](http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html)
 */
enum class response_status {
    /** Informational */
    continue_                       = 100,
    switching_protocols             = 101,
    /** Successful */
    ok                              = 200,
    created                         = 201,
    accepted                        = 202,
    non_authoritative_information   = 203,
    no_content                      = 204,
    reset_content                   = 205,
    partial_content                 = 206,
    /** Redirection */
    multiple_choices                = 300,
    moved_permanently               = 301,
    found                           = 302,
    see_other                       = 303,
    not_modified                    = 304,
    use_proxy                       = 305,
    temporary_redirect              = 307,
    /** Client error */
    bad_request                     = 400,
    unauthorized                    = 401,
    payment_required                = 402,
    forbidden                       = 403,
    not_found                       = 404,
    method_not_allowed              = 405,
    not_acceptable                  = 406,
    proxy_authentication_required   = 407,
    request_timeout                 = 408,
    conflict                        = 409,
    gone                            = 410,
    length_required                 = 411,
    precondition_failed             = 412,
    request_entity_too_large        = 413,
    request_uri_too_long            = 414,
    unsupported_media_type          = 415,
    requested_range_not_satisfiable = 416,
    expectation_failed              = 417,
    /** Server error */
    internal_server_error           = 500,
    not_implemented                 = 501,
    bad_gateway                     = 502,
    service_unavailable             = 503,
    gateway_timeout                 = 504,
    http_version_not_supported      = 505
};  // enum response_status

enum class response_class {
    informational   = 1,
    successful      = 2,
    redirection     = 3,
    client_error    = 4,
    server_error    = 5
};

inline response_class
status_class(response_status s)
{
    return static_cast< response_class >( static_cast< int >(s) / 100 );
}

inline bool
is_error(response_status s)
{
    return static_cast< int >(s) / 100 >= 4;
}

inline bool
is_error(response_class c)
{
    return c >= response_class::client_error;
}

inline ::std::ostream&
operator << (::std::ostream& os, response_status const& val)
{
    std::ostream::sentry s(os);
    if (s) {
        os << static_cast<int>(val);
    }
    return os;
}


}  // namespace http
}  // namespace tip



#endif /* TIP_HTTP_COMMON_RESPONSE_STATUS_HPP_ */
