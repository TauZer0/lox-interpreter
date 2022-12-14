#include "parser.hpp"

#include <lox/lox.hpp>

namespace lox {

using expressions::ExprPtr;
using expressions::MakeExpr;
using tokens::Token;
using tokens::Type;

Parser::Parser(std::vector<tokens::Token>&& tokens, Lox& lox) : tokens_(std::move(tokens)), lox_(lox) {
}

std::vector<statements::Stmt> Parser::Parse() {
    std::vector<statements::Stmt> statements;
    while (!IsAtEnd()) {
        auto stmt = Declaration();
        if (stmt.Is<std::monostate>()) {
            continue;
        }
        statements.push_back(std::move(stmt));
    }
    return statements;
}

ExprPtr Parser::Expression() {
    return Comma();
}

expressions::ExprPtr Parser::Comma() {
    auto matcher = [this]() -> expressions::ExprPtr {
        return Assignment();
    };
    return ParseExpr(std::move(matcher), Type::kComma);
}

expressions::ExprPtr Parser::Assignment() {
    auto expr = Conditional();
    if (Match(tokens::Type::kEqual)) {
        auto equals = Previous();
        auto value = Assignment();
        if (expr->Is<expressions::Variable>()) {
            auto name = expr->As<expressions::Variable>().name_;
            return MakeExpr<expressions::Assign>(name, std::move(value));
        }
        lox_.Error(equals, "Invalid assignment target.");
    }
    return expr;
}

expressions::ExprPtr Parser::Conditional() {
    auto expr = LogicOr();
    if (Match(Type::kQuestion)) {
        auto then_branch = Expression();
        Consume(Type::kColon, "Expected ':' after then-branch of ternary conditional expression.");
        auto else_branch = Expression();
        expr = MakeExpr<expressions::Conditional>(std::move(expr), std::move(then_branch), std::move(else_branch));
    }
    return expr;
}

expressions::ExprPtr Parser::LogicOr() {
    auto matcher = [this]() -> expressions::ExprPtr {
        return LogicAnd();
    };
    return ParseExpr<expressions::Logical>(std::move(matcher), Type::kOr);
}

expressions::ExprPtr Parser::LogicAnd() {
    auto matcher = [this]() -> expressions::ExprPtr {
        return Equality();
    };
    return ParseExpr<expressions::Logical>(std::move(matcher), Type::kAnd);
}

ExprPtr Parser::Equality() {
    auto matcher = [this]() -> expressions::ExprPtr {
        return Comparison();
    };
    return ParseExpr(std::move(matcher), Type::kEqualEqual, Type::kBangEqual);
}

ExprPtr Parser::Comparison() {
    auto matcher = [this]() -> expressions::ExprPtr {
        return Term();
    };
    return ParseExpr(std::move(matcher), Type::kLess, Type::kLessEqual, Type::kGreater, Type::kGreaterEqual);
}

ExprPtr Parser::Term() {
    auto matcher = [this]() -> expressions::ExprPtr {
        return Factor();
    };
    return ParseExpr(std::move(matcher), Type::kMinus, Type::kPlus);
}

ExprPtr Parser::Factor() {
    auto matcher = [this]() -> expressions::ExprPtr {
        return Unary();
    };
    return ParseExpr(std::move(matcher), Type::kStar, Type::kSlash);
}

ExprPtr Parser::Unary() {
    if (Match(Type::kBang, Type::kMinus)) {
        Token op = Previous();
        return MakeExpr<expressions::Unary>(Primary(), std::move(op));
    }
    return Primary();
}

ExprPtr Parser::Primary() {
    if (Match(Type::kFalse)) {
        return MakeExpr<expressions::Boolean>(false);
    } else if (Match(Type::kTrue)) {
        return MakeExpr<expressions::Boolean>(true);
    } else if (Match(Type::kNil)) {
        return MakeExpr<expressions::Nil>();
    } else if (Match(Type::kNumber)) {
        return MakeExpr<expressions::Number>(Previous().As<tokens::Number>().literal_);
    } else if (Match(Type::kString)) {
        return MakeExpr<expressions::String>(Previous().As<tokens::String>().literal_);
    } else if (Match(Type::kLeftParen)) {
        auto expr = Expression();
        Consume(Type::kRightParen, "Expected ')' after expression.");
        return MakeExpr<expressions::Grouping>(std::move(expr));
    } else if (Match(Type::kIdentifier)) {
        return MakeExpr<expressions::Variable>(Previous());
    }

    // Error productions
    if (Match(Type::kEqualEqual, Type::kBangEqual)) {
        Error(Previous(), "Missing left-hand operand.");
        Equality();
        return nullptr;
    } else if (Match(Type::kLess, Type::kLessEqual, Type::kGreater, Type::kGreaterEqual)) {
        Error(Previous(), "Missing left-hand operand.");
        Comparison();
        return nullptr;
    } else if (Match(Type::kPlus)) {
        Error(Previous(), "Missing left-hand operand.");
        Term();
        return nullptr;
    } else if (Match(Type::kSlash, Type::kStar)) {
        Error(Previous(), "Missing left-hand operand.");
        Factor();
        return nullptr;
    }

    throw Error(Peek(), "Expected expression.");
}

statements::Stmt Parser::Declaration() {
    try {
        if (Match(tokens::Type::kVar)) {
            return VarDeclaration();
        } else {
            return Statement();
        }
    } catch (const ParseError& error) {
        Synchronize();
        return {};
    }
}

statements::Stmt Parser::VarDeclaration() {
    auto name = Consume(tokens::Type::kIdentifier, "Expected variable name.");
    ExprPtr initializer;
    if (Match(tokens::Type::kEqual)) {
        initializer = Expression();
    }
    Consume(tokens::Type::kSemicolon, "Expect ';' after variable declaration.");
    return statements::MakeStmt<statements::Var>(std::move(name), std::move(initializer));
}

statements::Stmt Parser::Statement() {
    if (Match(tokens::Type::kPrint)) {
        return PrintStatement();
    } else if (Match(tokens::Type::kLeftBrace)) {
        return BlockStatement();
    } else if (Match(tokens::Type::kIf)) {
        return IfStatement();
    } else if (Match(tokens::Type::kWhile)) {
        return WhileStatement();
    } else if (Match(tokens::Type::kFor)) {
        return ForStatement();
    } else {
        return ExpressionStatement();
    }
}

statements::Stmt Parser::PrintStatement() {
    auto value = Expression();
    Consume(tokens::Type::kSemicolon, "Expected ';' after value.");
    return statements::MakeStmt<statements::Print>(std::move(value));
}

statements::Stmt Parser::BlockStatement() {
    std::vector<statements::Stmt> statements;
    while (!Check(tokens::Type::kRightBrace) && !IsAtEnd()) {
        auto stmt = Declaration();
        if (stmt.Is<std::monostate>()) {
            continue;
        }
        statements.push_back(std::move(stmt));
    }

    Consume(tokens::Type::kRightBrace, "Expected '}' after block.");
    return statements::MakeStmt<statements::Block>(std::move(statements));
}

statements::Stmt Parser::IfStatement() {
    Consume(tokens::Type::kLeftParen, "Expected '(' after 'if'.");
    auto expr = Expression();
    Consume(tokens::Type::kRightParen, "Expected ')' after if condition.");
    auto then_branch = std::make_shared<statements::Stmt>(Statement());
    if (Match(tokens::Type::kElse)) {
        auto else_branch = std::make_shared<statements::Stmt>(Statement());
        return statements::MakeStmt<statements::If>(std::move(expr), std::move(then_branch), std::move(else_branch));
    }
    return statements::MakeStmt<statements::If>(std::move(expr), std::move(then_branch), nullptr);
}

statements::Stmt Parser::WhileStatement() {
    Consume(tokens::Type::kLeftParen, "Expected '(' after 'while'.");
    auto condition = Expression();
    Consume(tokens::Type::kRightParen, "Expected ')' after while condition.");
    auto statement = std::make_shared<statements::Stmt>(Statement());
    return statements::MakeStmt<statements::While>(std::move(condition), std::move(statement));
}

statements::Stmt Parser::ForStatement() {
    Consume(tokens::Type::kLeftParen, "Expected '(' after 'for'.");

    statements::Stmt initializer;
    if (Match(tokens::Type::kVar)) {
        initializer = VarDeclaration();
    } else if (!Match(tokens::Type::kSemicolon)) {
        initializer = ExpressionStatement();
    }

    ExprPtr condition;
    if (!Check(tokens::Type::kSemicolon)) {
        condition = Expression();
    }
    Consume(tokens::Type::kSemicolon, "Expected ';' after loop condition.");

    ExprPtr increment;
    if (!Check(tokens::Type::kRightParen)) {
        increment = Expression();
    }
    Consume(tokens::Type::kRightParen, "Expected ')' after for clauses.");

    auto body = Statement();

    // Desugaring
    if (increment != nullptr) {
        std::vector<statements::Stmt> statements = {std::move(body),
                                                    statements::MakeStmt<statements::Expression>(std::move(increment))};
        body = statements::MakeStmt<statements::Block>(std::move(statements));
    }

    if (condition == nullptr) {
        condition = MakeExpr<expressions::Boolean>(true);
    }
    body = statements::MakeStmt<statements::While>(std::move(condition),
                                                   std::make_shared<statements::Stmt>(std::move(body)));

    if (!initializer.Is<std::monostate>()) {
        std::vector<statements::Stmt> statements = {std::move(initializer), std::move(body)};
        body = statements::MakeStmt<statements::Block>(std::move(statements));
    }

    return body;
}

statements::Stmt Parser::ExpressionStatement() {
    auto expr = Expression();
    Consume(tokens::Type::kSemicolon, "Expected ';' after value.");
    return statements::MakeStmt<statements::Expression>(std::move(expr));
}

bool Parser::Check(Type type) const {
    if (IsAtEnd()) {
        return false;
    }
    return Peek().GetType() == type;
}

const Token& Parser::Advance() {
    if (!IsAtEnd()) {
        ++current_;
    }
    return Previous();
}

bool Parser::IsAtEnd() const {
    return Peek().GetType() == Type::kEof;
}

const Token& Parser::Peek() const {
    return tokens_[current_];
}

const Token& Parser::Previous() const {
    return tokens_[current_ - 1];
}

const Token& Parser::Consume(Type type, const std::string& message) {
    if (Check(type)) {
        return Advance();
    }
    throw Error(Peek(), message);
}

ParseError Parser::Error(const tokens::Token& token, const std::string& message) {
    lox_.Error(token, message);
    return ParseError("");
}

void Parser::Synchronize() {
    Advance();

    while (!IsAtEnd()) {
        if (Previous().GetType() == tokens::Type::kSemicolon) {
            return;
        }

        switch (Peek().GetType()) {
            case tokens::Type::kClass:
            case tokens::Type::kFun:
            case tokens::Type::kVar:
            case tokens::Type::kFor:
            case tokens::Type::kIf:
            case tokens::Type::kWhile:
            case tokens::Type::kPrint:
            case tokens::Type::kReturn:
                return;
            default:
                Advance();
        }
    }
}

bool Parser::Match(tokens::Type type) {
    if (Check(type)) {
        Advance();
        return true;
    }
    return false;
}

}  // namespace lox
