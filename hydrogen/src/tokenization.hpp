
// Pragma once is the same thing as doing #Ifndef, #define, #endif, which is basically to tell the compiler to only include this

#pragma once

#include <string>
#include <vector>


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
    float_lit,
    decimal,
    ptr,
    deref,



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
    // These two variables are for better error checking features
    int line;
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
        inline std::vector<Token> tokenize() {
            int line_count = 1;
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
                        tokens.push_back({.type = TokenType::exit, line_count});
                        buf.clear();
                        continue;
                    } else if (buf == "let") {
                        tokens.push_back({.type = TokenType::let, line_count});
                        buf.clear();
                        continue;
                    } else if (buf == "if") {
                        tokens.push_back({.type = TokenType::if_, line_count});
                        buf.clear();
                        continue;
                    } else if (buf == "ptr") {
                        tokens.push_back({.type = TokenType::ptr, line_count});
                        buf.clear();
                        continue;
                    } else if (buf == "elif") {
                        tokens.push_back({.type = TokenType::elif, line_count});
                        buf.clear();
                    } else if (buf == "else") {
                        tokens.push_back({.type = TokenType::else_, line_count});
                        buf.clear();
                    } else {
                        tokens.push_back({.type = TokenType::ident, line_count, .value = buf});
                        buf.clear();
                        continue;
                    }
                } else if (peek().value() == '\n') {
                    consume();
                    line_count++;
                } else if (peek().value() == '@') {
                    consume();
                    tokens.push_back({TokenType::deref, line_count});
                } else if (peek().value() == '.') {
                    // Case for a decimal that starts with '.' i.e .69
                    buf.push_back(consume());
                    while(peek().has_value() && std::isdigit(peek().value())) {
                        buf.push_back(consume());
                    }
                    tokens.push_back({TokenType::float_lit, line_count, .value = buf});
                    buf.clear();
                } else if (std::isdigit(peek().value())) {
                    // Includes case for int lit and float that starts with another number, i.e 6.69
                    buf.push_back(consume());
                    bool has_decimal = 0;
                    while (peek().has_value() && (std::isdigit(peek().value()) || peek().value() == '.')) {
                        if (peek().value() == '.') {
                            has_decimal = 1;
                        }
                        buf.push_back(consume());
                    }

                    if (has_decimal) {
                        tokens.push_back({TokenType::float_lit, line_count, .value = buf});
                        buf.clear();
                    } else {
                        tokens.push_back({.type = TokenType::int_lit , line_count, .value = buf});
                        buf.clear();
                    }
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
                    tokens.push_back({.type = TokenType::open_paran, line_count});
                } else if (peek().value() == ')') {
                    consume();
                    tokens.push_back({.type = TokenType::close_paran, line_count});
                } else if (peek().value() == ';') {
                    consume();
                    tokens.push_back({.type = TokenType::semi, line_count});
                } else if (peek().value() == '=') {
                    consume();
                    tokens.push_back({.type = TokenType::equals, line_count});
                } else if (peek().value() == '+') {
                    consume();
                    tokens.push_back({.type = TokenType::plus, line_count});
                } else if (peek().value() == '*') {
                    consume();
                    tokens.push_back({.type = TokenType::star, line_count});
                }  else if (peek().value() == '-') {
                    consume();
                    tokens.push_back({.type = TokenType::sub, line_count});
                } else if (peek().value() == '/') {
                    consume();
                    tokens.push_back({.type = TokenType::div, line_count});
                } else if (peek().value() == '{') {
                    consume();
                    tokens.push_back({.type = TokenType::open_curly, line_count});
                } else if (peek().value() == '}') {
                    consume();
                    tokens.push_back({.type = TokenType::close_curly, line_count});
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