// Copyright (C) 2020-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef LEXY_ENGINE_MINUS_HPP_INCLUDED
#define LEXY_ENGINE_MINUS_HPP_INCLUDED

#include <lexy/engine/base.hpp>

namespace lexy
{
/// Matches `Matcher` but only if none of the `Excepts` match the same input.
template <typename Matcher, typename... Excepts>
struct engine_minus : lexy::engine_matcher_base
{
    static_assert((lexy::engine_is_matcher<Matcher> && ... && lexy::engine_is_matcher<Excepts>));

    enum class error_code
    {
        minus_failure = 1,
    };

    static constexpr error_code error_from_matcher(typename Matcher::error_code ec)
    {
        LEXY_PRECONDITION(ec != typename Matcher::error_code());
        return error_code(int(ec) + 1);
    }
    static constexpr auto error_to_matcher(error_code ec)
    {
        LEXY_PRECONDITION(int(ec) > 1);
        return typename Matcher::error_code(int(ec) - 1);
    }

    template <typename Reader>
    static constexpr error_code match(Reader& reader)
    {
        auto save = reader;

        // First match on the original input.
        if (auto ec = Matcher::match(reader); ec != typename Matcher::error_code())
            return error_from_matcher(ec);

        // Then check whether any of the Excepts match on the same input.
        auto partial      = lexy::partial_reader(save, reader.cur());
        auto except_match = ((lexy::engine_try_match<Excepts>(partial) && partial.eof()) || ...);
        if (except_match)
            // They did, so we don't match.
            return error_code::minus_failure;

        return error_code();
    }
};
} // namespace lexy

#endif // LEXY_ENGINE_MINUS_HPP_INCLUDED

