#pragma once
#include "common.h"
#include <memory>

class Expression {
public:
    virtual const Type& type() = 0;
    string name = "test_name";
    virtual Value eval(const Record& record) = 0;
    virtual void resolve(const Relation& rel) = 0;
};

class FieldExpression : public Expression {
    bool _resolved = false;
    string _target_name;
    Type t;
    int field;
public:
    //FieldExpression(const Relation& rel, int field_index) : field(field_index), t(rel.fields[field_index].type), target_resolved(true) {};
    FieldExpression(string name) : _target_name(name) {};
    const Type& type() { return t; }
    Value eval(const Record& record);
    void resolve(const Relation& rel);
    const string& target_name() { return _target_name; }
	int get_field() { return field; }
};

class ConstantExpression : public Expression {
    Type t;
    Value v;
public:
    ConstantExpression(Type type, Value value) : t(type), v(value) {};
    const Type& type() { return t; }
    Value eval(const Record& record) { return v; }
    void resolve(const Relation& rel) {}
	const Value& get_val() { return v; }
};

class UniaryExpression : public Expression {
public:
    enum class Operator {
        NEG
    };
private:
    bool _resolved = false;
    Type _type;
    Operator _op;
    unique_ptr<Expression> _r;
public:
    UniaryExpression(Operator op, unique_ptr<Expression> r) :
        _op(op), _r(move(r)) {}
    Value eval(const Record& record);
    const Type& type() { return _type; }
    void resolve(const Relation& rel);
	Expression* get_r()	{ return _r.get(); }
};

class BinaryExpression : public Expression {
public:
    // *不要随便改变运算符的顺序*
    // 在后面判断运算符类型时要用到
    enum class Operator {
        EQ,
        NE,
        LT,
        GT,
        LE,
        GE,
        AND,
        OR,
        ADD,
        SUB,
        MUL,
        DIV
    };
private:
    bool _resolved = false;
    Type _type;
    Operator _op;
    unique_ptr<Expression> _r1, _r2;

public:
    BinaryExpression(Operator op, unique_ptr<Expression> r1, unique_ptr<Expression> r2) :
        _op(op), _r1(move(r1)), _r2(move(r2)) {}
    Value eval(const Record& record);
    const Type& type() { return _type; }
    void resolve(const Relation& rel);
	Expression* get_r1() { return _r1.get(); }
	Expression* get_r2() { return _r2.get(); }
	Operator get_op() { return _op;	}
};