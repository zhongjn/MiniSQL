#pragma once

#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include <stdarg.h>
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
    char name[NAME_LENGTH] = { 0 }; // 名字
    bool unique;
    bool has_index;
    char index_name[NAME_LENGTH] = { 0 };
    Type type;
};

struct Field {
    string name;
    int offset;
    bool unique = false;
    // bool primary_key = false;
    bool has_index = false;
    string index_name;
    Type type;

    FieldData to_file() const;
    void from_file(const FieldData& f);
    Field() = default;
    Field(string name, Type type) : name(move(name)), type(type) {};
};

struct RelationData {
    int field_count;
    FieldData fields[MAX_FIELDS];
    // TODO: 索引信息
};
static_assert(sizeof(RelationData) <= BLOCK_SIZE, "RelationData cannot be contained by a single block. Consider reorganize data.");

struct Relation {
    string name;
    vector<Field> fields;
    vector<int> indexes;
    void update();
    int record_length() const;
    RelationData to_file() const;
    void from_file(const RelationData& f);
    Relation() = default;
    Relation(string name) : name(move(name)) {};
    // Relation(Relation&& rel) = default;
};

// 数据库在文件中的表示。必须放在独立的文件中。
struct DatabaseData {
    char rel_names[MAX_RELATIONS][NAME_LENGTH] = { 0 }; // 存储relation名。rel_names[i]的relation，将处于本文件第i+1个块中。
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
    bool nil() const { return block_index < 0 || pos < 0; }

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

// 两个链表，一个是使用中，一个是空闲
struct RecordEntryData {
    bool use;
    RecordPosition free_next;
    uint8_t values[0];
};

struct Value {
private:
	template<typename T>
	static int cmp(T t1, T t2) {
		if (t1 < t2) return -1;
		if (t1 > t2) return 1;
		return 0;
	}
public:
    union {
        int INT = 0;
        float FLOAT;
    };
    string CHAR;
	bool greater_than(const Value& x, Type type);
	static int cmp(Type type, const Value& v1, const Value& v2)
	{
		switch (type.tag)
		{
		case Type::Tag::INT: return cmp(v1.INT, v2.INT);
		case Type::Tag::FLOAT: return cmp(v1.FLOAT, v2.FLOAT);
		case Type::Tag::CHAR: return cmp(v1.CHAR, v2.CHAR);
		default: throw logic_error("Unexpected type.");
		}
	}
    void write(void* addr, Type type) const;
    void parse(void* addr, Type type);
    Value& operator=(Value&& v) noexcept {
        if (&v != this) {
            INT = v.INT;
            CHAR = move(v.CHAR);
        }
        return *this;
    }
    Value& operator=(const Value& v) {
        INT = v.INT;
        CHAR = v.CHAR;
        return *this;
    }
    Value() = default;
    Value(const Value& v) {
        *this = v;
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
    RecordPosition physical_position;
    //Record() = default;
    //Record(Record&& rec) noexcept {
    //    *this = move(rec);
    //}
    //Record& operator=(Record&& rec) noexcept {
    //    if (&rec != this) {
    //        values = move(rec.values);
    //    }
    //    return *this;
    //}
};

template<typename To, typename From>
inline unique_ptr<To> static_cast_unique_ptr(unique_ptr<From>&& old) {
    return unique_ptr<To>{static_cast<To*>(old.release())};
}

string string_format(const string fmt_str, ...);