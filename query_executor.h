#pragma once
#include <typeinfo>
#include "query_parser.h"
#include "storage_engine.h"
#include <unordered_map>

struct QueryResult {
    Relation relation;
    vector<Record> records;
    string prompt;
};

class QueryExecutor {
    //CatalogManager* _cm;
    //RecordManager* _rm;
    StorageEngine* _storage_eng;

    unique_ptr<Scanner> select_scanner(SelectStatement* stmt);
    QueryResult select_exe(SelectStatement* stmt);
    QueryResult update_exe(UpdateStatement* stmt);
    QueryResult insert_exe(InsertStatement* stmt);
    QueryResult delete_exe(DeleteStatement* stmt);

public:
    QueryExecutor(StorageEngine* storage_eng) : _storage_eng(storage_eng) {};
    QueryResult execute(unique_ptr<Statement> stmt);
};