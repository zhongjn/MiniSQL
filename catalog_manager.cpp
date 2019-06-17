#include "catalog_manager.h"

CatalogManager::CatalogManager(BlockManager* block_mgr) : block_mgr(block_mgr) {
    string& scheme = Files::scheme();

    if (block_mgr->file_blocks(scheme) == 0) {
        block_mgr->file_append_block(scheme);
        BlockGuard bg_init(block_mgr, scheme, 0);
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

static void check_name(const string& name) {
    if (name == "") throw logic_error("Empty name.");
    if (name.length() >= NAME_LENGTH) throw logic_error("Name exceeded maximum length.");
}

static void check_record_length(const Relation & rel) {
    if (rel.record_length() > BLOCK_SIZE) {
        throw logic_error("Exceeded maximum record length.");
    }
}

void CatalogManager::add_relation(const Relation & relation)
{
    check_name(relation.name);
    check_record_length(relation);

    BlockGuard bg(block_mgr, Files::scheme(), 0);
    auto* file_db = bg.addr<DatabaseData>();

    int block = file_db->get_block_index(relation.name.c_str());
    if (block >= 0) throw logic_error("Relation name duplicated.");

    int free_block = file_db->get_free_block_index();
    if (free_block < 0) throw logic_error("Exceeded maximum relations.");

    // 扩充文件大小
    int block_count = block_mgr->file_blocks(Files::scheme());
    if (free_block >= block_count) {
        block_mgr->file_append_block(Files::scheme());
    }

    char* dest = file_db->rel_names[file_db->block_to_rel(free_block)];
    strcpy(dest, relation.name.c_str());
    bg.set_modified();

    BlockGuard bg_rel(block_mgr, Files::scheme(), free_block);
    auto* file_rel = bg_rel.addr<RelationData>();
    *file_rel = relation.to_file();
    bg_rel.set_modified();
}

void CatalogManager::remove_relation(const string & name)
{
    check_name(name);

    BlockGuard bg(block_mgr, Files::scheme(), 0);
    auto* file_db = bg.addr<DatabaseData>();

    int block = file_db->get_block_index(name.c_str());
    if (block < 0) throw logic_error("Relation not found.");

    char* dest = file_db->rel_names[file_db->block_to_rel(block)];
    dest[0] = 0; // 清除名字字符串

    bg.set_modified();
}

Nullable<Relation> CatalogManager::get_relation(const string & name)
{
    check_name(name);

    BlockGuard bg(block_mgr, Files::scheme(), 0);
    auto* file_db = bg.addr<DatabaseData>();

    int block = file_db->get_block_index(name.c_str());
    if (block < 0) return Null();

    BlockGuard bg_rel(block_mgr, Files::scheme(), block);
    auto* file_rel = bg_rel.addr<RelationData>();

    Relation rel;
    rel.from_file(*file_rel);
    rel.name = name;
    return rel;
}

