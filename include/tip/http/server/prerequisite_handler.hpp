/*
 * prerequisite_handler.hpp
 *
 *  Created on: Oct 6, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_PREREQUISITE_HANDLER_HPP_
#define TIP_HTTP_SERVER_PREREQUISITE_HANDLER_HPP_

#include <tip/http/server/request_handler.hpp>
#include <tip/util/meta_helpers.hpp>

namespace tip {
namespace http {
namespace server {

namespace detail {

template < size_t Index, typename Prerequisite >
struct prerequisite {
	enum {
		index = Index
	};
	using type = Prerequisite;

	bool
	operator()(reply r) const
	{
		return value(r);
	}

	type value;
};


template < typename IndexTuple, typename ... T >
struct prerequisite_builder;

template < size_t N, typename IndexTuple, typename ... T >
bool
check_nth( prerequisite_builder< IndexTuple, T ... > const&, reply const& r );

template < size_t ... Indexes, typename ... T >
struct prerequisite_builder< util::indexes_tuple< Indexes ... >, T ... >
		: prerequisite< Indexes, T > ... {
	typedef prerequisite_builder<
			util::indexes_tuple< Indexes ... >, T ... > builder_type;
	enum {
		last_index = sizeof ... (T) - 1
	};

	prerequisite_builder() : prerequisite< Indexes, T >() ... {}

	template < size_t N >
	prerequisite< N, typename util::nth_type< 0, T ... >::type > const&
	nth() const
	{
		using nth_prerequisite = prerequisite< N, typename util::nth_type< 0, T ... >::type >;
		return static_cast< nth_prerequisite const& >(*this);
	}

	bool
	operator()(reply const& r) const
	{
		return check_nth<last_index>(*this, r);
	}
};

template < size_t N, typename IndexTuple, typename ... T >
struct check_helper {
	using builder_type = prerequisite_builder< IndexTuple, T ... >;
	using prev_check = check_helper< N - 1, IndexTuple, T ... >;
	static bool
	check(builder_type const& b, reply const& r)
	{
		return prev_check::check(b, r) && b.template nth<N>()(r);
	}
};

template < typename IndexTuple, typename ... T >
struct check_helper< 0, IndexTuple, T ... > {
	using builder_type = prerequisite_builder< IndexTuple, T ... >;
	static bool
	check(builder_type const& b, reply const& r)
	{
		return b.template nth<0>()(r);
	}
};

template < size_t N, typename IndexTuple, typename ... T >
bool
check_nth( prerequisite_builder< IndexTuple, T ... > const& builder, reply const& r )
{
	using check = check_helper< N, IndexTuple, T ... >;
	return check::check(builder, r);
}

}  // namespace detail

template < typename ... Prerequisite >
class prerequisites :
		public detail::prerequisite_builder<
			typename util::index_builder< sizeof ... (Prerequisite) >::type,
			Prerequisite ...  > {
public:
	typedef detail::prerequisite_builder<
		typename util::index_builder< sizeof ... (Prerequisite) >::type,
		Prerequisite ... > base_type;
};

template <>
class prerequisites<> {
public:
	inline bool
	operator()(reply const&) const
	{
		return true;
	}
};

template < typename ... Prerequisite >
class prerequisite_handler : public request_handler {
private:
	using prerequisites_type = prerequisites< Prerequisite ... >;
public:
	prerequisite_handler() {}
	virtual ~prerequisite_handler() {}
private:
	virtual void
	do_handle_request( reply r )
	{
		try {
			if (check_prerequisites(r))
				checked_handle_request(r);
		} catch (...) {
			r.server_error();
		}
	}

	virtual void
	checked_handle_request(reply r) = 0;

	bool
	check_prerequisites( reply const& r ) const
	{
		return prerequisites_(r);
	}
private:
	prerequisites_type prerequisites_;
};

//template < typename T, typename ... Prerequisites >
//struct prereqisite_handler_func {
//	using static_type = T;
//	using prerequisites_type = prereqiusites< Prerequisite ... >;
//
//private:
//	prerequisites_type prerequisites_;
//};

}  // namespace server
}  // namespace http
}  // namespace tip


#endif /* TIP_HTTP_SERVER_PREREQUISITE_HANDLER_HPP_ */
