/*
 * error.hpp
 *
 *  Created on: Oct 6, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_ERROR_HPP_
#define TIP_HTTP_SERVER_ERROR_HPP_

#include <stdexcept>
#include <tip/log.hpp>
#include <tip/http/common/response_status.hpp>

#include <boost/locale.hpp>

namespace tip {
namespace http {
namespace server {

class error: public std::runtime_error {
public:
	using localized_message         = ::boost::locale::message;
public:
	error(std::string const& cat, std::string const& w,
			response_status s
				= response_status::internal_server_error,
			log::logger::event_severity sv = log::logger::ERROR) :
		runtime_error(w), category_(cat), status_(s), severity_(sv) {};
	error(std::string const& cat, localized_message const& w,
			response_status s
				= response_status::internal_server_error,
			log::logger::event_severity sv = log::logger::ERROR) :
		runtime_error(w.str()), category_(cat), status_(s), severity_(sv), message_l10n_(w), is_localized_(true) {};
	virtual ~error() {}

	virtual std::string const&
	name() const;

	virtual int
	code() const
	{ return !0; }

	log::logger::event_severity
	severity() const
	{ return severity_; }

	std::string const&
	category() const
	{ return category_; }

	response_status
	status() const
	{ return status_; }

	localized_message const&
	message_l10n() const
	{ return message_l10n_; }

	bool
	is_localized() const
	{ return is_localized_; }

	virtual void
	log_error(std::string const& message = "") const;
private:
	std::string						category_;
	response_status					status_;
	log::logger::event_severity		severity_;
	localized_message               message_l10n_{};
	bool                            is_localized_{false};
};

class client_error : public error {
public:
	client_error(std::string const& cat, std::string const& w,
			response_status s
				= response_status::bad_request,
				log::logger::event_severity sv = log::logger::ERROR)
		: error(cat, w, s, sv) {};
	client_error(std::string const& cat, error::localized_message const& w,
			response_status s
				= response_status::bad_request,
				log::logger::event_severity sv = log::logger::ERROR)
		: error(cat, w, s, sv) {};
	virtual ~client_error() {};

	virtual std::string const&
	name() const;
};

} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_SERVER_ERROR_HPP_ */
