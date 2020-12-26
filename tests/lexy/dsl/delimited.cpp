// Copyright (C) 2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <lexy/dsl/delimited.hpp>

#include "verify.hpp"
#include <lexy/dsl/ascii.hpp>
#include <lexy/dsl/eof.hpp>
#include <lexy/dsl/list.hpp>
#include <lexy/dsl/option.hpp>
#include <lexy/match.hpp>

TEST_CASE("dsl::delimited()")
{
    constexpr auto cp = lexy::dsl::ascii::character;

    SUBCASE("pattern")
    {
        static constexpr auto rule = delimited(LEXY_LIT("("), LEXY_LIT(")"))(cp);
        CHECK(lexy::is_rule<decltype(rule)>);
        CHECK(lexy::is_branch<decltype(rule)>);

        struct callback
        {
            const char* str;

            constexpr auto list()
            {
                struct b
                {
                    int count = 0;

                    using return_type = int;

                    constexpr void operator()(lexy::lexeme_for<test_input> lex)
                    {
                        count += int(lex.size());
                    }

                    constexpr int finish() &&
                    {
                        return count;
                    }
                };
                return b{};
            }
            constexpr int success(const char* cur, int count)
            {
                CONSTEXPR_CHECK(cur - str == count + 2);
                return count;
            }

            constexpr int error(test_error<lexy::expected_literal> e)
            {
                CONSTEXPR_CHECK(e.string() == "(");
                CONSTEXPR_CHECK(e.position() == str);
                return -1;
            }
            constexpr int error(test_error<lexy::missing_delimiter> e)
            {
                CONSTEXPR_CHECK(e.begin() == str + 1);
                CONSTEXPR_CHECK(e.end() == lexy::_detail::string_view(str).end());
                CONSTEXPR_CHECK(e.message() == "missing delimiter");
                return -2;
            }
            constexpr int error(test_error<lexy::expected_char_class> e)
            {
                CONSTEXPR_CHECK(e.character_class() == lexy::_detail::string_view("ASCII"));
                return -3;
            }
        };

        auto empty = LEXY_VERIFY("");
        CHECK(empty == -1);

        auto zero = LEXY_VERIFY("()");
        CHECK(zero == 0);
        auto one = LEXY_VERIFY("(a)");
        CHECK(one == 1);
        auto two = LEXY_VERIFY("(ab)");
        CHECK(two == 2);
        auto three = LEXY_VERIFY("(abc)");
        CHECK(three == 3);

        auto unterminated = LEXY_VERIFY("(abc");
        CHECK(unterminated == -2);

        auto invalid_ascii = LEXY_VERIFY("(ab\xFF");
        CHECK(invalid_ascii == -3);
    }
    SUBCASE("branch")
    {
        struct open
        {};
        struct close
        {};

        static constexpr auto rule = delimited(LEXY_LIT("(") >> lexy::dsl::value_t<open>,
                                               LEXY_LIT(")") >> lexy::dsl::value_t<close>)(cp);
        CHECK(lexy::is_rule<decltype(rule)>);
        CHECK(lexy::is_branch<decltype(rule)>);

        struct callback
        {
            const char* str;

            constexpr auto list()
            {
                struct b
                {
                    int count = 0;

                    using return_type = int;

                    constexpr void operator()(lexy::lexeme_for<test_input> lex)
                    {
                        count += int(lex.size());
                    }

                    constexpr int finish() &&
                    {
                        return count;
                    }
                };
                return b{};
            }
            constexpr int success(const char* cur, open, int count, close)
            {
                CONSTEXPR_CHECK(cur - str == count + 2);
                return count;
            }

            constexpr int error(test_error<lexy::expected_literal> e)
            {
                CONSTEXPR_CHECK(e.string() == "(");
                CONSTEXPR_CHECK(e.position() == str);
                return -1;
            }
            constexpr int error(test_error<lexy::missing_delimiter> e)
            {
                CONSTEXPR_CHECK(e.begin() == str + 1);
                CONSTEXPR_CHECK(e.end() == lexy::_detail::string_view(str).end());
                CONSTEXPR_CHECK(e.message() == "missing delimiter");
                return -2;
            }
            constexpr int error(test_error<lexy::expected_char_class> e)
            {
                CONSTEXPR_CHECK(e.character_class() == lexy::_detail::string_view("ASCII"));
                return -3;
            }
        };

        auto empty = LEXY_VERIFY("");
        CHECK(empty == -1);

        auto zero = LEXY_VERIFY("()");
        CHECK(zero == 0);
        auto one = LEXY_VERIFY("(a)");
        CHECK(one == 1);
        auto two = LEXY_VERIFY("(ab)");
        CHECK(two == 2);
        auto three = LEXY_VERIFY("(abc)");
        CHECK(three == 3);

        auto unterminated = LEXY_VERIFY("(abc");
        CHECK(unterminated == -2);

        auto invalid_ascii = LEXY_VERIFY("(ab\xFF");
        CHECK(invalid_ascii == -3);
    }
}

TEST_CASE("predefined dsl::delimited")
{
    constexpr auto cp = lexy::dsl::ascii::character;

    CHECK(lexy::match(lexy::zstring_input(R"("abc")"), lexy::dsl::quoted(cp) + lexy::dsl::eof));
    CHECK(lexy::match(lexy::zstring_input(R"("""abc""")"),
                      lexy::dsl::triple_quoted(cp) + lexy::dsl::eof));
    CHECK(lexy::match(lexy::zstring_input(R"('abc')"),
                      lexy::dsl::single_quoted(cp) + lexy::dsl::eof));

    CHECK(lexy::match(lexy::zstring_input("`abc`"), lexy::dsl::backticked(cp) + lexy::dsl::eof));
    CHECK(lexy::match(lexy::zstring_input("``abc``"),
                      lexy::dsl::double_backticked(cp) + lexy::dsl::eof));
    CHECK(lexy::match(lexy::zstring_input("```abc```"),
                      lexy::dsl::triple_backticked(cp) + lexy::dsl::eof));
}

TEST_CASE("dsl::delimited with escape")
{
    constexpr auto        cp = lexy::dsl::ascii::character;
    static constexpr auto rule
        = delimited(LEXY_LIT("("), LEXY_LIT(")"))(cp, lexy::dsl::escape(LEXY_LIT("$"))
                                                          .capture(lexy::dsl::ascii::character));
    CHECK(lexy::is_rule<decltype(rule)>);
    CHECK(lexy::is_branch<decltype(rule)>);

    struct callback
    {
        const char* str;

        constexpr auto list()
        {
            struct b
            {
                int count = 0;

                using return_type = int;

                constexpr void operator()(lexy::lexeme_for<test_input> lex)
                {
                    count += int(lex.size());
                }

                constexpr int finish() &&
                {
                    return count;
                }
            };
            return b{};
        }
        constexpr int success(const char* cur, int count)
        {
            CONSTEXPR_CHECK(cur[-1] == ')');
            return count;
        }

        constexpr int error(test_error<lexy::expected_literal> e)
        {
            CONSTEXPR_CHECK(e.string() == "(");
            CONSTEXPR_CHECK(e.position() == str);
            return -1;
        }
        constexpr int error(test_error<lexy::missing_delimiter> e)
        {
            CONSTEXPR_CHECK(e.begin() == str + 1);
            CONSTEXPR_CHECK(e.end() == lexy::_detail::string_view(str).end());
            CONSTEXPR_CHECK(e.message() == "missing delimiter");
            return -2;
        }
        constexpr int error(test_error<lexy::expected_char_class> e)
        {
            CONSTEXPR_CHECK(e.character_class() == "ASCII");
            return -3;
        }
        constexpr int error(test_error<lexy::invalid_escape_sequence> e)
        {
            CONSTEXPR_CHECK(e.message() == "invalid escape sequence");
            return -4;
        }
    };

    auto empty = LEXY_VERIFY("");
    CHECK(empty == -1);

    auto zero = LEXY_VERIFY("()");
    CHECK(zero == 0);
    auto one = LEXY_VERIFY("(a)");
    CHECK(one == 1);
    auto two = LEXY_VERIFY("(ab)");
    CHECK(two == 2);
    auto three = LEXY_VERIFY("(abc)");
    CHECK(three == 3);

    auto unterminated = LEXY_VERIFY("(abc");
    CHECK(unterminated == -2);

    auto invalid_ascii = LEXY_VERIFY("(ab\xFF");
    CHECK(invalid_ascii == -3);

    auto escape = LEXY_VERIFY("(a$bc$))");
    CHECK(escape == 4);
}

TEST_CASE("dsl::escape")
{
    constexpr auto escape = lexy::dsl::escape(LEXY_LIT("$"));
    SUBCASE(".rule()")
    {
        static constexpr auto rule = escape.rule(LEXY_LIT("abc") >> lexy::dsl::value_c<0>);
        CHECK(lexy::is_branch<decltype(rule)>);

        struct callback
        {
            const char* str;

            constexpr int success(const char* cur, int i)
            {
                CONSTEXPR_CHECK(cur == str + 4);
                return i;
            }

            constexpr int error(test_error<lexy::expected_literal> e)
            {
                CONSTEXPR_CHECK(e.position() == str);
                CONSTEXPR_CHECK(e.string() == "$");
                return -1;
            }
            constexpr int error(test_error<lexy::invalid_escape_sequence> e)
            {
                CONSTEXPR_CHECK(e.position() == str + 1);
                return -2;
            }
        };

        auto empty = LEXY_VERIFY("");
        CHECK(empty == -1);

        auto abc = LEXY_VERIFY("$abc");
        CHECK(abc == 0);

        auto invalid = LEXY_VERIFY("$ab");
        CHECK(invalid == -2);
    }
    SUBCASE("multiple rules")
    {
        static constexpr auto rule = escape.rule(LEXY_LIT("a") >> lexy::dsl::value_c<1>)
                                         .rule(LEXY_LIT("b") >> lexy::dsl::value_c<2>)
                                         .rule(lexy::dsl::else_ >> lexy::dsl::value_c<0>);
        CHECK(lexy::is_branch<decltype(rule)>);

        struct callback
        {
            const char* str;

            constexpr int success(const char*, int i)
            {
                return i;
            }

            constexpr int error(test_error<lexy::expected_literal> e)
            {
                CONSTEXPR_CHECK(e.position() == str);
                CONSTEXPR_CHECK(e.string() == "$");
                return -1;
            }
        };

        auto empty = LEXY_VERIFY("");
        CHECK(empty == -1);

        auto a = LEXY_VERIFY("$a");
        CHECK(a == 1);
        auto b = LEXY_VERIFY("$b");
        CHECK(b == 2);

        auto invalid = LEXY_VERIFY("$c");
        CHECK(invalid == 0);
    }
    SUBCASE(".capture()")
    {
        static constexpr auto rule = escape.capture(lexy::dsl::ascii::character);
        CHECK(lexy::is_branch<decltype(rule)>);

        struct callback
        {
            const char* str;

            constexpr int success(const char* cur, lexy::lexeme_for<test_input> lex)
            {
                CONSTEXPR_CHECK(cur == str + 2);
                return *lex.begin();
            }

            constexpr int error(test_error<lexy::expected_literal> e)
            {
                CONSTEXPR_CHECK(e.position() == str);
                CONSTEXPR_CHECK(e.string() == "$");
                return -1;
            }
            constexpr int error(test_error<lexy::invalid_escape_sequence> e)
            {
                CONSTEXPR_CHECK(e.position() == str + 1);
                return -2;
            }
        };

        auto empty = LEXY_VERIFY("");
        CHECK(empty == -1);

        auto a = LEXY_VERIFY("$a");
        CHECK(a == 'a');
        auto b = LEXY_VERIFY("$b");
        CHECK(b == 'b');

        auto invalid = LEXY_VERIFY("$\xFF");
        CHECK(invalid == -2);
    }
#if LEXY_HAS_NTTP
    SUBCASE(".lit_c()")
    {
        static constexpr auto rule = escape.lit<"a">();
        CHECK(lexy::is_branch_rule<decltype(rule)>);

        struct callback
        {
            const char* str;

            constexpr int success(const char* cur, char c)
            {
                CONSTEXPR_CHECK(cur == str + 2);
                return c;
            }

            constexpr int error(test_error<lexy::expected_literal> e)
            {
                CONSTEXPR_CHECK(e.position() == str);
                CONSTEXPR_CHECK(e.string() == "$");
                return -1;
            }
            constexpr int error(test_error<lexy::invalid_escape_sequence> e)
            {
                CONSTEXPR_CHECK(e.position() == str + 1);
                return -2;
            }
        };

        auto empty = LEXY_VERIFY("");
        CHECK(empty == -1);

        auto a = LEXY_VERIFY("$a");
        CHECK(a == 'a');

        auto invalid = LEXY_VERIFY("$b");
        CHECK(invalid == -2);
    }
#endif
    SUBCASE(".lit_c()")
    {
        static constexpr auto rule = escape.lit_c<'a'>();
        CHECK(lexy::is_branch<decltype(rule)>);

        struct callback
        {
            const char* str;

            constexpr int success(const char* cur, char c)
            {
                CONSTEXPR_CHECK(cur == str + 2);
                return c;
            }

            constexpr int error(test_error<lexy::expected_literal> e)
            {
                CONSTEXPR_CHECK(e.position() == str);
                CONSTEXPR_CHECK(e.string() == "$");
                return -1;
            }
            constexpr int error(test_error<lexy::invalid_escape_sequence> e)
            {
                CONSTEXPR_CHECK(e.position() == str + 1);
                return -2;
            }
        };

        auto empty = LEXY_VERIFY("");
        CHECK(empty == -1);

        auto a = LEXY_VERIFY("$a");
        CHECK(a == 'a');

        auto invalid = LEXY_VERIFY("$b");
        CHECK(invalid == -2);
    }
}

TEST_CASE("predefined escapes")
{
    CHECK(lexy::match(lexy::zstring_input("\\"), lexy::dsl::backslash_escape));
    CHECK(lexy::match(lexy::zstring_input("$"), lexy::dsl::dollar_escape));
}

