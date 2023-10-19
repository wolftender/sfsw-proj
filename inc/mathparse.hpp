#pragma once
#include <vector>
#include <string_view>
#include <format>
#include <ranges>
#include <optional>

#include "function.hpp"

namespace mini {
    enum class token_type_t {
        unexpected,
        number,
        identifier,
        addition,
        subtraction,
        multiplication,
        division,
        exponentiation,
        comma,
        left_bracket,
        right_bracket,
        dot
    };

    struct token_t {
        token_type_t type;
        std::string_view content;

        token_t(token_type_t type, std::string_view content) :
            type(type),
            content(content) {}
    };

    class math_lexer final {
        private:
            const std::string_view& m_expression;
            std::string_view::const_iterator m_begin, m_end, m_curr;

        public:
            math_lexer(const std::string_view& expression);

            ~math_lexer() = default;
            math_lexer(const math_lexer&) = delete;
            math_lexer& operator=(const math_lexer&) = delete;

            std::vector<token_t> tokenize();

        private:
            bool m_ch_in_set(char ch, const std::string_view& set);
            bool m_is_end();

            char m_peek();
            char m_get();

            token_t m_atom(token_type_t type);
            token_t m_identifier();
            token_t m_number();
            std::optional<token_t> m_next();
    };

    class math_parser final {
    private:
        std::vector<token_t> m_tokens;

    public:
        math_parser(const std::vector<token_t>& tokens) {
            m_tokens = tokens;
        }

        ~math_parser() = default;
        math_parser(const math_parser&) = delete;
        math_parser& operator=(const math_parser&) = delete;

        std::vector<token_t> to_rpn() const;
        f_func parse() const;

    private:
        template<std::ranges::forward_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, token_t>
        f_func m_parse(const Range& stack, std::ranges::iterator_t<Range> & top) const {
            // pop token from stack
            token_t token = *top;
            std::advance(top, 1);

            switch (token.type) {
                case token_type_t::number:
                    return mk_const(std::stof(std::string(token.content)));

                case token_type_t::identifier: {
                    if (token.content == "t") {
                        return mk_lin(1.0f);
                    } else if (token.content == "sin") {
                        auto arg1 = m_parse(stack, top);
                        return mk_comp(mk_sin(), std::move(arg1));
                    } else if (token.content == "cos") {
                        auto arg1 = m_parse(stack, top);
                        return mk_comp(mk_cos(), std::move(arg1));
                    } else if (token.content == "exp") {
                        auto arg1 = m_parse(stack, top);
                        return mk_comp(mk_exp(), std::move(arg1));
                    }
                }

                break;

                case token_type_t::addition: {
                    auto arg2 = m_parse(stack, top);
                    auto arg1 = m_parse(stack, top);

                    return mk_sum(std::move(arg1), std::move(arg2));
                }

                case token_type_t::subtraction: {
                    auto arg2 = m_parse(stack, top);
                    auto arg1 = m_parse(stack, top);

                    return mk_sub(std::move(arg1), std::move(arg2));
                }

                case token_type_t::multiplication: {
                    auto arg2 = m_parse(stack, top);
                    auto arg1 = m_parse(stack, top);

                    return mk_mul(std::move(arg1), std::move(arg2));
                }

                case token_type_t::division: {
                    auto arg2 = m_parse(stack, top);
                    auto arg1 = m_parse(stack, top);

                    return mk_frac(std::move(arg1), std::move(arg2));
                }

                default:
                    break;
            }

            throw std::runtime_error(std::format("invalid expression, token: {}", token.content));
        }
    };
}