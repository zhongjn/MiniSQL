#pragma once

#include "common.h"
#include "nullable.h"
#include "index.h"
#include "scanner.h"

class IndexManager {
public:
    IndexManager(BlockManager* block_mgr) {};
    // 对一列添加索引。由于存在现有数据，因此传入一个scanner来读取现有数据。
    void add_index(const Relation& rel, int field_index, unique_ptr<Scanner> scanner);
    void remove_index(const Relation& rel, int field_index);
    IndexIterator get_index(const Relation& rel, IndexUsage index_usage);
    void add_item(const Relation& rel, int field_index, const Value& value, RecordPosition record_pos);
    void remove_item(const Relation& rel, int field_index, const Value& value);
};

