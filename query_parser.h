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
    Nullable<string> alias;
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

    void assert(bool p) {
        if (!p) {
            stringstream ss;
            ss << "Syntax error. pos=" << _pos;
            throw logic_error(ss.str());
        }
    }

    bool reset(int p) { _pos = p; return true; }
    bool consume(Token& match, initializer_list<const char*> contents);
    bool consume(Token& match, initializer_list<TokenType> types);
    bool consume(Token& match, const char* content) { return consume(match, { content }); }
    bool consume(const char* content) { return consume(Token(), content); }
    bool consume(Token& match, TokenType type) { return consume(match, { type }); }


    BinaryExpression::Operator binary_op(const string& op) {
        using Op = BinaryExpression::Operator;
        static unordered_map<string, Op> lookup = {
            {"==", Op::EQ},
            {"is", Op::EQ},
            {"!=", Op::NE},
            {"<", Op::LT},
            {">", Op::GT},
            {"<=", Op::LE},
            {">=", Op::GE},
            {"+", Op::ADD},
            {"-", Op::SUB},
            {"*", Op::MUL},
            {"/", Op::DIV},
            {"and", Op::AND},
            {"or", Op::OR}
        };
        return lookup.at(op);
    }

    pair<Type, Value> literal(const string& lit) {
        if (lit.front() == '\"' && lit.back() == '\"') {
            // string
            string value = lit.substr(1, lit.length() - 2);
            return pair<Type, Value>(Type::create_CHAR(value.length() + 1), Value::create_CHAR(move(value)));
        }
        else if (lit.find('.', 0) != string::npos) {
            // float
            float value;
            stringstream ss(lit);
            ss >> value;
            return pair<Type, Value>(Type::create_FLOAT(), Value::create_FLOAT(value));
        }
        else {
            // int
            int value;
            stringstream ss(lit);
            ss >> value;
            return pair<Type, Value>(Type::create_INT(), Value::create_INT(value));
        }
    }


    typedef bool(QueryParser:: * ExprGen)(unique_ptr<Expression>&);

    // parse a sequence by given left associative binary operators
    bool left_binary_op_seq(ExprGen child, unique_ptr<Expression> & expr, initializer_list<const char*> ops) {
        int save = _pos;
        if (reset(save) && (this->*child)(expr)) {
            Token op;
            while (consume(op, ops)) {
                unique_ptr<Expression> next;
                assert((this->*child)(next));
                // TODO: parse operator
                expr = unique_ptr<Expression>(new BinaryExpression(binary_op(op.content), move(expr), move(next)));
            }
            return true;
        }
        return false;
    }

    bool factor(unique_ptr<Expression> & expr) {
        int save = _pos;
        Token match;
        if (reset(save) && consume(match, TokenType::literal)) {
            auto lit = literal(match.content);
            expr = unique_ptr<Expression>(new ConstantExpression(lit.first, lit.second));
            return true;
        }
        else if (reset(save) && consume(match, TokenType::identifier)) {
            expr = unique_ptr<Expression>(new FieldExpression(match.content));
            return true;
        }
        else if (reset(save) && consume("(") && expression(expr) && consume(")")) {
            return true;
        }
        return false;
    }

    bool uniary(unique_ptr<Expression> & expr) {
        // TODO: uniary
        if (consume("-") && factor(expr)) {
            expr = unique_ptr<Expression>(new UniaryExpression(UniaryExpression::Operator::NEG, move(expr)));
        }
        return factor(expr);
    }

    bool seq_muldiv(unique_ptr<Expression> & expr) { return left_binary_op_seq(&QueryParser::uniary, expr, { "*", "/" }); }
    bool seq_addsub(unique_ptr<Expression>& expr) { return left_binary_op_seq(&QueryParser::seq_muldiv, expr, { "+", "-" }); }
    bool seq_cmp(unique_ptr<Expression>& expr) { return left_binary_op_seq(&QueryParser::seq_addsub, expr, { ">=", "<=", ">", "<" }); }
    bool seq_eq(unique_ptr<Expression>& expr) { return left_binary_op_seq(&QueryParser::seq_cmp, expr, { "==", "=", "is", "!=" }); }
    bool seq_and(unique_ptr<Expression> & expr) { return left_binary_op_seq(&QueryParser::seq_eq, expr, { "and" }); }
    bool seq_or(unique_ptr<Expression> & expr) { return left_binary_op_seq(&QueryParser::seq_and, expr, { "or" }); }

    bool expression(unique_ptr<Expression> & expr) {
        return seq_or(expr);
    }

    bool select_field(SelectField & f) {
        assert(expression(f.expr));
        bool as = consume("as");
        Token t_alias;
        if (consume(t_alias, TokenType::identifier)) {
            f.alias = t_alias.content;
        }
        else {
            assert(!as);
            f.alias = Null();
        }
        return true;
    }

    bool select_list(vector<SelectField> & fields) {
        bool first = true;
        while (1) {
            if (first || consume(",")) {
                SelectField f;
                assert(select_field(f));
                fields.push_back(move(f));
            }
            else {
                break;
            }
            first = false;
        }
        return fields.size() > 0;
    }

    bool select_source(SelectSource & source) {
        int save = _pos;
        if (consume("(") && select_stmt_inner(source.subquery) && consume(")")) {
            source.type = SelectSource::Type::subquery;
            return true;
        }
        Token rel_physical;
        if (consume(rel_physical, TokenType::identifier)) {
            source.type = SelectSource::Type::physical;
            source.physical = rel_physical.content;
            return true;
        }
        return false;
    }

    bool select_stmt_inner(unique_ptr<SelectStatement> & stmt) {
        int save = _pos;
        if (reset(save) && consume("select")) {
            vector<SelectField> select;
            unique_ptr<Expression> where;
            SelectSource from;
            select_list(select);
            if (consume("from")) {
                assert(select_source(from));
            }
            if (consume("where")) {
                assert(expression(where));
            }
            stmt = unique_ptr<SelectStatement>(new SelectStatement());
            stmt->select = move(select);
            stmt->from = move(from);
            stmt->where = move(where);
            return true;
        }
        return false;
    }

    bool select_stmt(unique_ptr<Statement> & stmt) {
        unique_ptr<SelectStatement> select;
        if (select_stmt_inner(select)) {
            stmt = static_cast_unique_ptr<Statement>(move(select));
            return true;
        }
        return false;
    }

    bool stmt(unique_ptr<Statement> & s) {
        int save = _pos;
        if (reset(save) && select_stmt(s)) {
            return true;
        }
        return false;
    }

public:
    unique_ptr<Statement> parse(vector<Token> tokens) {
        _tokens = move(tokens);
        _pos = 0;
        unique_ptr<Statement> s;
        assert(stmt(s));
        return s;
    }
};