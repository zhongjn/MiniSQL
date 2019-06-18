#include "test.h"
#include "../scanner.h"
#include "../record_manager.h"

TEST_CASE(scanner_disk_filter) {
    BlockManager bm;
    RecordManager rm(&bm);

    Relation rel("test_table");
    Field f_id("id", Type::create_INT());
    f_id.unique = true;
    rel.fields.push_back(f_id);
    rel.fields.push_back(Field("name", Type::create_CHAR(20)));
    rel.fields.push_back(Field("contact", Type::create_CHAR(20)));
    rel.update();

    // case 1. constant 0 expression
    {
        auto expr = unique_ptr<Expression>(new ConstantExpression(Type::create_INT(), Value::create_INT(0)));
        auto sc_disk = rm.select_record(rel, nullptr);
        auto sc_filter = unique_ptr<Scanner>(new FilterScanner(move(sc_disk), move(expr)));
        int count = 0;
        while (sc_filter->next()) {
            count++;
        }
        assert(count == 0, "case 1");
    }

    // case 2. constant 1 expression
    {
        auto expr = unique_ptr<Expression>(new ConstantExpression(Type::create_INT(), Value::create_INT(1)));
        auto sc_disk = rm.select_record(rel, nullptr);
        auto sc_filter = unique_ptr<Scanner>(new FilterScanner(move(sc_disk), move(expr)));
        int count = 0;
        while (sc_filter->next()) {
            count++;
        }
        assert(count == 2, "case 2");
    }

    // case 3. string predicate expression
    {
        auto expr_z3 = unique_ptr<Expression>(new ConstantExpression(Type::create_CHAR(20), Value::create_CHAR("Zhang San")));
        auto expr_name = unique_ptr<Expression>(new FieldExpression("name"));
        auto expr_eq = unique_ptr<Expression>(new BinaryExpression(BinaryExpression::Operator::EQ, move(expr_z3), move(expr_name)));
        expr_eq->resolve(rel);

        auto sc_disk = rm.select_record(rel, nullptr);
        auto sc_filter = unique_ptr<Scanner>(new FilterScanner(move(sc_disk), move(expr_eq)));
        int count = 0;
        while (sc_filter->next()) {
            if (count == 0) {
                assert(sc_filter->current().values[0].INT == 1, "case 3 value");
            }
            count++;
        }
        assert(count == 1, "case 3");
    }
    string a;
    string b;
}

TEST_CASE(scanner_project) {
    BlockManager bm;
    RecordManager rm(&bm);

    Relation rel("test_table");
    Field f_id("id", Type::create_INT());
    f_id.unique = true;
    rel.fields.push_back(f_id);
    rel.fields.push_back(Field("name", Type::create_CHAR(20)));
    rel.fields.push_back(Field("contact", Type::create_CHAR(20)));
    rel.update();

    {
        auto sc_disk = rm.select_record(rel, nullptr);
        vector<unique_ptr<Expression>> fields;
        auto fe = new FieldExpression("name");
        fe->resolve(rel);
        fields.push_back(unique_ptr<Expression>(fe));
        auto sc_proj = unique_ptr<ProjectScanner>(new ProjectScanner(move(sc_disk), move(fields)));

        int count = 0;
        while (sc_proj->next()) {
            if (count == 0)  assert(sc_proj->current().values[0].CHAR == "Zhang San", "row 0");
            if (count == 1)  assert(sc_proj->current().values[0].CHAR == "Li Si", "row 1");
            count++;
        }
        assert(count == 2, "count");
    }
}