#include "test.h"
#include "../catalog_manager.h"

TEST_CASE(catalog_relation) {

    {
        BlockManager bm;
        CatalogManager cm(&bm);

        Nullable<Relation> rt = cm.get_relation("test_table");
        if (rt) cm.remove_relation("test_table");

        Relation rel("test_table");
        Field f_id("id", Type::create_INT());
        f_id.unique = true;
        rel.fields.push_back(f_id);
        rel.fields.push_back(Field("name", Type::create_CHAR(20)));
        rel.fields.push_back(Field("contact", Type::create_CHAR(20)));
        cm.add_relation(rel);
    }

    {
        BlockManager bm;
        CatalogManager cm(&bm);

        Nullable<Relation> rel = cm.get_relation("test_table");
        assert(!rel.null(), "Relation not saved.");
        assert(rel.value().name == "test_table", "relation name");

        auto& fields = rel.value().fields;
        assert(fields.size() == 3, "field count");

        auto& f0 = fields[0];
        assert(f0.name == "id" && f0.unique && f0.type.tag == Type::Tag::INT && f0.offset == 0, "field info");
        auto& f1 = fields[1];
        assert(f1.name == "name" && !f1.unique && f1.type.tag == Type::Tag::CHAR && f1.type.CHAR.len == 20 && f1.offset == 4, "field info");
        auto& f2 = fields[2];
        assert(f2.name == "contact" && !f2.unique && f2.type.tag == Type::Tag::CHAR && f2.type.CHAR.len == 20 && f2.offset == 24, "field info");
    }
}

TEST_CASE(catalog_index) {
    {
        BlockManager bm;
        CatalogManager cm(&bm);

        if (cm.get_index_location("test_my_index")) {
            cm.remove_index("test_my_index");
        }
    }

    {
        BlockManager bm;
        CatalogManager cm(&bm);

        cm.add_index("test_table", "id", "test_my_index");
        Nullable<IndexLocation> il = cm.get_index_location("test_my_index");
        assert(il, "index not found");
        assert(il->field == 0, "field");
        assert(il->relation_name == "test_table", "relation");


    }

    {
        BlockManager bm;
        CatalogManager cm(&bm);

        // cm.add_index("test_table", "name", "some_invalid_index");
        cm.remove_index("test_my_index");
        assert(cm.get_index_location("test_my_index").null(), "not deleted");
    }
}