#include "case.h"
#include "../record_manager.h"

TEST_CASE(record) {
    BlockManager bm;
    RecordManager rm(&bm);

    Relation rel("test_table");
    Field f_id("id", Type::create_INT());
    f_id.unique = true;
    f_id.primary_key = true;
    rel.fields.push_back(f_id);
    rel.fields.push_back(Field("name", Type::create_CHAR(20)));
    rel.fields.push_back(Field("contact", Type::create_CHAR(20)));
    rel.update();

    rm.add_relation(rel);
    
    Value v;

    {
        vector<Value> values;

        v.INT = 1;
        values.push_back(v);
        v.CHAR = "Zhang San";
        values.push_back(v);
        v.CHAR = "z3@example.com";
        values.push_back(v);

        rm.insert_record(rel, values);
    }

    {
        vector<Value> values;

        v.INT = 2;
        values.push_back(v);
        v.CHAR = "Li Si";
        values.push_back(v);
        v.CHAR = "l4@example.com";
        values.push_back(v);

        rm.insert_record(rel, values);
    }

    auto scanner = rm.select_record(rel, nullptr);
    int i = 0;
    while (scanner->next()) {
        auto& rec = scanner->current();
        if (i == 0) {
            assert(rec.values.size() == 3, "value count");
            assert(rec.values[0].INT == 1, "value");
            assert(rec.values[1].CHAR == "Zhang San", "value");
            assert(rec.values[2].CHAR == "z3@example.com", "value");
        }
        else if (i == 1) {
            assert(rec.values.size() == 3, "value count");
            assert(rec.values[0].INT == 2, "value");
            assert(rec.values[1].CHAR == "Li Si", "value");
            assert(rec.values[2].CHAR == "l4@example.com", "value");
        }
        i++;
    }

    assert(i == 2, "record count");
}