/**
 * json_body_context.hpp
 *
 *  Created on: 01 сент. 2015 г.
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_JSON_BODY_CONTEXT_HPP_
#define TIP_HTTP_SERVER_JSON_BODY_CONTEXT_HPP_

#include <tip/http/server/reply.hpp>
#include <tip/http/server/request_handler.hpp>
#include <tip/http/server/reply_context.hpp>
#include <tip/http/server/error.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/locale_message.hpp>

#include <memory>

namespace tip {
namespace http {
namespace server {

namespace detail {

template < typename Archive, typename T >
class has_serialize_member {
    static Archive& ar;
    template < typename U >
    static ::std::true_type
    test( decltype( ::std::declval<U>().serialize(ar) ) const* );

    template < typename U >
    static ::std::false_type
    test(...);
public:
    static constexpr bool value = decltype( test<T>(nullptr) )::value;
};

template < typename Archive, typename T >
class has_load_member {
    static Archive& ar;
    template < typename U >
    static ::std::true_type
    test( decltype( ::std::declval<U>().load(ar) ) const* );

    template < typename U >
    static ::std::false_type
    test(...);
public:
    static constexpr bool value = decltype( test<T>(nullptr) )::value;
};

template < typename Archive, typename T >
class has_save_member {
    static Archive& ar;
    template < typename U >
    static ::std::true_type
    test( decltype( ::std::declval<U>().save(ar) ) const* );

    template < typename U >
    static ::std::false_type
    test(...);
public:
    static constexpr bool value = decltype( test<T>(nullptr) )::value;
};

struct __json_meta_function_helper {
    template < typename T >
    __json_meta_function_helper(T const&);
};

template < typename Archive >
::std::false_type
serialize(Archive const&, __json_meta_function_helper const&);
template < typename Archive >
::std::false_type
load(Archive&, __json_meta_function_helper const&);
template < typename Archive >
::std::false_type
save(Archive&, __json_meta_function_helper const&);


template < typename Archive, typename T >
class has_serialize_function {
    static Archive& ar;
    static T&       val;
public:
    static constexpr bool value = !::std::is_same<
        decltype( serialize( ar, val ) ),
        ::std::false_type >::value;
};

template < typename Archive, typename T >
class has_load_function {
    static Archive& ar;
    static T&       val;
public:
    static constexpr bool value = !::std::is_same<
        decltype( load( ar, val ) ),
        ::std::false_type >::value;
};

template < typename Archive, typename T >
class has_save_function {
    static Archive& ar;
    static T&       val;
public:
    static constexpr bool value = !::std::is_same<
        decltype( save( ar, val ) ),
        ::std::false_type >::value;
};

enum class unmarshal_type {
    load_member,
    serialize_member,
    load_function,
    serialize_function,
};

template < unmarshal_type V >
using load_type = ::std::integral_constant< unmarshal_type, V >;

template <typename ... T>
struct serialization_not_found;

template < typename T, typename U >
struct serialization_not_found< T, U > {
    static_assert( !::std::is_same<T, U>::value,
            "Serialization functions not found" );
};

template < typename Archive, typename T >
struct load_type_selector
    : ::std::conditional<
        has_load_member<Archive, T>::value,
        load_type<unmarshal_type::load_member>,
        typename::std::conditional<
             has_serialize_member<Archive, T>::value,
             load_type<unmarshal_type::serialize_member>,
             typename ::std::conditional<
                 has_load_function<Archive, T>::value,
                 load_type<unmarshal_type::load_function>,
                 typename ::std::conditional<
                     has_serialize_function<Archive, T>::value,
                     load_type<unmarshal_type::serialize_function>,
                     serialization_not_found<T, T>
                 >::type
             >::type
        >::type
    >::type {};

enum class marshal_type {
    save_member,
    serialize_member,
    save_function,
    serialize_function
};

template < marshal_type V >
using save_type = ::std::integral_constant< marshal_type, V >;

template < typename Archive, typename T >
struct save_type_selector
    : ::std::conditional<
        has_save_member<Archive, T>::value,
        save_type< marshal_type::save_member >,
        typename ::std::conditional<
            has_serialize_member<Archive, T>::value,
            save_type< marshal_type::serialize_member >,
            typename ::std::conditional<
                has_save_function<Archive, T>::value,
                save_type< marshal_type::save_function >,
                typename ::std::conditional<
                    has_serialize_function<Archive, T>::value,
                    save_type< marshal_type::serialize_function >,
                    serialization_not_found<T, T>
                >::type
            >::type
        >::type
    >::type {};
}  /* namespace detail */

class json_body_context: public reply::context {
public:
    using input_arhive_type = cereal::JSONInputArchive;
    using output_archive_type = cereal::JSONOutputArchive;

    enum status_type {
        empty_body,
        invalid_json,
        json_ok
    };
    static reply::id id;
public:
    json_body_context( reply const& r);
    virtual ~json_body_context();

    status_type
    status() const;
    explicit operator bool()
    { return status() == json_ok; }

    input_arhive_type&
    incoming();
    output_archive_type&
    outgoing();

    template < typename T >
    void
    input(T& val)
    {
        load_impl(val,
                detail::load_type_selector<input_arhive_type,
                    typename ::std::decay<T>::type>{});
    }

    template < typename T >
    void
    output(T&& val)
    {
        save_impl(::std::forward<T>(val),
                detail::save_type_selector<output_archive_type,
                    typename ::std::decay<T>::type>{});
    }

    template < typename T >
    json_body_context&
    operator << (T&& val)
    {
        output(::std::forward<T>(val));
        return *this;
    }
private:
    template < typename T >
    void
    load_impl(T& val, detail::load_type< detail::unmarshal_type::load_member > const&)
    {
        val.load(incoming());
    }
    template < typename T >
    void
    load_impl(T& val, detail::load_type< detail::unmarshal_type::serialize_member > const&)
    {
        val.serialize(incoming());
    }
    template < typename T >
    void
    load_impl(T& val, detail::load_type< detail::unmarshal_type::load_function > const&)
    {
        load(incoming(), val);
    }
    template < typename T >
    void
    load_impl(T& val, detail::load_type< detail::unmarshal_type::serialize_function > const&)
    {
        serialize(incoming(), val);
    }

    template < typename T >
    void
    save_impl(T&& val, detail::save_type< detail::marshal_type::save_member > const&)
    {
        val.save(outgoing());
    }
    template < typename T >
    void
    save_impl(T&& val, detail::save_type< detail::marshal_type::serialize_member > const&)
    {
        val.serialize(outgoing());
    }
    template < typename T >
    void
    save_impl(T&& val, detail::save_type< detail::marshal_type::save_function > const&)
    {
        save(outgoing(), ::std::forward<T>(val));
    }
    template < typename T >
    void
    save_impl(T&& val, detail::save_type< detail::marshal_type::serialize_function > const&)
    {
        serialize(outgoing(), ::std::forward<T>(val));
    }
private:
    struct impl;
    using pimpl = std::unique_ptr<impl>;
    pimpl pimpl_;
};

template < typename Archive >
void
CEREAL_SAVE_FUNCTION_NAME(Archive& ar, error const& e)
{
    ar(     ::cereal::make_nvp("code",      e.code()                ));
    ar(     ::cereal::make_nvp("error",     e.name()                ));
    ar(     ::cereal::make_nvp("severity",  e.severity()            ));
    ar(     ::cereal::make_nvp("category",  e.category()            ));

    if(e.is_localized())
        ar( ::cereal::make_nvp("message",   e.message_l10n()        ));
    else
        ar( ::cereal::make_nvp("message",   std::string(e.what())   ));
}

class json_error : public client_error {
public:
    json_error(std::string const& w) : client_error("JSONPARSE", w) {}
    json_error(std::string const& cat, std::string const& w,
            response_status s
                = response_status::internal_server_error,
                event_severity sv = ::psst::log::logger::ERROR)
            : client_error(cat, w, s, sv) {}
    virtual ~json_error() {}

    virtual std::string const&
    name() const;
};

struct json_error_sender {
    static void
    send_error(::tip::http::server::reply r, ::tip::http::server::error const& e)
    {
        e.log_error();
        json_body_context& json = use_context<json_body_context>(r);
        json << e;
        r.done(e.status());
    }
};

template < typename BodyType, bool allow_empty_body = false >
class json_transformer {
public:
    using request_type = BodyType;
    using pointer = std::shared_ptr< request_type >;

    pointer
    operator()(reply r) const
    {
        json_body_context& json = use_context< json_body_context >(r);
        if ((bool)json) {
            pointer req(std::make_shared< request_type >());
            try {
                json.input(*req);
                return req;
            } catch (std::exception const& e) {
                throw json_error(e.what());
            } catch (...) {
                throw json_error("Invalid JSON request");
            }
        }
        switch (json.status()) {
        case json_body_context::empty_body:
            if (!allow_empty_body)
                throw json_error("Empty body (expected JSON object)");
            break;
        case json_body_context::invalid_json:
            throw json_error("Malformed JSON");
        default:
            throw json_error("Unknown JSON status");
        }
        return pointer{};
    }

    static void
    error(reply r, class error const& e)
    {
        json_error_sender::send_error(r, e);
    }
};

} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_SERVER_JSON_BODY_CONTEXT_HPP_ */
