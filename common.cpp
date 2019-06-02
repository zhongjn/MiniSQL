#include "common.h"

int Type::length() const {
    switch (tag)
    {
        case Tag::INT:
        case Tag::FLOAT: return 4;
        case Tag::CHAR: return CHAR.len;
        default: throw logic_error("Unexpected type.");
    }
}


FieldData Field::to_file() const {
    if (name.length() >= NAME_LENGTH) throw logic_error("Exceeded maximum name length");

    FieldData f;
    strcpy(f.name, name.c_str());
    f.primary_key = primary_key;
    f.type = type;
    f.unique = unique;
    return f;
}

void Field::from_file(const FieldData & f) {
    name = f.name;
    primary_key = f.primary_key;
    type = f.type;
    unique = f.unique;
}

void Relation::update() {
    int len = 0;
    for (auto& f : fields) {
        f.offset = len;
        len += f.type.length();
    }
}

int Relation::record_length() const {
    int len = 0;
    for (auto& f : fields) {
        len += f.type.length();
    }
    return sizeof(RecordEntryData) + len;
}

RelationData Relation::to_file() const {
    if (name.length() >= NAME_LENGTH) throw logic_error("Exceeded maximum name length");

    RelationData f;
    f.field_count = fields.size();
    // strcpy(f.name, name.c_str());

    for (int i = 0; i < f.field_count; i++) {
        f.fields[i] = fields[i].to_file();
    }
    return f;
}

void Relation::from_file(const RelationData& f) {
    fields.clear();
    int pos = 0;
    for (int i = 0; i < f.field_count; i++) {
        Field fld;
        fld.from_file(f.fields[i]);
        fld.offset = pos;
        fields.push_back(fld);
        pos += fld.type.length();
    }
}



int DatabaseData::get_block_index(const char* relation_name) const {
    for (int i = 0; i < MAX_RELATIONS; i++) {
        if (strcmp(relation_name, rel_names[i]) == 0) {
            return i + 1;
        }
    }
    return -1; // not found
}

int DatabaseData::get_free_block_index() const {
    for (int i = 0; i < MAX_RELATIONS; i++) {
        if (rel_names[i][0] == 0) {
            return i + 1;
        }
    }
    return -1;
}

void Value::write(void* addr, Type type) const {
    using Tag = Type::Tag;
    switch (type.tag) {
        case Tag::INT: {
            *((int*)addr) = INT; 
            break;
        }
        case Tag::FLOAT: {
            *((float*)addr) = FLOAT; 
            break;
        }
        case Tag::CHAR: {
            int len_limit = type.CHAR.len;
            if (CHAR.length() >= len_limit)
                throw logic_error("String too long.");
            strcpy((char*)addr, CHAR.c_str());
            break;
        }
        default: break;
    }
}

void Value::parse(void* addr, Type type){
    using Tag = Type::Tag;
    switch (type.tag) {
        case Tag::INT: {
            INT = *((int*)addr);
            break;
        }
        case Tag::FLOAT: {
            FLOAT = *((float*)addr);
            break;
        }
        case Tag::CHAR: {
            CHAR = string((char*)addr);
            break;
        }
        default: break;
    }
}
