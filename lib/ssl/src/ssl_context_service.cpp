/*
 * ssl_context_service.cpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#include <tip/ssl_context_service.hpp>
#include <boost/filesystem.hpp>

namespace tip {
namespace ssl {

namespace {

char const* const TLS_CIPHER_LIST =
        "HIGH:!aNULL:!eNULL:!kECDH:!DSS:!MD5:!EXP:!PSK:!SRP:!CAMELLIA:!SEED:@STRENGTH";

} /* namespace  */


ssl_context_service::io_service::id ssl_context_service::id;

struct ssl_context_service::impl {
	context_type context_;

	impl() : context_(context_type::tlsv12)
	{
	    using ssl_context       = ::boost::asio::ssl::context;
	    auto res = ::SSL_CTX_set_cipher_list(context_.native_handle(), TLS_CIPHER_LIST);
	    if (res != 1) {
	        throw ::std::runtime_error{"Failed to set TLS cipher list"};
	    }
	    context_.set_options(
            ssl_context::default_workarounds |
            ssl_context::no_sslv2 |
            ssl_context::no_sslv3 |
            ssl_context::single_dh_use
        );
	}

	void
	load_verify_files(path_type const& p)
	{
		namespace fs = boost::filesystem;
		if (fs::exists(p)) {
			if (fs::is_directory(p)) {
				fs::directory_iterator f(p);
				fs::directory_iterator eod;
				for (; f != eod; ++f) {
					if (fs::is_regular_file(*f)) {
						context_.load_verify_file(f->path().native());
					}
				}
			} else if (fs::is_regular_file(p)) {
				context_.load_verify_file(p.native());
			}
		}
	}

	void
	load_default_verify_path()
	{
	    context_.set_default_verify_paths();
	}
};

ssl_context_service::ssl_context_service(io_service& svc) : base_type(svc), pimpl_(new impl)
{
}

ssl_context_service::~ssl_context_service()
{
}

ssl_context_service::context_type&
ssl_context_service::context()
{
	return pimpl_->context_;
}

ssl_context_service::context_type const&
ssl_context_service::context() const
{
	return pimpl_->context_;
}

void
ssl_context_service::load_verify_files(path_type const& p)
{
	pimpl_->load_verify_files(p);
}

void
ssl_context_service::load_default_verify_path()
{
    pimpl_->load_default_verify_path();
}

void
ssl_context_service::shutdown_service()
{
}

} /* namespace ssl */
} /* namespace tip */
