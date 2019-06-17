#include "record_manager.h"

RecordManager::RecordManager(BlockManager* block_mgr) : block_mgr(block_mgr) {

}

void RecordManager::add_relation(const Relation& rel) {
    auto& rel_file = Files::relation(rel.name);

    if (block_mgr->file_blocks(rel_file) == 0) {
        block_mgr->file_append_block(rel_file);
    }

    BlockGuard bg(block_mgr, rel_file, 0);
    auto* rel_entry = new (bg.addr()) RelationEntryData;
    rel_entry->empty = RECORD_START;

    bg.set_modified();
}

void RecordManager::remove_relation(const string& name) {
    block_mgr->file_delete(Files::relation(name));
}

RecordPosition RecordManager::insert_record(const Relation& rel, const Record& record) {
    auto& values = record.values;

    if (rel.fields.size() != values.size()) {
        throw logic_error("Fields count mismatch.");
    }

    auto& rel_file = Files::relation(rel.name);
    BlockGuard bg(block_mgr, rel_file, 0);
    auto* rel_entry = bg.addr<RelationEntryData>();

    int record_len = rel.record_length();

    RecordPosition insert_pos;
    bool free_used = false;

    // 1. find a free space (if no free record exists, use new empty area)
    if (rel_entry->free_head.nil()) {
        auto empty = rel_entry->empty;
        if (empty.block_index >= block_mgr->file_blocks(rel_file)) {
            block_mgr->file_append_block(rel_file);
        }
        // rel_entry->free_head = RecordPosition(empty);
        insert_pos = empty;
        // prepare empty area pointer for next time
        rel_entry->empty = empty.next(record_len);
    }
    else {
        free_used = true;
        insert_pos = rel_entry->free_head;
    }

    BlockGuard bg_record(block_mgr, rel_file, insert_pos.block_index);
    auto* rec_entry = bg_record.addr<RecordEntryData>(insert_pos.pos);
    if (free_used) {
        rel_entry->free_head = rec_entry->free_next;
    }
    rec_entry->use = true;

    // 2. 找到插入位置，设置use，如果用了free，那么指定新的free_head
    update_record(rel, insert_pos, record);

    bg.set_modified();
    return insert_pos;
}

unique_ptr<Scanner> RecordManager::select_record(const Relation& rel, unique_ptr<IndexIterator> index_it)
{
    return unique_ptr<Scanner>(new DiskScanner(block_mgr, rel, move(index_it)));
}

void RecordManager::delete_record(const Relation& rel, RecordPosition pos) {
    auto& rel_file = Files::relation(rel.name);

    BlockGuard bg(block_mgr, rel_file, 0);
    auto* rel_entry = bg.addr<RelationEntryData>();
    
    BlockGuard bg_record(block_mgr, rel_file, pos.block_index);
    auto* rec_entry = bg_record.addr<RecordEntryData>(pos.pos);

    if (!rec_entry->use) throw logic_error("The record does not exsist.");

    RecordPosition head_origin = rel_entry->free_head;
    rel_entry->free_head = pos;
    rec_entry->use = false;
    rec_entry->free_next = head_origin;

    bg_record.set_modified();
    bg.set_modified();
}

void RecordManager::update_record(const Relation& rel, RecordPosition pos, const Record& record) {
    auto& rel_file = Files::relation(rel.name);

    BlockGuard bg_record(block_mgr, rel_file, pos.block_index);
    auto* rec_entry = bg_record.addr<RecordEntryData>(pos.pos);

    // 3. 填入值
    uint8_t* values_addr = rec_entry->values;
    int i_field = 0;
    for (auto& field : rel.fields) {
        void* addr = values_addr + field.offset;
        record.values[i_field].write(addr, field.type);
        i_field++;
    }
    bg_record.set_modified();
}
