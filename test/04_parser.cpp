#include "test.h"
#include "../query_parser.h"

TEST_CASE(parser_select_simple) {
    auto tokens = QueryLexer().tokenize("select a from test");
    auto stmt = QueryParser().parse(move(tokens));
    assert(typeid(*stmt) == typeid(SelectStatement), "stmt type");

    auto ss = static_cast<SelectStatement*>(stmt.release());

    assert(ss->select.size() == 1, "field count");
    assert(ss->select[0].expr->name == "a", "expr name");

    auto expr = ss->select[0].expr.get();
    assert(typeid(*expr) == typeid(FieldExpression), "expr type");
    
    auto fe = static_cast<FieldExpression*>(expr);
    assert(fe->target_name() == "a", "expr field name");

    assert(ss->from->type == SelectSource::Type::physical, "from");
    assert(ss->from->physical == "test", "from name");

    assert(ss->where == nullptr, "where");
}

TEST_CASE(parser_select_expr) {

    Relation rel;
    rel.fields.push_back(Field("b", Type::create_CHAR(10)));
    rel.fields.push_back(Field("a", Type::create_INT()));

    auto tokens = QueryLexer().tokenize("select 2+a*3 as C from test");
    auto stmt = QueryParser().parse(move(tokens));
    assert(typeid(*stmt) == typeid(SelectStatement), "stmt type");

    auto ss = static_cast<SelectStatement*>(stmt.release());
    auto expr = ss->select[0].expr.get();

    assert(expr->name == "C", "expr name");

    expr->resolve(rel);

    // case 1
    {
        Record rec;
        rec.values.push_back(Value::create_CHAR("abc"));
        rec.values.push_back(Value::create_INT(3));
        assert(expr->eval(rec).INT == 11, "case 1");
    }
   
    // case 2
    {
        Record rec;
        rec.values.push_back(Value::create_CHAR("ac"));
        rec.values.push_back(Value::create_INT(9));
        assert(expr->eval(rec).INT == 29, "case 2");
    }
}