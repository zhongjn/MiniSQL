#pragma once
#include "common.h"
#include "block_manager.h"
#include "catalog_manager.h"
#include "record_manager.h"
#include "index_manager.h"



class StorageEngine {
    BlockManager bm;
    CatalogManager cm;
    RecordManager rm;
    IndexManager im;
private:
    unique_ptr<IndexIterator> get_index_iterator(const Relation& rel, Nullable<IndexUsage> index_usage) {
        if (index_usage) {
            return unique_ptr<IndexIterator>(new IndexIterator(im.get_index(rel, index_usage.value())));
        }
        else {
            return  nullptr;
        }
    }

public:
    StorageEngine(int max_blocks) : bm(max_blocks), cm(&bm), rm(&bm), im(&bm) {

    }

    void add_relation(const Relation& relation) {
        cm.add_relation(relation);
        rm.add_relation(relation);
    }

    void remove_relation(const string& name) {
        cm.remove_relation(name);
        rm.remove_relation(name);
    }

    Nullable<Relation> get_relation(const string& name) {
        return cm.get_relation(name);
    }

    void add_index(const Relation& rel, int field_index) {
        cm.add_index(rel, field_index);
        im.add_index(rel, field_index, rm.select_record(rel, nullptr));
    }

    void remove_index(const Relation& rel, int field_index) {
        cm.remove_index(rel, field_index);
        im.remove_index(rel, field_index);
    }

    void insert_record(const Relation& rel, const Record& record) {
        RecordPosition pos = rm.insert_record(rel, record);
        for (int field_index : rel.indexes) {
            im.add_item(rel, field_index, record.values[field_index], pos);
        }
    }

    void delete_record(const Relation& rel, const Record& record) {
        if (record.physical_position.nil()) throw logic_error("Unexpected error. This record does not has a physical position.");
        rm.delete_record(rel, record.physical_position);
        for (int field_index : rel.indexes) {
            im.remove_item(rel, field_index, record.values[field_index]);
        }
    }

    unique_ptr<Scanner> select_record(const Relation& rel, Nullable<IndexUsage> index_usage) {
        return rm.select_record(rel, get_index_iterator(rel, index_usage));
    }

};