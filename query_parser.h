#pragma once

#include "query_lexer.h"
#include "expression.h"
#include "common.h"
#include "nullable.h"

struct Statement {
private:
    virtual void placeholder() {}
};

struct SelectStatement;

struct SelectSource {
    enum class Type {
        physical,
        subquery
    } type;
    string physical;
    unique_ptr<SelectStatement> subquery;
};

struct InsertStatement : public Statement {
    string into;
    vector<string> fields;
    vector<pair<Type, Value>> values;
};

struct SelectField {
    unique_ptr<Expression> expr;
    // Nullable<string> alias;
};

struct SelectStatement : public Statement {
    vector<SelectField> select;
    Nullable<SelectSource> from;
    unique_ptr<Expression> where;
};

struct UpdateStatement : public Statement {
    // TODO
};

struct DeleteStatement : public Statement {
    // TODO
};

class QueryParser {
private:
    int _pos = 0;
    vector<Token> _tokens;

    void assert(bool p);

    bool reset(int p) { _pos = p; return true; }

    bool consume(Token& match, initializer_list<const char*> contents);
    bool consume(Token& match, initializer_list<TokenType> types);
    bool consume(Token& match, const char* content) { return consume(match, { content }); }
    bool consume(const char* content) { return consume(Token(), content); }
    bool consume(Token& match, TokenType type) { return consume(match, { type }); }

    BinaryExpression::Operator binary_op(const string& op);

    pair<Type, Value> literal(const string& lit);


    typedef bool(QueryParser::* ExprGen)(unique_ptr<Expression>&);

    // parse a sequence by given left associative binary operators
    bool left_binary_op_seq(ExprGen child, unique_ptr<Expression>& expr, initializer_list<const char*> ops);
    bool factor(unique_ptr<Expression>& expr);
    bool uniary(unique_ptr<Expression>& expr);
    bool seq_muldiv(unique_ptr<Expression>& expr) { return left_binary_op_seq(&QueryParser::uniary, expr, { "*", "/" }); }
    bool seq_addsub(unique_ptr<Expression>& expr) { return left_binary_op_seq(&QueryParser::seq_muldiv, expr, { "+", "-" }); }
    bool seq_cmp(unique_ptr<Expression>& expr) { return left_binary_op_seq(&QueryParser::seq_addsub, expr, { ">=", "<=", ">", "<" }); }
    bool seq_eq(unique_ptr<Expression>& expr) { return left_binary_op_seq(&QueryParser::seq_cmp, expr, { "==", "=", "is", "!=" }); }
    bool seq_and(unique_ptr<Expression>& expr) { return left_binary_op_seq(&QueryParser::seq_eq, expr, { "and" }); }
    bool seq_or(unique_ptr<Expression>& expr) { return left_binary_op_seq(&QueryParser::seq_and, expr, { "or" }); }
    bool expression(unique_ptr<Expression>& expr, int& start, int& end) {
        start = _pos;
        bool r = seq_or(expr);
        end = _pos;
        return r;
    }
    bool expression(unique_ptr<Expression>& expr) {
        int a, b;
        return expression(expr, a, b);
    }

    bool select_field(SelectField& f);
    bool select_list(vector<SelectField>& fields);
    bool select_source(SelectSource& source);
    bool select_stmt_inner(unique_ptr<SelectStatement>& stmt);
    bool select_stmt(unique_ptr<Statement>& stmt);

    bool stmt(unique_ptr<Statement>& s);

public:
    unique_ptr<Statement> parse(vector<Token> tokens);
};