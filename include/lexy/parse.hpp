// Copyright (C) 2020-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef LEXY_PARSE_HPP_INCLUDED
#define LEXY_PARSE_HPP_INCLUDED

#include <lexy/_detail/invoke.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl/base.hpp>
#include <lexy/production.hpp>
#include <lexy/result.hpp>

namespace lexy
{
struct _no_parse_state
{};

template <typename To, typename... Args>
constexpr bool _is_convertible = false;
template <typename To, typename Arg>
constexpr bool _is_convertible<To, Arg> = std::is_convertible_v<Arg, To>;

template <typename Input, typename State, typename Callback>
struct _parse_handler
{
    const Input*               _input;
    State&                     _state;
    LEXY_EMPTY_MEMBER Callback _callback;

    template <typename Production>
    static auto _value_cb()
    {
        using value = lexy::production_value<Production>;
        if constexpr (lexy::is_callback<typename value::type>)
            return value::get;
        else
            return LEXY_DECLVAL(lexy::sink_callback<typename value::type>);
    }

    template <typename Production>
    using result_type_for = lexy::result<typename decltype(_value_cb<Production>())::return_type,
                                         typename Callback::return_type>;

    template <typename Production>
    constexpr auto get_sink(Production)
    {
        return lexy::production_value<Production>::get.sink();
    }

    template <typename Production, typename Iterator>
    constexpr auto start_production(Production, Iterator pos)
    {
        return pos;
    }

    template <typename Production, typename Iterator, typename... Args>
    constexpr auto finish_production(Production, Iterator, Args&&... args)
    {
        using result_type = result_type_for<Production>;
        using value       = typename lexy::production_value<Production>;

        if constexpr (lexy::is_callback_for<typename value::type, Args&&...>)
        {
            // We have a callback for those arguments; invoke it.
            return lexy::invoke_as_result<result_type>(lexy::result_value, value::get,
                                                       LEXY_FWD(args)...);
        }
        else if constexpr (lexy::is_sink<typename value::type> //
                           && _is_convertible<typename result_type::value_type, Args&&...>)
        {
            // We don't have a matching callback, but it is a single argument that has the
            // correct type already. Assume it came from the list sink and
            // construct the result without invoking a callback.
            return result_type(lexy::result_value, LEXY_FWD(args)...);
        }
        else
        {
            // We're missing a callback overload.
            static_assert(_detail::error<Production, Args...>,
                          "missing callback overload for production");
            return result_type();
        }
    }

    template <typename Production, typename Iterator, typename Error>
    constexpr auto error(Production p, Iterator pos, Error&& error)
    {
        lexy::error_context err_ctx(p, *_input, pos);
        return lexy::invoke_as_result<result_type_for<Production>>(lexy::result_error, _callback,
                                                                   err_ctx, LEXY_FWD(error));
    }
};

template <typename T>
constexpr bool _is_parse_handler = false;
template <typename Input, typename State, typename Callback>
constexpr bool _is_parse_handler<_parse_handler<Input, State, Callback>> = true;

/// Parses the production into a value, invoking the callback on error.
template <typename Production, typename Input, typename State, typename Callback>
constexpr auto parse(const Input& input, State&& state, Callback callback)
{
    using handler_t = _parse_handler<Input, std::decay_t<State>, Callback>;

    auto                handler = handler_t{&input, state, LEXY_MOV(callback)};
    auto                reader  = input.reader();
    lexy::parse_context context(Production{}, handler, reader.cur());

    using rule = lexy::production_rule<Production>;
    return lexy::rule_parser<rule, lexy::context_value_parser>::parse(context, reader);
}

template <typename Production, typename Input, typename Callback>
constexpr auto parse(const Input& input, Callback callback)
{
    return parse<Production>(input, _no_parse_state{}, callback);
}
} // namespace lexy

namespace lexyd
{
template <auto Fn>
struct _state : rule_base
{
    template <typename NextParser>
    struct parser
    {
        template <typename Context, typename Reader, typename... Args>
        LEXY_DSL_FUNC auto parse(Context& context, Reader& reader, Args&&... args) ->
            typename Context::result_type
        {
            auto& handler   = context.handler();
            using handler_t = std::remove_reference_t<decltype(handler)>;

            if constexpr (lexy::_is_parse_handler<handler_t>)
            {
                using state_type = decltype(handler._state);
                static_assert(!std::is_same_v<state_type, lexy::_no_parse_state>,
                              "lexy::dsl::state requires passing a state to lexy::parse()");

                if constexpr (std::is_same_v<decltype(Fn), decltype(nullptr)>)
                    return NextParser::parse(context, reader, LEXY_FWD(args)..., handler._state);
                else
                    return NextParser::parse(context, reader, LEXY_FWD(args)...,
                                             lexy::_detail::invoke(Fn, handler._state));
            }
            else
            {
                // Not used with parse, ignore.
                return NextParser::parse(context, reader, LEXY_FWD(args)...);
            }
        }
    };
};

/// Produces the parsing state passed to `parse()` as a value.
constexpr auto parse_state = _state<nullptr>{};

/// Produces a member of the parsing state as a value.
template <auto Fn>
constexpr auto parse_state_member = _state<Fn>{};
} // namespace lexyd

#endif // LEXY_PARSE_HPP_INCLUDED

