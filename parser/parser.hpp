#pragma once

#include <data_structures/ast/expressions.hpp>
#include <data_structures/ast/statements.hpp>
#include <data_structures/tokens/tokens.hpp>
#include <lox/errors.hpp>
#include <vector>

namespace lox {

// program          -> declaration* EOF ;
// declaration      -> varDecl | statement ;
// varDecl          -> "var" IDENTIFIER ( "=" expression )? ";" ;
// statement        -> exprStmt | printStmt | block | ifStmt | whileStmt | forStmt ;
// exprStmt         -> expression ";" ;
// printStmt        -> "print" expression ";" ;
// block            -> "{" declaration* "}" ;
// ifStmt           -> "if" "(" expression ")" statement ( "else" statement )? ;
// whileStmt        -> "while" "(" expression ")" statement ;
// forStmt          -> "for" "(" (varDecl | exprStmt | ";" ) expression? ";" expression? ")" statement ;
// expression       -> comma ;
// comma            -> assignment ( "," assignment )* ;
// assignment       -> IDENTIFIER "=" assignment | conditional ;
// conditional      -> logic_or ( "?" expression ":" expression )? ;
// logic_or         -> logic_and ( "or" logic_and )* ;
// logic_and        -> equality ( "and" equality )* ;
// equality         -> comparison ( ( "!=" | "==" ) comparison )* ;
// comparison       -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
// term             -> factor ( ( "-" | "+" ) factor )* ;
// factor           -> unary ( ( "/" | "*" ) unary )* ;
// unary            -> ( "!" | "-" ) primary ;
// primary          -> NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" | IDENTIFIER ;
//
// Error productions for binary expressions without lhs:
// primary          -> ( "!=" | "==" ) equality
//                  | ( ">" | ">=" | "<" | "<=" ) comparison
//                  | "+" term
//                  | ( "/" | "*" ) factor ;

class Lox;

class Parser {
 public:
    Parser(std::vector<tokens::Token>&& tokens, Lox& lox);
    std::vector<statements::Stmt> Parse();

 private:
    expressions::ExprPtr Expression();
    expressions::ExprPtr Comma();
    expressions::ExprPtr Assignment();
    expressions::ExprPtr Conditional();
    expressions::ExprPtr LogicOr();
    expressions::ExprPtr LogicAnd();
    expressions::ExprPtr Equality();
    expressions::ExprPtr Comparison();
    expressions::ExprPtr Term();
    expressions::ExprPtr Factor();
    expressions::ExprPtr Unary();
    expressions::ExprPtr Primary();

    statements::Stmt Declaration();
    statements::Stmt VarDeclaration();
    statements::Stmt Statement();
    statements::Stmt PrintStatement();
    statements::Stmt BlockStatement();
    statements::Stmt IfStatement();
    statements::Stmt WhileStatement();
    statements::Stmt ForStatement();
    statements::Stmt ExpressionStatement();

    bool Check(tokens::Type type) const;
    const tokens::Token& Advance();
    bool IsAtEnd() const;
    const tokens::Token& Peek() const;
    const tokens::Token& Previous() const;
    const tokens::Token& Consume(tokens::Type type, const std::string& message);
    ParseError Error(const tokens::Token& token, const std::string& message);
    void Synchronize();

    bool Match(tokens::Type type);

    template <tokens::IsTokenType T, tokens::IsTokenType... Args>
    bool Match(T type, Args... types) {
        return Match(type) || Match(types...);
    }

    template <expressions::IsExpression As = expressions::Binary, typename Sub, tokens::IsTokenType... Args>
    expressions::ExprPtr ParseExpr(Sub&& sub_expr, Args&&... types) {
        auto expr = sub_expr();
        while (Match(std::forward<Args>(types)...)) {
            auto op = Previous();
            auto right = sub_expr();
            expr = MakeExpr<As>(std::move(expr), std::move(right), std::move(op));
        }
        return expr;
    }

 private:
    std::vector<tokens::Token> tokens_;
    uint32_t current_ = 0;
    Lox& lox_;
};

}  // namespace lox
