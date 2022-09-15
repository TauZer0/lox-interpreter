#pragma once

#include <data_structures/ast/expressions.hpp>
#include <data_structures/tokens/tokens.hpp>
#include <vector>

namespace lox {

template <typename T>
concept IsTokenType = std::is_same_v<T, tokens::Type>;

class Parser {
 public:
    explicit Parser(std::vector<tokens::Token>&& tokens);
    expressions::ExprPtr Parse();

 private:
    expressions::ExprPtr Expression();
    expressions::ExprPtr Equality();
    expressions::ExprPtr Comparison();
    expressions::ExprPtr Term();
    expressions::ExprPtr Factor();
    expressions::ExprPtr Unary();
    expressions::ExprPtr Primary();

    bool Check(tokens::Type type) const;
    const tokens::Token& Advance();
    bool IsAtEnd() const;
    const tokens::Token& Peek() const;
    const tokens::Token& Previous() const;
    const tokens::Token& Consume(tokens::Type type, const std::string& message);

    bool Match(tokens::Type type);

    template <IsTokenType T, IsTokenType... Args>
    bool Match(T type, Args... types) {
        return Match(type) || Match(types...);
    }

 private:
    std::vector<tokens::Token> tokens_;
    uint32_t current_ = 0;
};

}  // namespace lox
