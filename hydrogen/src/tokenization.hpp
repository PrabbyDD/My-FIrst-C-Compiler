//
// Created by prabbyd on 1/31/24.
// Pragma once is the same thing as doing #Ifndef, #define, #endif, which is basically to tell the compiler to only include this
// file once in the source code, else there could be compilation errors with multiple inclusions
// Header file typically used to define functions (you can implement small ones, like adding, with inline)
// YOu can use pragma or #ifndef in header files , also good to decalre constants here

#pragma once

#include <string>
#include <vector>

// Enum for the type of token something is
enum class TokenType{
    exit,
    int_lit,
    semi,
    open_paran,
    close_paran,
    ident,
    let,
    equals,
    plus,
    star,
    sub,
    div,
    open_curly,
    close_curly,
    if_,
    elif,
    else_,

};

// 2 functions in one, return precedence level of operator, and tell if token is bin operator
std::optional<int> bin_prec(TokenType type) {
    switch (type) {
        case TokenType::plus:
            return 0;
        case TokenType::sub:
            return 0;
        case TokenType::div:
            return 1;
        case TokenType::star:
            return 1;
        default:
            return {};
    }
}

// Struct to define a token
struct Token {
    TokenType type;
    // optional<> is a wrapper (like smart pointer) that represents a val that may or may not be there
    // this allows us to handle absence of values in a more safe way rather than using sentinels or nulls
    std::optional<std::string> value {};
};

class Tokenizer {
    public:
        inline explicit Tokenizer(std::string src)
            : m_src(std::move(src))
        {
        }

        // Turn the m_src string into a list of tokens.
        // We implement the function here in header because it will often be reused  and is simple.
        inline std::vector<Token> tokenize() {
            std::string buf;
            std::vector<Token> tokens;
            while (peek().has_value()) {
                // Case for keywords
                if (std::isalpha(peek().value())) {
                    buf.push_back(consume());
                    while (peek().has_value() && std::isalnum(peek().value())) {
                        buf.push_back(consume());
                    }
                    if (buf == "exit") {
                        tokens.push_back({.type = TokenType::exit});
                        buf.clear();
                        continue;
                    } else if (buf == "let") {
                        tokens.push_back({.type = TokenType::let});
                        buf.clear();
                        continue;
                    } else if (buf == "if") {
                        tokens.push_back({.type = TokenType::if_});
                        buf.clear();
                        continue;
                    } else if (buf == "elif") {
                        tokens.push_back({.type = TokenType::elif});
                        buf.clear();
                    } else if (buf == "else") {
                        tokens.push_back({.type = TokenType::else_});
                        buf.clear();
                    } else {
                        tokens.push_back({.type = TokenType::ident, .value = buf});
                        buf.clear();
                        continue;
                    }
                } else if (std::isdigit(peek().value())) {
                    buf.push_back(consume());
                    while (peek().has_value() && std::isdigit(peek().value())) {
                        buf.push_back(consume());
                    }
                    tokens.push_back({.type = TokenType::int_lit ,.value = buf});
                    buf.clear();
                } else if (std::isspace(peek().value())) {
                    consume();
                } else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/') {
                    consume();
                    consume();

                    // Keep going until we encounter a new line, but dont need to consume newline bc isspace does that
                    while (peek().has_value() && peek().value() != '\n') {
                        consume();
                    }
                } else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*') {
                    consume();
                    consume();

                    while (peek().has_value()) {
                        if (peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/') {
                            break;
                        }
                        consume();
                    }
                    if (peek().has_value()) {
                        consume();
                    }
                    if (peek().has_value()) {
                        consume();
                    }
                } else if (peek().value() == '(') {
                    consume();
                    tokens.push_back({.type = TokenType::open_paran});
                } else if (peek().value() == ')') {
                    consume();
                    tokens.push_back({.type = TokenType::close_paran});
                } else if (peek().value() == ';') {
                    consume();
                    tokens.push_back({.type = TokenType::semi});
                } else if (peek().value() == '=') {
                    consume();
                    tokens.push_back({.type = TokenType::equals});
                } else if (peek().value() == '+') {
                    consume();
                    tokens.push_back({.type = TokenType::plus});
                } else if (peek().value() == '*') {
                    consume();
                    tokens.push_back({.type = TokenType::star});
                }  else if (peek().value() == '-') {
                    consume();
                    tokens.push_back({.type = TokenType::sub});
                } else if (peek().value() == '/') {
                    consume();
                    tokens.push_back({.type = TokenType::div});
                } else if (peek().value() == '{') {
                    consume();
                    tokens.push_back({.type = TokenType::open_curly});
                } else if (peek().value() == '}') {
                    consume();
                    tokens.push_back({.type = TokenType::close_curly});
                } else {
                    std::cerr << "Invalid Token!" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            m_index = 0;
            return tokens;
        }

    private:
        // Const values need to be used o/w compiler throws error because if we cant modify it, we are using its return value somewhere, o/w function doesnt do anything
        // This function peeks ahead a certain number of characters in a string to see if it exists and what it is.
        inline std::optional<char> peek(int offset = 0) const {
            if (m_index + offset >= m_src.length()) {
                // OOB, return {}, which returns default instance of that variable, in this case empty string. If func was returning ptr, it would return nullptr
                return {};
            } else {
                return m_src[m_index + offset];
            }
        }

        // Return the current char in string, and increment counter to next char.
        inline char consume() {
            // returns value @m_index, then increments
            return m_src[m_index++];
        }

        std::string m_src;
        size_t m_index = 0;
};