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

TEST_CASE(parser_insert) {
	auto tokens = QueryLexer().tokenize("insert into student values ('12345678',22)");
	auto stmt = QueryParser().parse(move(tokens));
	assert(typeid(*stmt) == typeid(InsertStatement), "stmt type");
	auto ss = static_cast<InsertStatement*>(stmt.release());
	
	assert(ss->into == "student", "into");
	assert(ss->fields.null(), "field");
	assert(ss->values[0].second.CHAR == "12345678", "values0");
	assert(ss->values[1].second.INT == 22, "values1");
}

TEST_CASE(parser_update) {
	auto tokens = QueryLexer().tokenize("update s set a = 1 + a, b = 1.05 * b where ccc == 111");
	auto stmt = QueryParser().parse(move(tokens));
	assert(typeid(*stmt) == typeid(UpdateStatement), "stmt type");
	auto ss = static_cast<UpdateStatement*>(stmt.release());

	assert(ss->table == "s", "table");
	assert(ss->set[0].item == "a", "updatefield name0");
	assert(ss->set[0].expr != nullptr, "updatefield expr0 null");
	assert(ss->set[1].item == "b", "updatefield name1");
	assert(ss->set[1].expr != nullptr, "updatefield expr1 null");
	assert(ss->where != nullptr, "where null");

}

TEST_CASE(parser_delete) {
	auto tokens = QueryLexer().tokenize("delete from student where sno = '88888888'");
	auto stmt = QueryParser().parse(move(tokens));
	assert(typeid(*stmt) == typeid(DeleteStatement), "stmt type");
	auto ss = static_cast<DeleteStatement*>(stmt.release());

	assert(ss->relation == "student", "table");
	assert(ss->where != nullptr, "where null");
}

TEST_CASE(parser_create_table) {
	auto tokens = QueryLexer().tokenize("create table student (sno char(8), sname char(16) unique, sage int, primary key(sno))");
	auto stmt = QueryParser().parse(move(tokens));
	assert(typeid(*stmt) == typeid(CreateTableStatement), "stmt type");
	auto ss = static_cast<CreateTableStatement*>(stmt.release());

	assert(ss->table == "student", "table");
	assert(ss->fields[0].name == "sno", "f0 name");
	assert(ss->fields[0].type.tag == Type::Tag::CHAR, "f0 type");
	assert(ss->fields[0].type.length() == 8, "f0 type length");
	assert(ss->fields[0].limit.null(), "f0 limit");
	assert(ss->fields[1].name == "sname", "f1 name");
	assert(ss->fields[1].type.tag == Type::Tag::CHAR, "f1 type");
	assert(ss->fields[1].type.length() == 16, "f1 type length");
	assert(ss->fields[1].limit.value() == "unique", "f1 limit");
	assert(ss->fields[2].name == "sage", "f2 name");
	assert(ss->fields[2].type.tag == Type::Tag::INT, "f2 type");
	assert(ss->fields[2].limit.null(), "f2 limit");
	assert(ss->key == "sno", "key");
}

TEST_CASE(parser_drop_table) {
	auto tokens = QueryLexer().tokenize("drop table student");
	auto stmt = QueryParser().parse(move(tokens));
	assert(typeid(*stmt) == typeid(DropTableStatement), "stmt type");
	auto ss = static_cast<DropTableStatement*>(stmt.release());

	assert(ss->table == "student", "table");
}

TEST_CASE(parser_create_index) {
	auto tokens = QueryLexer().tokenize("create index i on student ( sname )");
	auto stmt = QueryParser().parse(move(tokens));
	assert(typeid(*stmt) == typeid(CreateIndexStatement), "stmt type");
	auto ss = static_cast<CreateIndexStatement*>(stmt.release());

	assert(ss->indexname == "i", "indexname");
	assert(ss->table == "table", "table");
	assert(ss->attribution == "sname", "attribution");
}

TEST_CASE(parser_drop_index) {
	auto tokens = QueryLexer().tokenize("drop index i");
	auto stmt = QueryParser().parse(move(tokens));
	assert(typeid(*stmt) == typeid(DropIndexStatement), "stmt type");
	auto ss = static_cast<DropIndexStatement*>(stmt.release());

	assert(ss->indexname == "i", "indexname");
}