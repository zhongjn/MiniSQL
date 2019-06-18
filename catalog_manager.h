#pragma once

#include <string>
#include <unordered_map>
#include "common.h"
#include "block_manager.h"
#include "files.h"
#include "nullable.h"

using namespace std;

class CatalogManager {
    BlockManager* block_mgr;
    void set_index_rel(const string& rel_name, int field_index, bool use_index);

public:
    CatalogManager(BlockManager* block_mgr);

    void add_relation(const Relation& relation);
    void remove_relation(const string& name);
    Nullable<Relation> get_relation(const string& name);

    void add_index(const string& rel_name, const string& field_name, const string& index_name);
    void remove_index(const string& index_name);
    Nullable<IndexLocation> get_index_location(const string& index_name);

};
