#pragma once

#include "common.h"
#include "nullable.h"
#include "index.h"
#include "scanner.h"
struct tree_information {
	int root;
	int degree;
};
struct treenode
{
	bool Is_root;
	bool Is_leaf;
	int size;
	int fa;
	int next;
	int before;
};
class IndexManager {

public:
	IndexManager(BlockManager* block_mgr);
    // TODO: 对一列添加索引。由于存在现有数据，因此传入一个scanner来读取现有数据。
	void add_index(const Relation& rel, int field_index, unique_ptr<Scanner> scanner);
	tree_information IndexManager::get_information(const Relation& rel, int field_index, string& scheme);
	void insert_value(string& scheme, Type type, const Value& value, int degree, int num, int pos);
	void insert_record(string& scheme, Type type, RecordPosition record_pos, int degree, int num, int pos);
	void insert_block(string& scheme, Type type, int block_index, int degree, int num, int pos);
	Value find_min(string& scheme, Type type, int num, int degree);
	RecordPosition get_record(string& scheme, Type type, int num, int degree, int pos);
	int get_block(string& scheme, Type type, int num, int degree, int pos);
	Value get_value(string& scheme, Type type, int num, int degree, int pos);
	void shift_value(string& scheme, Type type, int degree, int num, int pos, int direction);
	void shift_block(string& scheme, Type type, int degree, int num, int pos, int direction);
	void shift_record(string& scheme, Type type, int degree, int num, int pos, int direction);
	void merge_leaf(string&scheme, Type type, int left_leaf, int right_leaf, int degree);
	void merge_nonleaf(string&scheme, Type type, int left_nonleaf, int right_nonleaf, int degree);
	Nullable<RecordPosition> find(string& scheme, Type type, int degree, Value value, int num);
	void delete_parent_information(string& scheme, Type type, int num, Value value, int degree);
    void remove_index(const Relation& rel, int field_index) {}
	int get_new_block(string& scheme);
    // TODO: 获取一列上的索引（迭代器），迭代器会告诉你下一行的磁盘位置
    IndexIterator get_index(const Relation& rel, IndexUsage index_usage) { return IndexIterator();  };
	void work(string& scheme, const Relation& rel, int field_index, const Value& value, RecordPosition record_pos, int num, int degree);
    // TODO: 对一列添加一个值
	void add_item(const Relation& rel, int field_index, const Value& value, RecordPosition record_pos);
	void fresh_information(string& scheme, Type type, int num, int degree, Value old_value, Value new_value);
    // TODO: 对一列删除一个值
	void remove_single_item(string& scheme, Type type, int degree, Value value, int num);
	void remove_item(const Relation& rel, int field_index, const Value& value);
private:
	BlockManager* block_mgr;
};

