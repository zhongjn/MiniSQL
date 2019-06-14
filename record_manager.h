#pragma once

#include <list>
#include <vector>
#include <stdexcept>
#include <memory>
#include "nullable.h"
#include "common.h"
#include "block_manager.h"
#include "scanner.h"
#include "expression.h"

using namespace std;

class RecordManager {
    BlockManager* block_mgr;
public:
    RecordManager(BlockManager* block_mgr);
    void add_relation(const Relation& rel);
    void remove_relation(const string& name);
    RecordPosition insert_record(const Relation& rel, const Record& record);
    unique_ptr<Scanner> select_record(const Relation& rel, unique_ptr<IndexIterator> index_it);
    void delete_record(const Relation& rel, RecordPosition pos);
    void update_record(const Relation& rel, RecordPosition pos, const Record& record);
};