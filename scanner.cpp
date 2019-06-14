#include "scanner.h"

DiskScanner::DiskScanner(BlockManager* block_mgr, Relation _rel, unique_ptr<IndexIterator> index_it) :
    rel(move(_rel)), index_it(move(index_it)), block_mgr(block_mgr),
    bg_rel(block_mgr, Files::relation(rel.name), 0)
{
    record_length = rel.record_length();
}

void DiskScanner::parse_cur_record(RecordEntryData* rec_entry, RecordPosition rp) {
    cur_record.values.clear();
    for (auto& field : rel.fields) {
        Value value;
        value.parse(rec_entry->values + field.offset, field.type);
        cur_record.values.push_back(move(value));
    }
    cur_record.physical_position = rp;
}

bool DiskScanner::next() {
    if (index_it) {
        if (index_it->next()) {
            cur_pos = index_it->current();
            BlockGuard bg_rec(block_mgr, Files::relation(rel.name), cur_pos.block_index);
            auto rec_entry = bg_rec.addr<RecordEntryData>(cur_pos.pos);
            if (!rec_entry->use) throw logic_error("Unexpected unused record.");
            parse_cur_record(rec_entry, cur_pos);
            return true;
        }
        return false;
    }
    else {
        while (1) {
            cur_pos = cur_pos.nil() ? RECORD_START : cur_pos.next(record_length);
            auto rel_entry = bg_rel.addr<RelationEntryData>();
            // 如果尚未到达所有记录尾部
            if (RecordPosition::cmp(cur_pos, rel_entry->empty) < 0) {
                BlockGuard bg_rec(block_mgr, Files::relation(rel.name), cur_pos.block_index);
                auto rec_entry = bg_rec.addr<RecordEntryData>(cur_pos.pos);
                // 如果当前条有效，那么找到了
                if (rec_entry->use) {
                    parse_cur_record(rec_entry, cur_pos);
                    return true;
                }
            }
            else {
                return false;
            }
        }
    }
}


FilterScanner::FilterScanner(unique_ptr<Scanner> from, unique_ptr<Expression> pred) : _from(move(from)), _pred(move(pred)) {
    if (_pred == nullptr) throw logic_error("Expression cannot be null.");
    _pred->resolve(_from->rel_out());
    if (_pred->type().tag != Type::Tag::INT) {
        throw logic_error("This expression cannot be a predicate.");
    }
}

bool FilterScanner::next() {
    while (_from->next()) {
        cur_record = move(_from->current());
        if (_pred->eval(cur_record).INT) {
            return true;
        }
    }
    return false;
}


ProjectScanner::ProjectScanner(unique_ptr<Scanner> from, vector<unique_ptr<Expression>> fields) : _from(move(from)), _fields(move(fields)) {
    resolve(_from->rel_out());
    for (int i = 0; i < _fields.size(); i++) {
        auto& f = _fields[i];
        _rel_out.fields.push_back(Field(f->name, f->type()));
    }
    _rel_out.update();
}

bool ProjectScanner::next() {
    if (_from->next()) {
        _cur_record = Record();
        for (auto& f : _fields) {
            _cur_record.values.push_back(f->eval(_from->current()));
        }
        return true;
    }
    return false;
}
