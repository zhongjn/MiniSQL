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
    // if (index_name.length() >= NAME_LENGTH) throw logic_error("Exceeded maximum index name length");

    FieldData f;
    strcpy(f.name, name.c_str());
    f.type = type;
    f.unique = unique;
    f.has_index = has_index;
    // strcpy(f.index_name, index_name.c_str());
    return f;
}

void Field::from_file(const FieldData & f) {
    name = f.name;
    type = f.type;
    unique = f.unique;
    has_index = f.has_index;
    // index_name = f.index_name;
}

void Relation::update() {
    int len = 0;
    indexes.clear();
    int i = 0;
    for (auto& f : fields) {
        f.offset = len;
        len += f.type.length();
        if (f.has_index) {
            indexes.push_back(i);
        }
        i++;
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
    for (int i = 0; i < f.field_count; i++) {
        Field fld;
        fld.from_file(f.fields[i]);
        fields.push_back(fld);
    }
    update();
}

void IndexLocation::from_file(const IndexNameLocationData& f) {
    field = f.field;
    relation_name = f.relation_name;
}

int DatabaseData::get_block(const char* relation_name) const {
    for (int i = 0; i < MAX_RELATIONS; i++) {
        if (strcmp(relation_name, rel_names[i]) == 0) {
            return i + 1;
        }
    }
    return -1; // not found
}

int DatabaseData::get_free_block() const {
    for (int i = 0; i < MAX_RELATIONS; i++) {
        if (rel_names[i][0] == 0) {
            return i + 1;
        }
    }
    return -1;
}

int DatabaseData::get_index(const char* index_name) const {
    for (int i = 0; i < MAX_INDEXES; i++) {
        if (strcmp(index_name, indexes[i].index_name) == 0) {
            return i;
        }
    }
    return -1; // not found
}

int DatabaseData::get_free_index() const {
    for (int i = 0; i < MAX_INDEXES; i++) {
        if (indexes[i].index_name[0] == 0) {
            return i;
        }
    }
    return -1; // not found
}

bool Value::greater_than(const struct Value& x, Type type)
{
	using Tag = Type::Tag;
	switch (type.tag)
	{
	case Tag::INT:
	{
		if (this->INT > x.INT)
			return true;
		else return false;
	}
	case Tag::CHAR:
	{
		if (this->CHAR > x.CHAR)
			return true;
		return false;
	}
	default:
	{
		if (this->FLOAT > x.FLOAT)
			return true;
		return false;
	}
	}
	return true;
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

string string_format(const string fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
    unique_ptr<char[]> formatted;
    va_list ap;
    while (1) {
        formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return string(formatted.get());
}
