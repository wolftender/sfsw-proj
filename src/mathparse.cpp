#include <cstdlib>
#include <optional>
#include <queue>
#include <stack>
#include <cassert>
#include <string_view>
#include <stdexcept>
#include <format>
#include <ranges>
#include <string>
#include <functional>

#include "mathparse.hpp"

namespace mini {
    constexpr std::string_view VALID_CHARACTERS = "abcedfghijklmnopqrstuvwyxz0123456789/*+-(),.";
    constexpr std::string_view VALID_LETTERS = "abcedfghijklmnopqrstuvwyxz";
    constexpr std::string_view VALID_DIGITS = "0123456789";
    constexpr std::string_view EMPTY_CHARACTERS = " \n\t\r";
    constexpr std::string_view ZERO = "0";

    constexpr char TOKEN_ADD = '+';
    constexpr char TOKEN_SUB = '-';
    constexpr char TOKEN_MUL = '*';
    constexpr char TOKEN_DIV = '/';
    constexpr char TOKEN_LBR = '(';
    constexpr char TOKEN_RBR = ')';
    constexpr char TOKEN_COM = ',';
    constexpr char TOKEN_DOT = '.';

    math_lexer::math_lexer(const std::string_view& expression) : m_expression(expression) {
        m_begin = m_expression.begin();
        m_end = m_expression.end();
        m_curr = m_begin;
    }

    std::vector<token_t> math_lexer::tokenize() {
        std::vector<token_t> tokens;

        for (auto ret = m_next(); ret.has_value(); ret = m_next()) {
            auto token = ret.value();

            if (token.type == token_type_t::subtraction) {
                tokens.push_back(token_t(token_type_t::number, ZERO));
            }

            tokens.push_back(token);

            if (token.type == token_type_t::unexpected) {
                break;
            }
        }

        return tokens;
    }

    bool math_lexer::m_ch_in_set(char ch, const std::string_view& set) {
        return (set.find(ch) != std::string_view::npos);
    }

    bool math_lexer::m_is_end() {
        return m_curr == m_end;
    }

    char math_lexer::m_peek() {
        if (m_is_end()) {
            return 0;
        }

        return *m_curr;
    }

    char math_lexer::m_get() {
        if (m_is_end()) {
            return 0;
        }

        char ch = *m_curr;
        m_curr++;

        return ch;
    }

    token_t math_lexer::m_atom(token_type_t type) {
        const auto start = m_curr;
        (void)m_get();

        return token_t(type, std::string_view(&(*start), 1));
    }

    token_t math_lexer::m_identifier() {
        const auto start = m_curr;
        (void)m_get();

        while (m_ch_in_set(m_peek(), VALID_LETTERS)) {
            (void)m_get();
        }

        return token_t(token_type_t::identifier, std::string_view(&(*start), m_curr - start));
    }

    token_t math_lexer::m_number() {
        bool dot = false;

        const auto start = m_curr;
        (void)m_get();

        char ch = m_peek();
        while (ch == TOKEN_DOT || m_ch_in_set(ch, VALID_DIGITS)) {
            if (ch == TOKEN_DOT) {
                if (dot) {
                    return token_t(token_type_t::unexpected, std::string_view(&(*start), m_curr - start));
                } else {
                    dot = true;
                }
            }

            (void)m_get();
            ch = m_peek();
        }

        return token_t(token_type_t::number, std::string_view(&(*start), m_curr - start));
    }

    std::optional<token_t> math_lexer::m_next() {
        while (m_ch_in_set(m_peek(), EMPTY_CHARACTERS)) {
            (void)m_get();
        }

        const char ch = m_peek();
        switch (ch) {
        case '\0':
            return {};

        case TOKEN_ADD:
            return m_atom(token_type_t::addition);

        case TOKEN_SUB:
            return m_atom(token_type_t::subtraction);

        case TOKEN_MUL:
            return m_atom(token_type_t::multiplication);

        case TOKEN_DIV:
            return m_atom(token_type_t::division);

        case TOKEN_LBR:
            return m_atom(token_type_t::left_bracket);

        case TOKEN_RBR:
            return m_atom(token_type_t::right_bracket);

        case TOKEN_COM:
            return m_atom(token_type_t::comma);

        default:
            if (m_ch_in_set(ch, VALID_LETTERS)) {
                return m_identifier();
            } else if (m_ch_in_set(ch, VALID_DIGITS)) {
                return m_number();
            }

            return m_atom(token_type_t::unexpected);
        }
    }

    std::vector<token_t> math_parser::to_rpn() const {
        std::vector<token_t> output_queue;
        std::stack<token_t> operator_stack;

        for (const auto& token : m_tokens) {
            auto precedence = static_cast<int>(token.type);
            switch (token.type) {
            case token_type_t::number:
                output_queue.push_back(token);
                break;

            case token_type_t::identifier:
                if (token.content == "t") {
                    output_queue.push_back(token);
                } else {
                    operator_stack.push(token);
                }

                break;

            case token_type_t::left_bracket:
                operator_stack.push(token);
                break;

            case token_type_t::right_bracket:
                while (operator_stack.top().type != token_type_t::left_bracket) {
                    output_queue.push_back(operator_stack.top());
                    operator_stack.pop();

                    if (operator_stack.empty()) {
                        throw std::runtime_error("mismatched brackets");
                        return {};
                    }
                }

                operator_stack.pop();
                break;

            case token_type_t::comma:
                if (operator_stack.empty()) {
                    throw std::runtime_error("unexpected comma");
                }

                while (operator_stack.top().type != token_type_t::left_bracket) {
                    output_queue.push_back(operator_stack.top());
                    operator_stack.pop();

                    if (operator_stack.empty()) {
                        throw std::runtime_error("unexpected comma");
                    }
                }

                break;

            case token_type_t::addition:
            case token_type_t::subtraction:
            case token_type_t::multiplication:
            case token_type_t::division:
            case token_type_t::exponentiation:
                while (!operator_stack.empty()) {
                    auto& top = operator_stack.top();

                    if (top.type == token_type_t::left_bracket) {
                        break;
                    }

                    if (static_cast<int>(top.type) < precedence) {
                        break;
                    }

                    output_queue.push_back(top);
                    operator_stack.pop();
                }

                operator_stack.push(token);
                break;

            default:
                throw std::runtime_error(std::format("unexpected token {}", token.content));
            }
        }

        while (!operator_stack.empty()) {
            auto& top = operator_stack.top();
            switch (top.type) {
            case token_type_t::identifier:
            case token_type_t::addition:
            case token_type_t::subtraction:
            case token_type_t::multiplication:
            case token_type_t::division:
            case token_type_t::exponentiation:
                output_queue.push_back(top);
                break;

            default:
                throw std::runtime_error(std::format("unexpected token {}", top.content));
            }

            operator_stack.pop();
        }

        return output_queue;
    }

    f_func math_parser::parse() const {
        auto stack = to_rpn() | std::views::reverse;
        auto top = stack.begin();

        auto fn = m_parse(stack, top);

        if (top != stack.end()) {
            throw std::runtime_error(std::format("unexpected token {}", top->content));
        }

        return fn;
    }
}