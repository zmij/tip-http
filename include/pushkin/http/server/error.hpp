/*
 * error.hpp
 *
 *  Created on: Oct 6, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_SERVER_ERROR_HPP_
#define PUSHKIN_HTTP_SERVER_ERROR_HPP_

#include <stdexcept>
#include <pushkin/log.hpp>
#include <pushkin/l10n/message.hpp>
#include <boost/locale.hpp>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <pushkin/http/common/response_status.hpp>

namespace psst {
namespace http {
namespace server {

class error: public std::runtime_error {
public:
    using localized_message             = ::boost::locale::message;
    using format                        = ::psst::l10n::format;
    using formatted_message_ptr         = ::psst::l10n::format_shared_ptr;
    using localized_contents            = ::boost::variant<localized_message, formatted_message_ptr>;
    using optional_localized_message    = ::boost::optional<localized_contents>;
    using event_severity                = ::psst::log::logger::event_severity;
public:
    error(std::string const& cat, std::string const& w,
            response_status s
                = response_status::internal_server_error,
            event_severity sv = ::psst::log::logger::ERROR)
        : runtime_error(w), category_(cat), status_(s), severity_(sv) {};
    error(std::string const& cat, localized_message const& w,
            response_status s
                = response_status::internal_server_error,
                event_severity sv = ::psst::log::logger::ERROR)
        : runtime_error(w.str()), category_(cat), status_(s),
          severity_(sv), message_l10n_(w) {};
    error(::std::string const& cat, format&& w,
            response_status s
                            = response_status::internal_server_error,
                            event_severity sv = ::psst::log::logger::ERROR)
        : runtime_error(w.str()), category_(cat), status_(s),
          severity_(sv),
          message_l10n_(::std::make_shared<format>(::std::move(w))){};

    virtual ~error() {}

    virtual std::string const&
    name() const;

    virtual int
    code() const
    { return !0; }

    event_severity
    severity() const
    { return severity_; }

    std::string const&
    category() const
    { return category_; }

    response_status
    status() const
    { return status_; }

    localized_contents const&
    message_l10n() const
    { return *message_l10n_; }

    bool
    is_localized() const
    { return message_l10n_.is_initialized(); }

    virtual void
    log_error(std::string const& message = "") const;
private:
    std::string                     category_;
    response_status                 status_;
    event_severity                  severity_;

    optional_localized_message      message_l10n_;
};

class client_error : public error {
public:
    client_error(std::string const& cat, std::string const& w,
            response_status s
                = response_status::bad_request,
                event_severity sv = ::psst::log::logger::ERROR)
        : error(cat, w, s, sv) {};
    client_error(std::string const& cat, localized_message const& w,
            response_status s
                = response_status::bad_request,
                event_severity sv = ::psst::log::logger::ERROR)
        : error(cat, w, s, sv) {};
    client_error(std::string const& cat, format&& w,
            response_status s
                = response_status::bad_request,
                event_severity sv = ::psst::log::logger::ERROR)
        : error(cat, ::std::move(w), s, sv) {};
    virtual ~client_error() {};

    virtual std::string const&
    name() const;
};

} /* namespace server */
} /* namespace http */
} /* namespace psst */

#endif /* PUSHKIN_HTTP_SERVER_ERROR_HPP_ */
