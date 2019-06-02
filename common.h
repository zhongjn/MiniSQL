#pragma once

#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include "files.h"
#include "block_manager.h"

using namespace std;

const int NAME_LENGTH = 32;
const int MAX_FIELDS = 32;
const int MAX_RELATIONS = 32;
const int MAX_RECORD_LENGTH = 1024;



struct Type {
    enum class Tag {
        INT,
        CHAR,
        FLOAT
    } tag;
    // Type type; // tagged union
    union {
        struct {
            int len;
        } CHAR;
    };
    int length() const;
    static Type create_INT() { Type t; t.tag = Tag::INT; return t; }
    static Type create_FLOAT() { Type t; t.tag = Tag::FLOAT; return t; }
    static Type create_CHAR(int len) { Type t; t.tag = Tag::CHAR; t.CHAR.len = len; return t; }
};

struct FieldData {
    char name[NAME_LENGTH] = { 0 }; // ����
    bool unique;
    bool primary_key;
    Type type;
};

struct Field {
    string name;
    int offset;
    bool unique = false;
    bool primary_key = false;
    Type type;

    FieldData to_file() const;
    void from_file(const FieldData& f);
    Field() = default;
    Field(string name, Type type) : name(move(name)), type(type) {};
};

struct RelationData {
    int field_count;
    FieldData fields[MAX_FIELDS];
    // TODO: ������Ϣ
};
static_assert(sizeof(RelationData) <= BLOCK_SIZE, "RelationData cannot be contained by a single block. Consider reorganize data.");

struct Relation {
    string name;
    vector<Field> fields;
    void update();
    int record_length() const;
    RelationData to_file() const;
    void from_file(const RelationData& f);
    Relation() = default;
    Relation(string name) : name(move(name)) {};
    // Relation(Relation&& rel) = default;
};

// ���ݿ����ļ��еı�ʾ��������ڶ������ļ��С�
struct DatabaseData {
    char rel_names[MAX_RELATIONS][NAME_LENGTH] = { 0 }; // �洢relation����rel_names[i]��relation�������ڱ��ļ���i+1�����С�
    int get_block_index(const char* relation_name) const;
    int get_free_block_index() const;
    int block_to_rel(int i) { return i - 1; }
    int rel_to_block(int i) { return i + 1; }
};
static_assert(sizeof(DatabaseData) <= BLOCK_SIZE, "DatabaseData cannot be contained by a single block. Consider reorganize data.");

struct RecordPosition {
    int block_index = -1;
    int pos = -1;
    RecordPosition() = default;
    RecordPosition(int block_index, int pos) : block_index(block_index), pos(pos) {};
    bool nil() { return block_index < 0 || pos < 0; }

    static int cmp(RecordPosition p1, RecordPosition p2) {
        if (p1.nil() || p2.nil()) throw logic_error("Cannot compare nil value.");
        if (p1.block_index != p2.block_index) return p1.block_index - p2.block_index;
        return p1.pos - p2.pos;
    }

    RecordPosition next(int record_len) const {
        RecordPosition n = *this;
        n.pos += record_len;
        if (BLOCK_SIZE - n.pos <= record_len) {
            n.block_index++;
            n.pos = 0;
        }
        return n;
    }
};
const RecordPosition RECORD_START = RecordPosition(0, 2048);

struct RelationEntryData {
    bool deleted = false;
    RecordPosition free_head;
    RecordPosition empty;
};

// ����������һ����ʹ���У�һ���ǿ���
struct RecordEntryData {
    bool use;
    RecordPosition free_next;
    uint8_t values[0];
};

struct Value {
    union {
        int INT = 0;
        float FLOAT;
    };
    string CHAR;
    void write(void* addr, Type type) const;
    void parse(void* addr, Type type);
    Value& operator=(Value&& v) noexcept {
        if (&v != this) {
            INT = v.INT;
            CHAR = move(v.CHAR);
        }
        return *this;
    }
    Value() = default;
    Value(const Value& v) {
        INT = v.INT;
        CHAR = v.CHAR;
    }
    Value(Value&& v) noexcept {
        *this = move(v);
    }
    static Value create_INT(int i) {
        Value v;
        v.INT = i;
        return v;
    }
    static Value create_FLOAT(float f) {
        Value v;
        v.FLOAT = f;
        return v;
    }
    static Value create_CHAR(string s) {
        Value v;
        v.CHAR = move(s);
        return v;
    }
};

struct Record {
    vector<Value> values;
    Record() = default;
    Record(Record&& rec) noexcept {
        *this = move(rec);
    }
    Record& operator=(Record&& rec) noexcept {
        if (&rec != this) {
            values = move(rec.values);
        }
        return *this;
    }
};

class IndexIterator : public iterator<forward_iterator_tag, RecordPosition> {

};

template<typename To, typename From>
unique_ptr<To> static_cast_unique_ptr(unique_ptr<From>&& old) {
    return unique_ptr<To>{static_cast<To*>(old.release())};
    //conversion: unique_ptr<FROM>->FROM*->TO*->unique_ptr<TO>
}