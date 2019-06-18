#include "catalog_manager.h"

static void check_name(const string& name) {
    if (name == "") throw logic_error("Empty name.");
    if (name.length() >= NAME_LENGTH) throw logic_error("Name exceeded maximum length.");
}

static void check_record_length(const Relation & rel) {
    if (rel.record_length() > BLOCK_SIZE) {
        throw logic_error("Exceeded maximum record length.");
    }
}

void CatalogManager::set_index_rel(const string & rel_name, int field_index, bool use_index) {
    check_name(rel_name);

    BlockGuard bg(block_mgr, Files::catalog(), 0);
    auto* file_db = bg.addr<DatabaseData>();

    int block = file_db->get_block(rel_name.c_str());
    if (block < 0) throw logic_error("Relation does not exist");

    BlockGuard bg_rel(block_mgr, Files::catalog(), block);
    auto* file_rel = bg_rel.addr<RelationData>();

    if (field_index < 0 || field_index >= file_rel->field_count) return;

    FieldData & field = file_rel->fields[field_index];
    if (field.has_index != use_index) {
        if (use_index && !field.unique) throw logic_error("The field must be unique.");
        field.has_index = use_index;
        bg_rel.set_modified();
    }
    //else {
    //    throw logic_error(use_index ? "The index alread exists" : "The index does not exist");
    //}
}

CatalogManager::CatalogManager(BlockManager * block_mgr) : block_mgr(block_mgr) {
    string& catalog = Files::catalog();

    if (block_mgr->file_blocks(catalog) == 0) {
        block_mgr->file_append_block(catalog);
        BlockGuard bg_init(block_mgr, catalog, 0);
        auto* db = new(bg_init.addr()) DatabaseData;
        bg_init.set_modified();
    }

    //BlockGuard bg(block_mgr, scheme, 0);
    //auto* file_db = (INFILE_Database*)bg.addr;
    //for (int i = 0; i < MAX_RELATIONS; i++) {
    //    char* name = file_db->rel_names[i];
    //    if (name[0] != 0) {
    //        relation_lookup.insert(pair<string, int>(name, i));
    //    }
    //}'

}

void CatalogManager::add_relation(const Relation & relation)
{
    check_name(relation.name);
    check_record_length(relation);

    BlockGuard bg(block_mgr, Files::catalog(), 0);
    auto* file_db = bg.addr<DatabaseData>();

    int block = file_db->get_block(relation.name.c_str());
    if (block >= 0) throw logic_error("Relation name duplicated.");

    int free_block = file_db->get_free_block();
    if (free_block < 0) throw logic_error("Exceeded maximum relations.");

    // 扩充文件大小
    int block_count = block_mgr->file_blocks(Files::catalog());
    if (free_block >= block_count) {
        block_mgr->file_append_block(Files::catalog());
    }

    char* dest = file_db->rel_names[file_db->block_to_rel(free_block)];
    strcpy(dest, relation.name.c_str());
    bg.set_modified();

    BlockGuard bg_rel(block_mgr, Files::catalog(), free_block);
    auto* file_rel = bg_rel.addr<RelationData>();
    *file_rel = relation.to_file();
    bg_rel.set_modified();
}

void CatalogManager::remove_relation(const string & name)
{
    check_name(name);

    BlockGuard bg(block_mgr, Files::catalog(), 0);
    auto* file_db = bg.addr<DatabaseData>();

    int block = file_db->get_block(name.c_str());
    if (block < 0) throw logic_error("Relation not found.");

    char* dest = file_db->rel_names[file_db->block_to_rel(block)];
    dest[0] = 0; // 清除名字字符串

    bg.set_modified();
}

Nullable<Relation> CatalogManager::get_relation(const string & name)
{
    check_name(name);

    BlockGuard bg(block_mgr, Files::catalog(), 0);
    auto* file_db = bg.addr<DatabaseData>();

    int block = file_db->get_block(name.c_str());
    if (block < 0) return Null();

    BlockGuard bg_rel(block_mgr, Files::catalog(), block);
    auto* file_rel = bg_rel.addr<RelationData>();

    Relation rel;
    rel.from_file(*file_rel);
    rel.name = name;
    return rel;
}

void CatalogManager::add_index(const string & rel_name, const string & field_name, const string & index_name) {
    check_name(rel_name);
    check_name(index_name);
    Nullable<Relation> rel = get_relation(rel_name);
    if (!rel) throw logic_error("This relation does not exist");

    BlockGuard bg(block_mgr, Files::catalog(), 0);
    auto* file_db = bg.addr<DatabaseData>();

    int free_index = file_db->get_free_index();
    if (free_index < 0) throw logic_error("Exceeded maximum indexes");

    int fi = 0;
    bool found = false;
    for (Field& f : rel->fields) {
        if (f.name == field_name) {
            set_index_rel(rel_name, fi, true);
            f.index_name = index_name;
            found = true;
            break;
        }
        fi++;
    }

    if (!found) throw logic_error("This field does not exist");

    IndexNameLocationData* il = &file_db->indexes[free_index];
    il->field = fi;
    strcpy(il->index_name, index_name.c_str());
    strcpy(il->relation_name, rel_name.c_str());
    bg.set_modified();
}

void CatalogManager::remove_index(const string & index_name) {
    BlockGuard bg(block_mgr, Files::catalog(), 0);
    auto* file_db = bg.addr<DatabaseData>();

    int index = file_db->get_index(index_name.c_str());
    if (index < 0) throw logic_error("Index not found");

    IndexNameLocationData& il = file_db->indexes[index];
    set_index_rel(il.relation_name, il.field, false);

    file_db->indexes[index].index_name[0] = 0;
    bg.set_modified();
}

Nullable<IndexLocation> CatalogManager::get_index_location(const string & index_name) {
    BlockGuard bg(block_mgr, Files::catalog(), 0);
    auto* file_db = bg.addr<DatabaseData>();

    int index = file_db->get_index(index_name.c_str());
    if (index < 0) return Null();

    IndexLocation il;
    il.from_file(file_db->indexes[index]);
    return il;
}
//
//void CatalogManager::add_index_rel(const string& rel_name, int field_index) {
//    set_index(rel_name, field_index, true);
//}
//
//void CatalogManager::remove_index_rel(const string& rel_name, int field_index) {
//    set_index(rel_name, field_index, false);
//}
//
