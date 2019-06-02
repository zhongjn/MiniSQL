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
public:
    CatalogManager(BlockManager* block_mgr);
    void add_relation(const Relation& relation);
    void remove_relation(const string& name);
    Nullable<Relation> get_relation(const string& name);
    // vector<Relation> list_relation();
};
