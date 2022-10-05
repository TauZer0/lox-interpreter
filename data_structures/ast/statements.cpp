#include "statements.hpp"

namespace lox::statements {

Expression::Expression(expressions::ExprPtr expr) : expr_(std::move(expr)) {
}

Print::Print(expressions::ExprPtr expr) : expr_(std::move(expr)) {
}

Var::Var(tokens::Token&& name, expressions::ExprPtr initializer)
    : name_(std::move(name)), initializer_(std::move(initializer)) {
}

Block::Block(std::vector<Stmt>&& statements) : statements_(std::move(statements)) {
}

}  // namespace lox::statements
