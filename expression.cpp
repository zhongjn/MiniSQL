#include "expression.h"

void FieldExpression::resolve(const Relation& rel) {
    int index = 0;
    for (auto& f : rel.fields) {
        if (f.name == _target_name) {
            _resolved = true;
            field = index;
            t = f.type;
        }
        index++;
    }
}

bool is_alg_type(Type t) {
    using T = Type::Tag;
    return t.tag == T::INT || t.tag == T::FLOAT;
}

using Op1 = UniaryExpression::Operator;

Value FieldExpression::eval(const Record& record) {
    if (!_resolved) throw exception("not resolved");
    return record.values[field];
}

Value UniaryExpression::eval(const Record& record) {
    if (!_resolved) throw exception("not resolved");
    Value v = _r->eval(record);
    Value vr;
    switch (_r->type().tag)
    {
        case Type::Tag::CHAR:
            break;
        case Type::Tag::INT:
            if (_op == Op1::NEG) vr.INT = -v.INT;
            break;
        case Type::Tag::FLOAT:
            if (_op == Op1::NEG) vr.FLOAT = -v.FLOAT;
            break;
        default: throw logic_error("Unsupported type.");
    }
    return vr;
}

void UniaryExpression::resolve(const Relation & rel) {
    _r->resolve(rel);
    Type t = _r->type();
    if (_op == Op1::NEG && is_alg_type(t)) {
        _type = t;
        _resolved = true;
    }
    else {
        throw logic_error("decude failed");
    }
}


using Op2 = BinaryExpression::Operator;
template<typename V>
bool in_range(V v, V from_in, V to_in) {
    return v >= from_in && v <= to_in;
}

bool is_cmp_op(Op2 op) {
    return in_range(op, Op2::EQ, Op2::GE);
}

bool is_logic_op(Op2 op) {
    return in_range(op, Op2::AND, Op2::OR);
}

bool is_alg_op(Op2 op) {
    return in_range(op, Op2::ADD, Op2::DIV);
}

static void deduce_failed() {
    throw logic_error("deduce failed");
}

Type deduce_type(Op2 op, Type t1, Type t2) {
    using T = Type::Tag;
    Type t;
    if (is_alg_op(op)) {
        if (is_alg_type(t1) && is_alg_type(t2)) {
            if (t1.tag == T::FLOAT || t2.tag == T::FLOAT) {
                t.tag = T::FLOAT;
            }
            else {
                t.tag = T::INT;
            }
        }
        else {
            deduce_failed();
        }
    }
    else if (is_cmp_op(op)) {
        if (t1.tag == t2.tag) {
            t.tag = T::INT;
        }
        else {
            deduce_failed();
        }
    }
    else if (is_logic_op(op)) {
        if (t1.tag == T::INT && t2.tag == T::INT) {
            t.tag = T::INT;
        }
        else {
            deduce_failed();
        }
    }
    else if (op == Op2::ADD && t1.tag == T::CHAR && t2.tag == T::CHAR) { // string add
        t.tag = T::CHAR;
        t.CHAR.len = t1.CHAR.len + t2.CHAR.len;
    }
    else {
        deduce_failed();
    }
    return t;
}

#define OP_CASE(case_op, fn) case BinaryExpression::Operator::case_op: return v1 fn v2;

template<typename V1, typename V2>
int do_cmp_op(Op2 op, V1 v1, V2 v2) {
    switch (op)
    {
        OP_CASE(EQ, == );
        OP_CASE(NE, != );
        OP_CASE(LT, < );
        OP_CASE(GT, > );
        OP_CASE(LE, <= );
        OP_CASE(GE, >= );
        default: throw logic_error("Unsupported operator.");
    }
}

template<typename V1, typename V2>
int do_logic_op(Op2 op, V1 v1, V2 v2) {
    switch (op)
    {
        OP_CASE(AND, &&);
        OP_CASE(OR, || );
        default: throw logic_error("Unsupported operator.");
    }
}

template<typename V1, typename V2, typename R = decltype(V1() + V2())>
R do_alg_op(Op2 op, V1 v1, V2 v2) {
    switch (op)
    {
        OP_CASE(ADD, +);
        OP_CASE(SUB, -);
        OP_CASE(MUL, *);
        OP_CASE(DIV, / );
        default: throw logic_error("Unsupported operator.");
    }
}

Value BinaryExpression::eval(const Record & record) {
    if (!_resolved) throw exception("not resolved");
    Value v1 = _r1->eval(record), v2 = _r2->eval(record);
    Value vr;
    switch (_r1->type().tag)
    {
        case Type::Tag::CHAR:
            if (is_cmp_op(_op)) vr.INT = do_cmp_op(_op, v1.CHAR, v2.CHAR);
            else if (_op == Operator::ADD) vr.CHAR = v1.CHAR + v2.CHAR;
            break;
        case Type::Tag::INT:
            if (is_cmp_op(_op)) vr.INT = do_cmp_op(_op, v1.INT, v2.INT);
            else if (is_logic_op(_op)) vr.INT = do_logic_op(_op, v1.INT, v2.INT);
            else if (is_alg_op(_op)) vr.INT = do_alg_op(_op, v1.INT, v2.INT);
            break;
        case Type::Tag::FLOAT:
            if (is_cmp_op(_op)) vr.INT = do_cmp_op(_op, v1.FLOAT, v2.FLOAT);
            else if (is_alg_op(_op)) vr.FLOAT = do_alg_op(_op, v1.FLOAT, v2.FLOAT);
            break;
        default: throw logic_error("Unsupported type.");
    }
    return vr;
}

void BinaryExpression::resolve(const Relation & rel) {
    _r1->resolve(rel);
    _r2->resolve(rel);
    Type t1 = _r1->type(), t2 = _r2->type();
    _type = deduce_type(_op, t1, t2);
    _resolved = true;
}