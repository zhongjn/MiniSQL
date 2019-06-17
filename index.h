#pragma once
#include "common.h"
#include "nullable.h"
#include "block_manager.h"
// TODO: B-Tree���
class IndexIterator {
    Type _value_type;
    Nullable<Value> _start, _end;
public:
    bool next() { throw "not implemented"; }
    RecordPosition current() { throw "not implemented"; }
};

struct IndexUsage {
	int field_index;
	Nullable<Value> from, to;
	bool from_exclusive, to_exclusive;
};