#include "test.h"
#include "../record_manager.h"
#include "../catalog_manager.h"
#include "../query_executor.h"

TEST_CASE(executor_direct) {
    StorageEngine eng;

    auto tokens = QueryLexer().tokenize("select id,name,contact from test_table");
    auto stmt = QueryParser().parse(move(tokens));
    auto table = QueryExecutor(&eng).execute(move(stmt));

    assert(table.records.size() == 2, "record count");

    {
        auto& rec = table.records[0];
        assert(rec.values.size() == 3, "value count");
        assert(rec.values[0].INT == 1, "value");
        assert(rec.values[1].CHAR == "Zhang San", "value");
        assert(rec.values[2].CHAR == "z3@example.com", "value");
    }

    {
        auto& rec = table.records[1];
        assert(rec.values.size() == 3, "value count");
        assert(rec.values[0].INT == 2, "value");
        assert(rec.values[1].CHAR == "Li Si", "value");
        assert(rec.values[2].CHAR == "l4@example.com", "value"); 
    }
}

TEST_CASE(executor_expr_where) {
    StorageEngine eng;

    auto tokens = QueryLexer().tokenize("select 2+4*-id from test_table where id=2");
    auto stmt = QueryParser().parse(move(tokens));
    auto table = QueryExecutor(&eng).execute(move(stmt));

    assert(table.records.size() == 1, "record count");

    {
        auto& rec = table.records[0];
        assert(rec.values.size() == 1, "value count");
        assert(rec.values[0].INT == -6, "value");
    }
}

TEST_CASE(executor_where_logic) {
    StorageEngine eng;

    auto tokens = QueryLexer().tokenize("select * from test_table where id>=1 and (name='Li Si' or name='Zhang San')");
    auto stmt = QueryParser().parse(move(tokens));
    auto table = QueryExecutor(&eng).execute(move(stmt));

    assert(table.records.size() == 2, "record count");

}

TEST_CASE(executor_nested) {
    StorageEngine eng;

    auto tokens = QueryLexer().tokenize("select N from (select id I, name N from test_table) where I=1");
    auto stmt = QueryParser().parse(move(tokens));
    auto table = QueryExecutor(&eng).execute(move(stmt));

    assert(table.records.size() == 1, "record count");

    {
        auto& rec = table.records[0];
        assert(rec.values.size() == 1, "value count");
        assert(rec.values[0].CHAR == "Zhang San", "value");
    }
}