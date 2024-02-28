//
// Created by prabbyd on 1/31/24.
// This file crates a tree like structure of our expressions that makes it easier to parse the code and make assembly from it
// Similar to tokenizer class except we peek and consume token by token instead of char by char

#pragma once

#include <variant>
#include "arena.hpp"
#include "tokenization.hpp"

struct NodeTermIntLit {
    Token int_lit;
};

// An identifier is like the x in "let x = 7"
struct NodeTermIdent {
     Token ident;
};

// Define here so C++ compiler knows our intent for BinExprAdd/Mult
struct NodeExpr;

struct NodeBinExprAdd{
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprMulti{
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprSub{
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprDiv{
    NodeExpr* lhs;
    NodeExpr* rhs;
};

// NOde for expressions that let us add or multiply numbers in the correct order of operations
struct NodeBinExpr {
    std::variant<NodeBinExprAdd*, NodeBinExprMulti*, NodeBinExprSub*, NodeBinExprDiv*> var;
};

// Node for a paranthesised expression, like (10 + 1) / 11
struct NodeTermParan {
    NodeExpr* expr;
};


// Expressions fit either terms or NodeBinExpr, and a term fits either int_lit or identifier
struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParan*> var;
};

// Node for expressions, like integers, strings, etc
struct NodeExpr {
    // variant is a feature that lets a variable hold multiple different types of data
    std::variant<NodeTerm*, NodeBinExpr*> var;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

// Forward declare because stmts can have scopes, which have lists of statements
struct NodeStmt;

struct NodeScope {
    std::vector<NodeStmt*> stmts;
};

struct NodeStmtIf {
    NodeExpr* expr;
    NodeScope* scope;
};

// Node representing statements, like setting variables with let, exit statements, etc..
struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtLet*, NodeScope*, NodeStmtIf*> var;
};

// A node representing the program as a list of statements to parse
struct NodeProg {
    std::vector<NodeStmt*> stmts;
};

// This parsing works very similarly to tokenizer, peek while we have tokens to peek.
class Parser {
public:
    inline explicit Parser(std::vector<Token>& tokens)
            : m_tokens(std::move(tokens)),
              m_allocator(1024 * 1024 * 4) // 4MB
    {}

    // Parse a scope
    std::optional<NodeScope*> parse_scope() {
        if (!try_consume(TokenType::open_curly)) {
            return {};
        }
        auto scope = m_allocator.alloc<NodeScope>();
        while (auto stmt = parse_stmt()) {
            scope->stmts.push_back(stmt.value());
        }
        try_consume(TokenType::close_curly, "Expected '}'");
        return scope;
    }

    // Parse terms, which are identifiers or int lits
    std::optional<NodeTerm*> parse_term() {
        if (auto int_lit = try_consume(TokenType::int_lit)) {
            auto term_int_lit = m_allocator.alloc<NodeTermIntLit>();
            term_int_lit->int_lit = int_lit.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_int_lit;
            return term;
        } else if (auto ident = try_consume(TokenType::ident)) {
            auto term_ident = m_allocator.alloc<NodeTermIdent>();
            term_ident->ident = ident.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_ident;
            return term;
        } else if (auto paran = try_consume(TokenType::open_paran)) {
            // This case is for if we encounter parans used for containing expressions of highest precendence
            auto term_paran = m_allocator.alloc<NodeTermParan>();
            auto expr = parse_expr();
            if (!expr.has_value()) {
                std::cerr << "Invalid operation between paranthesis!" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paran, "Expected close paranthesis");
            term_paran->expr = expr.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_paran;
            return term;
        } else {
            return {};
        }
    }

    // Function to parse an expression, which are things like terms or binary expressions
    std::optional<NodeExpr*> parse_expr(int min_prec = 0) {

        /* Begin implementation of precedence climbing*/
        std::optional<NodeTerm*> term_lhs = parse_term();
        // Doing this bc add->lhs or mult->lhs is an expr, and we currently have it as a term, so we need to transfer
        auto expr_lhs = m_allocator.alloc<NodeExpr>();
        expr_lhs->var = term_lhs.value();
        if (!term_lhs.has_value()) {
            return {};
        }

        while(1) {
            std::optional<Token> curr_tok = peek();
            std::optional<int> prec;
            // If token does exist, but either wrong precedence
            if (curr_tok.has_value()) {
                prec = bin_prec(curr_tok->type);
                if (!prec.has_value() || prec < min_prec) {
                    break;
                }
            } else {
                // Token does not have value
                break;
            }

            Token op = consume();
            // Get rhs with additional precedence
            int next_min_prec = prec.value() + 1;
            auto expr_rhs = parse_expr(next_min_prec);

            if (!expr_rhs.has_value()) {
                std::cerr << "Unable to parse expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            // We know current token is binary operator, so we can consume and do recursive call
            auto expr = m_allocator.alloc<NodeBinExpr>();
            auto expr_lhs2 = m_allocator.alloc<NodeExpr>();
            if (op.type == TokenType::plus) {
                auto add = m_allocator.alloc<NodeBinExprAdd>();
                expr_lhs2->var = expr_lhs->var;
                add->lhs = expr_lhs2;
                add->rhs = expr_rhs.value();
                expr->var = add;
            } else if (op.type == TokenType::star) {
                auto star = m_allocator.alloc<NodeBinExprMulti>();
                expr_lhs2->var = expr_lhs->var;
                star->lhs = expr_lhs2;
                star->rhs = expr_rhs.value();
                expr->var = star;
            } else if (op.type == TokenType::sub) {
                auto sub = m_allocator.alloc<NodeBinExprSub>();
                expr_lhs2->var = expr_lhs->var;
                sub->lhs = expr_lhs2;
                sub->rhs = expr_rhs.value();
                expr->var = sub;
            } else if (op.type == TokenType::div) {
                auto div = m_allocator.alloc<NodeBinExprDiv>();
                expr_lhs2->var = expr_lhs->var;
                div->lhs = expr_lhs2;
                div->rhs = expr_rhs.value();
                expr->var = div;
            }
            // Reset expr lhs
            expr_lhs->var = expr;
        }
        return expr_lhs;
        /* ENd implmentation of precedence climbing */

    }


        // Function to parse statements, such as functions like let, or exits, etc.
        std::optional<NodeStmt*> parse_stmt() {
            // Exit statment case
            if (peek().value().type == TokenType::exit && peek(1).has_value() &&
                peek(1).value().type == TokenType::open_paran) {
                // Make NodeStmtExit* in arena allocator
                auto node_stmt_exit = m_allocator.alloc<NodeStmtExit>();

                // Consume exit token and open paranthesis token
                consume();
                consume();

                // We have an exit type, now check inside of exit for valid expression to evaluate
                // node_expr will be true if parse_expr returned something, false o/w
                if (auto node_expr = parse_expr()) {
                    // if there was an expression to parse, we give this exit node the expression to parse as well before returning it in main.
                    node_stmt_exit->expr = node_expr.value();
                } else {
                    std::cerr << "Invalid Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                try_consume(TokenType::close_paran, "Expected ')'");
                try_consume(TokenType::semi, "Expected ';'");

                // Make return statement
                auto node_stmt_return = m_allocator.alloc<NodeStmt>();
                node_stmt_return->var = node_stmt_exit;
                return node_stmt_return;

            // "let" statment case for setting variables
            } else if (peek().value().type == TokenType::let && peek(1).has_value() &&
            peek(1).value().type == TokenType::ident && peek(2).has_value() && peek(2).value().type == TokenType::equals) {
                // Consume let token
                consume();

                // Make NodeStmtLet*
                auto node_stmt_let = m_allocator.alloc<NodeStmtLet>();
                node_stmt_let->ident = consume();

                // Get rid of equals token
                consume();

                // Parse expression after equals, c
                if (auto expr = parse_expr()) {
                    node_stmt_let->expr = expr.value();
                } else {
                    std::cerr << "Invalid expression given to let!" << std::endl;
                    exit(EXIT_FAILURE);
                }
                // Check for ending semicolon
                try_consume(TokenType::semi, "Expected ';'");
                auto node_stmt_return = m_allocator.alloc<NodeStmt>();
                node_stmt_return->var = node_stmt_let;
                return node_stmt_return;

            // Scopes
            } else if (peek().has_value() && peek().value().type == TokenType::open_curly) {
                if (auto scope = parse_scope()) {
                    auto stmt = m_allocator.alloc<NodeStmt>();
                    stmt->var = scope.value();
                    return stmt;
                } else {
                    std::cerr << "Invalid Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }

            // If statements
            } else if (auto if_ = try_consume(TokenType::if_)){
                try_consume(TokenType::open_paran, "Expected '(");
                auto stmt_if = m_allocator.alloc<NodeStmtIf>();
                if (auto expr = parse_expr()) {
                    stmt_if->expr = expr.value();
                } else {
                    std::cerr << "Invalid Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                try_consume(TokenType::close_paran, "Expected')");
                if (auto scope = parse_scope()) {
                    stmt_if->scope = scope.value();
                } else {
                    std::cerr << "Expected a scope!" << std::endl;
                    exit(EXIT_FAILURE);
                }
                auto stmt = m_allocator.alloc<NodeStmt>();
                stmt->var = stmt_if;
                return stmt;
            } else {
                return {};
            }
        }

        // Function that parses an entire program, token by token
        std::optional<NodeProg*> parse_prog() {
            // Given a list of tokens, so we have to keep going until tno more tokens
            auto node_prog = m_allocator.alloc<NodeProg>();
            while (peek().has_value()) {
                if (auto stmt = parse_stmt()) {
                    node_prog->stmts.push_back(stmt.value());
                } else {
                    std::cerr << "Invalid Statement" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            return node_prog;
        }
    private:
        inline std::optional<Token> peek(int offset = 0) const {
            if (m_index + offset >= m_tokens.size()) {
                // OOB
                return {};
            } else {
                return m_tokens[m_index + offset];
            }
        }

        inline Token try_consume(TokenType type, const std::string& err_msg) {
            if (peek().has_value() && peek().value().type == type) {
                return consume();
            } else {
                std::cerr << err_msg << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        inline std::optional<Token> try_consume(TokenType type) {
            if (peek().has_value() && peek().value().type == type) {
                return consume();
            } else {
                return {};
            }
        }

        inline Token consume() {
            // returns value @m_index, then increments
            return m_tokens[m_index++];
        }

        const std::vector<Token> m_tokens;
        size_t m_index = 0;
        ArenaAllocator m_allocator;

};
