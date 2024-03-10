
#pragma once

#include "parser.hpp"
#include <cassert>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <bitset>
#include <optional>

class Generator {
    public:
        inline explicit Generator(NodeProg* prog)
            : m_prog(std::move(prog))
        {}

        void gen_term(const NodeTerm* term) {
            struct TermVisitor {
                // Changed gen from pointer to reference because refs cannot be null, ptrs can, this gives us good null checking
                Generator& gen;

                // This triggers anytime we need to use the value of an identifier
                void operator()(const NodeTermIdent* term_ident) const {
                    const auto it = std::find_if(
                            gen.m_vars.cbegin(),
                            gen.m_vars.cend(),
                            [&](const Var& var) { return var.name == term_ident->ident.value.value();
                            });
                    if (it == gen.m_vars.cend()) {
                        std::cerr << "Undeclared identifier: " << term_ident->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    std::stringstream offset;
                    // Taking value from further down in stack @ stack_loc and then making a copy of it and pushing it to top of stack so that it can be used
                    offset << "QWORD [rsp + " << (gen.m_stack_size - it->stack_loc - 1) * 8 << "]";
                    gen.push(offset.str());
                }
                // Move value into register, push from register to stack
                void operator()(const NodeTermIntLit* term_int_lit) const {
                    gen.m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                    gen.push("rax");
                }
                // Move value into sse reg, then to stack
                void operator()(const NodeTermFloatLit* term_float_lit) const {
                    // Need to convert float to hex to use the proper instruction
                    std::string hexFloat = gen.floatStringToHex(term_float_lit->float_lit.value.value());
                    gen.m_output << "    mov rcx, 0x" << hexFloat << "\n";
                    gen.m_output << "    movq xmm0, rcx" << "\n";
                    gen.push_float("xmm0");
                }

                void operator()(const NodeTermParan* paran) const {
                    gen.gen_expr(paran->expr);
                }
            };

            // Remember that "this" is a keyword that refers to the current object instance.
            // "this" is a pointer to the current object, *this dereferences it and allows us to use it
            // In this case "this" is Generator*
            TermVisitor visitor({.gen = *this});
            std::visit(visitor, term->var);
        }

        void gen_bin_expr(const NodeBinExpr* bin_expr) {
            struct BinExprVisitor {
                Generator& gen;
                void operator()(const NodeBinExprAdd* add) const {
                    gen.m_output << "    ;; / begin addition\n";

                    // Assembly for adding is to load values into 2 diff regs, then add
                    gen.gen_expr(add->rhs);
                    gen.gen_expr(add->lhs);

                    if (add->lhs->int_or_float == TokenType::float_lit || add->rhs->int_or_float == TokenType::float_lit) {
                        gen.pop_float("xmm0");
                        gen.pop_float("xmm1");
                        gen.m_output << "    addss xmm0, xmm1\n";
                        gen.push_float("xmm0");
                    } else {
                        //To add, pop off top of stack into registers, doesnt matter which way we do, add is commutative
                        gen.pop("rax");
                        gen.pop("rbx");
                        gen.m_output << "    add rax, rbx\n";

                        // put result back on stack, i think rax gets overwritten with new val
                        gen.push("rax");
                    }

                    gen.m_output << "    ;; / end addition\n";
                }

                // Similarly done for multiplciation
                void operator()(const NodeBinExprMulti* mult) const {
                    gen.m_output << "    ;; / begin multiplication\n";
                    gen.gen_expr(mult->rhs);
                    gen.gen_expr(mult->lhs);
                    if (mult->lhs->int_or_float == TokenType::float_lit || mult->rhs->int_or_float == TokenType::float_lit) {
                        gen.pop_float("xmm0");
                        gen.pop_float("xmm1");
                        gen.m_output << "    mulss xmm0, xmm1\n";
                        gen.push_float("xmm0");

                    } else {
                        gen.pop("rax");
                        gen.pop("rbx");
                        gen.m_output << "    mul rbx\n";

                        gen.push("rax");
                    }

                    gen.m_output << "    ;; / end multiplication\n";
                }

                void operator()(const NodeBinExprSub* sub) const {
                    gen.m_output << "    ;; / begin subtraction\n";
                    gen.gen_expr(sub->rhs);
                    gen.gen_expr(sub->lhs);

                    if (sub->lhs->int_or_float == TokenType::float_lit || sub->rhs->int_or_float == TokenType::float_lit) {
                        gen.pop_float("xmm0");

                        gen.pop_float("xmm1");
                        gen.m_output << "    subss xmm0, xmm1\n";
                        gen.push_float("xmm0");

                    } else {
                        gen.pop("rax");
                        gen.pop("rbx");
                        gen.m_output << "    sub rax, rbx\n";
                        gen.push("rax");
                    }

                    gen.m_output << "    ;; / end subtraction\n";
                }

                void operator()(const NodeBinExprDiv* div) const {
                    gen.m_output << "    ;; / begin division\n";
                    gen.gen_expr(div->rhs);
                    gen.gen_expr(div->lhs);

                    if (div->lhs->int_or_float == TokenType::float_lit || div->rhs->int_or_float == TokenType::float_lit) {

                        gen.pop_float("xmm0");
                        gen.pop_float("xmm1");
                        gen.m_output << "    divss xmm0, xmm1\n";
                        gen.push_float("xmm0");

                    } else {

                        gen.pop("rax");
                        gen.pop("rbx");
                        gen.m_output << "    div rbx\n";

                        gen.push("rax");
                    }

                    gen.m_output << "    ;; / end division\n";

                }
            };

            BinExprVisitor visitor{.gen = *this};
            std::visit(visitor, bin_expr->var);
        }

        void gen_scope(const NodeScope* scope) {
            begin_scope();
            // Parse the list of statements specific to this scope and generate their code
            for (const NodeStmt* stmt: scope->stmts) {
                gen_stmt(stmt);
            }
            end_scope();
        }

        void gen_expr(const NodeExpr* expr)  {
            struct ExprVisitor {
                Generator& gen;
                void operator()(const NodeTerm* term) const {
                    gen.gen_term(term);
                }
                void operator()(const NodeBinExpr* expr_bin) const {
                   gen.gen_bin_expr(expr_bin);
                }
            };

            ExprVisitor visitor{.gen = *this};
            std::visit(visitor, expr->var);
        }

        void gen_if_pred(const NodeIfPred* pred, const std::string& end_label) {
            struct PredVisitor {
                Generator& gen;
                const std::string& end_label_store;
                void operator ()(const NodeIfPredElif* pred_elif) {
                    gen.m_output << "    ;; / begin elif\n";
                    gen.gen_expr(pred_elif->expr);

                    gen.pop("rax");

                    std::string label = gen.create_label();

                    gen.m_output << "    test rax, rax\n";
                    gen.m_output << "    jz " << label << "\n";
                    gen.gen_scope(pred_elif->scope);
                    // As soon as one of the elifs resolves, then dont check anything else and jump to endif
                    gen.m_output << "    jmp " << end_label_store << '\n';
                    // This is an elif, so we can have infinite elifs. Need to check if has value
                    if (pred_elif->pred.has_value()) {
                        gen.m_output << label << ":\n";
                        gen.gen_if_pred(pred_elif->pred.value(), end_label_store);
                    }
                    gen.m_output << "    ;; / end elif\n";
                }
                void operator ()(const NodeIfPredElse* pred_else) {
                    gen.m_output << "    ;; / begin else\n";
                    gen.gen_scope(pred_else->scope);
                    gen.m_output << "    ;; / end else\n";
                }
            };

            PredVisitor visitor{.gen = *this, .end_label_store = end_label};
            std::visit(visitor, pred->var);
        }
        void gen_stmt(const NodeStmt* stmt)  {

            /*
             * Typically we will have statement(expression), like exit(60). So we eval expression first in
             * gen_expr, then push that value onto top of stack in assembly, and now in this function
             * we pop it off top of stack and evaluate the statement with it
             */
            struct StmtVisitor {
                Generator& gen;
                void operator ()(const NodeStmtExit* stmt_exit) {
                    gen.gen_expr(stmt_exit->expr);

                    // Move code 60 telling program to exit
                    gen.m_output << "    mov rax, 60\n";

                    // Pop expression eval from rdi and evaluate syscall
                    gen.pop("rdi");
                    gen.m_output << "    syscall\n";
                }

                void operator ()(const NodeStmtLet* stmt_let) {

                    // Code to find if identifier already in vector of identifiers
                    auto it = std::find_if(
                            gen.m_vars.cbegin(),
                            gen.m_vars.cend(),
                            [&](const Var& var) { return var.name == stmt_let->ident.value.value();
                            });
                    if (it != gen.m_vars.cend()) {
                        // If we already initialized a variable with same identifier name
                        // ex. trying to init let x = 7 and let x = 8 after is wrong
                        std::cerr << "Identifier already initialized! " << stmt_let->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    // Insert into vector, optionally its int or float type
                    gen.m_vars.push_back({.stack_loc = gen.m_stack_size, .name = stmt_let->ident.value.value(), .int_or_float = stmt_let->int_or_float});

                    // Evaluate expression, variable could potentially be let y = x, so we need to evaluate x or get it
                    // Now value of expression is at top of the stack
                    gen.gen_expr(stmt_let->expr);
                    gen.m_output <<"    ;; /let\n";
                }
                void operator ()(const NodeStmtPtr* stmt_ptr) {

                    // Code to find if identifier1 already in vector of identifiers
                    auto it = std::find_if(
                            gen.m_vars.cbegin(),
                            gen.m_vars.cend(),
                            [&](const Var& var) { return var.name == stmt_ptr->ident1.value.value();
                            });
                    if (it != gen.m_vars.cend()) {
                        // If we already initialized a variable with same identifier name
                        std::cerr << "Identifier already initialized! " << stmt_ptr->ident1.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    // Code to find if identifier2 is NOT in the vector, it should be in vector
                    auto it2 = std::find_if(
                            gen.m_vars.cbegin(),
                            gen.m_vars.cend(),
                            [&](const Var& var) { return var.name == stmt_ptr->ident2.value.value();
                            });
                    if (it2 == gen.m_vars.cend()) {
                        // If we already initialized a variable with same identifier name
                        std::cerr << "Identifier not initialized! " << stmt_ptr->ident2.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }


                    // Make new Var to give us info about where it is on stack and what identifier it is
                    gen.m_vars.push_back({.stack_loc = gen.m_stack_size, .name = stmt_ptr->ident1.value.value()});

                    // Retrieve ident2 address and then push it onto stack, pop off in deref
                    // load address into rbx
                    for (auto& j: gen.m_vars) {
                        std::cout << j.name << " located at " << j.stack_loc << "\n";
                        std::cout << it.base()->name << " located at " << it->stack_loc << "\n";
                        std::cout << it2->name << " located at " << it2->stack_loc << "\n";
                    }

                    gen.m_output << "    lea dword rbx, [rsp + " << (gen.m_stack_size - it2->stack_loc - 1) * 8 << "]\n";
                    gen.push("rbx");
                }
                void operator ()(const NodeStmtAssign* stmt_assign) {

                    // ID should already be in vector, if not throw an error
                    auto it = std::find_if(
                            gen.m_vars.cbegin(),
                            gen.m_vars.cend(),
                            [&](const Var& var) { return var.name == stmt_assign->ident.value.value();
                            });
                    if (it != gen.m_vars.cend()) {
                        // Recall this function generates assembly that puts result of this expr on top of stack

                        // Pop off top of stack into rax (int) or xmm0 (float), then back into memory
                        if (it->int_or_float == TokenType::int_lit) {
                            gen.gen_expr(stmt_assign->expr);
                            gen.pop("rax");
                            gen.m_output << "    mov [rsp + " << (gen.m_stack_size - it->stack_loc - 1) * 8 << "], rax\n";
                        } else if (it->int_or_float == TokenType::float_lit) {
                            gen.gen_expr(stmt_assign->expr);
                            gen.pop_float("xmm0");
                            gen.m_output << "    movq [rsp + " << (gen.m_stack_size - it->stack_loc - 1) * 8 << "], xmm0\n";
                        } else {
                            // Code to find if deref ident
                            auto it2 = std::find_if(
                                    gen.m_vars.cbegin(),
                                    gen.m_vars.cend(),
                                    [&](const Var& var) { return var.name == stmt_assign->deref_ident.value.value();
                                    });
                            if (it2 != gen.m_vars.cend()) {
                                // If we already initialized a variable with same identifier name
                                // ex. trying to init let x = 7 and let x = 8 after is wrong
                                std::cerr << "Identifier already initialized! " << stmt_assign->deref_ident.value.value() << std::endl;
                                exit(EXIT_FAILURE);
                            }

                            // Push address from deref ident into rbx, then value into rax, then at stack location of id1
                            gen.m_output << "    mov rbx, [rsp + " << (gen.m_stack_size - it2->stack_loc - 1) * 8 << "]\n";
                            gen.m_output << "    mov rax, [rbx]\n";
                            gen.m_output << "    mov [rsp + " << (gen.m_stack_size - it->stack_loc - 1) * 8 << "], rax\n";
                        }
                    } else {
                        std::cerr << "Identifier not initialized: " << stmt_assign->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }

                }
                void operator ()(const NodeScope* stmt_scope) {
                    gen.gen_scope(stmt_scope);
                }

                void operator ()(const NodeStmtIf* stmt_if) {
                    // Puts result of expression on top of stack
                    gen.gen_expr(stmt_if->expr);

                    // Pop off the top into rax, result of expression in rax
                    gen.pop("rax");

                    // No types, so no bools, so if result is anything other than 0 its true, aka jump to a label
                    std::string label = gen.create_label();

                    // Generate assembly for jump statement
                    gen.m_output << "    test rax, rax\n";
                    gen.m_output << "    jz " << label << "\n";
                    gen.gen_scope(stmt_if->scope);
                    if (stmt_if->pred.has_value()) {
                        std::string end_label = gen.create_label();
                        gen.m_output << "    jmp " << end_label << "\n";
                        gen.m_output << label << ":\n";
                        gen.gen_if_pred(stmt_if->pred.value(), end_label);
                        // End label is the label that skips over everything once if elif else resolves
                        gen.m_output << end_label<< ":\n";
                    } else {
                        gen.m_output << label << ":\n";
                    }
                    gen.m_output << "    ;; /if\n";
                }
            };
            StmtVisitor visitor{.gen = *this};
            std::visit(visitor, stmt->var);
        }

        // Generate the program based on the abstract syntax tree its made up from
        std::string gen_prog()  {

            m_output << "global _start\n_start:\n";
            for (const NodeStmt* stmt: m_prog->stmts) {
                gen_stmt(stmt);
            }

            m_output << "    mov rax, 60\n";
            m_output << "    mov rdi, 0\n";
            m_output << "    syscall";
            return m_output.str();
        }
    private:
        void begin_scope() {
            m_scopes.push_back(m_vars.size());
        }
        void end_scope() {
            // Pop off variables until we get to last begin_scope() scope, because they could be nested scopes
            size_t pop_count = m_vars.size() - m_scopes.back();

            // Move stack pointer in assembly back to where this scope began
            // Stack grows from top so we add not subtract when we want to remove these elements
            // Each object is 8 bytes, mult by 8
            m_output << "    add rsp, " << pop_count * 8 << "\n";
            m_stack_size -= pop_count;

            // Remove variables associated with scope from m_vars
            for (int i = 0; i < pop_count; i++) {
                m_vars.pop_back();
            }
            m_scopes.pop_back();
        }

        void push(const std::string& reg) {
            m_output << "    push " << reg << "\n";
            m_stack_size++;
        }

        void pop(const std::string& reg) {
            m_output << "    pop " << reg << "\n";
            m_stack_size--;
        }

        // Since no actual push or pop command for sse registers, gonna have to do this manually
        void push_float(const std::string& reg) {
            m_output << "    sub rsp, 8" << "\n";
            m_output << "    movq qword [rsp], "<< reg <<  "\n";
            m_stack_size++;
        }

        void pop_float(const std::string& reg) {
            m_output << "    movq " << reg << ", QWORD [rsp]" << "\n";
            m_output << "    add rsp, 8" << "\n";
            m_stack_size--;
        }

        // Function to convert float to hexadecimal
        std::string floatStringToHex(const std::string& floatString) {
            // Convert string to float
            float floatValue = std::stof(floatString);

            // Convert float to binary string
            std::bitset<sizeof(float) * 8> bits(*reinterpret_cast<unsigned int*>(&floatValue));

            // Convert binary string to hexadecimal string
            std::stringstream hexStream;
            hexStream << std::hex << std::uppercase << std::setw(sizeof(float) * 2) << std::setfill('0') << bits.to_ulong();
            return hexStream.str();
        }

        // Creates labels for assembly jumping for if statements
        std::string create_label() {
            return "label" + std::to_string(m_label_count++);
        }

        const NodeProg* m_prog;
        std::stringstream m_output;

        // Our own stack pointer to keep track of what we are pushing and popping onto stack
        size_t m_stack_size = 0;

        struct  Var {
            size_t stack_loc;
            std::string name;
            std::optional<TokenType> int_or_float;
        };

        // vector to store object and its location on stack
        std::vector<Var> m_vars {};

        // Vector of indices into Vars
        std::vector<size_t> m_scopes {};

        int m_label_count = 0;
};