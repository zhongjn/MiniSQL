#pragma once

#include "common.h"
#include "nullable.h"
#include "index.h"
#include "scanner.h"

class IndexManager {
public:
    IndexManager(BlockManager* block_mgr) {};

    // TODO: 对一列添加索引。由于存在现有数据，因此传入一个scanner来读取现有数据。
    void add_index(const Relation& rel, int field_index, unique_ptr<Scanner> scanner) {}
    
    // TODO: 对一列删除索引
    void remove_index(const Relation& rel, int field_index) {}

    // TODO: 获取一列上的索引（迭代器），迭代器会告诉你下一行的磁盘位置
    IndexIterator get_index(const Relation& rel, IndexUsage index_usage) { return IndexIterator();  };
    
    // TODO: 对一列添加一个值
    void add_item(const Relation& rel, int field_index, const Value& value, RecordPosition record_pos) {}
    
    // TODO: 对一列删除一个值
    void remove_item(const Relation& rel, int field_index, const Value& value) {}
};

