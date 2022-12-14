#pragma once

#include <data_structures/tokens/tokens.hpp>
#include <lox/helpers.hpp>
#include <memory>
#include <string>
#include <variant>

namespace lox::expressions {

struct String;
struct Number;
struct Boolean;
struct Nil;
struct Unary;
struct Binary;
struct Grouping;
class Expr;

using ExprPtr = std::shared_ptr<Expr>;

struct String {
    explicit String(std::string value);

    std::string value_;
};

struct Number {
    explicit Number(double value);

    double value_ = 0.0;
};

struct Boolean {
    explicit Boolean(bool value);

    bool value_ = false;
};

struct Nil {
    std::monostate value_;
};

struct Unary {
    Unary(ExprPtr expr, tokens::Token&& op);

    ExprPtr expr_;
    tokens::Token op_;
};

struct Binary {
    Binary(ExprPtr left, ExprPtr right, tokens::Token&& op);

    ExprPtr left_;
    ExprPtr right_;
    tokens::Token op_;
};

struct Conditional {
    Conditional(ExprPtr first, ExprPtr second, ExprPtr third);

    ExprPtr first_;
    ExprPtr second_;
    ExprPtr third_;
};

struct Grouping {
    explicit Grouping(ExprPtr expr);

    ExprPtr expr_;
};

struct Variable {
    explicit Variable(const tokens::Token& name);

    tokens::Token name_;
};

struct Assign {
    Assign(const tokens::Token& name, ExprPtr value);

    tokens::Token name_;
    ExprPtr value_;
};

struct Logical {
    Logical(ExprPtr left, ExprPtr right, tokens::Token&& op);

    ExprPtr left_;
    ExprPtr right_;
    tokens::Token op_;
};

template <typename T>
concept IsLiteral = IsTypeOf<T, String, Number, Boolean, Nil>;

template <typename T>
concept IsExpression = IsLiteral<T> || IsTypeOf<T, Unary, Binary, Conditional, Grouping, Variable, Assign, Logical>;

class Expr {
 public:
    template <IsExpression T>
    explicit Expr(T&& expr) : expr_(std::forward<T>(expr)) {
    }

    template <typename V>
    auto Accept(V& visitor) const {
        return std::visit(visitor, expr_);
    }

    template <IsExpression T>
    const T& As() const {
        return std::get<T>(expr_);
    }

    template <IsExpression T>
    bool Is() const {
        return std::holds_alternative<T>(expr_);
    }

 private:
    std::variant<String, Number, Boolean, Nil, Unary, Binary, Conditional, Grouping, Variable, Assign, Logical> expr_;
};

template <IsExpression T, typename... Args>
ExprPtr MakeExpr(Args&&... args) {
    return std::make_shared<Expr>(T(std::forward<Args>(args)...));
}

}  // namespace lox::expressions
