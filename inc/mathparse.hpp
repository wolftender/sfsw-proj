#pragma once
#include <vector>
#include <string_view>

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
        dot,
        end
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
            token_t m_next();
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
    };
}