#pragma once
#include <typeinfo>
#include "query_parser.h"
#include "scanner.h"
#include "record_manager.h"
#include "catalog_manager.h"
#include <unordered_map>

class QueryExecutor {
    CatalogManager* _cm;
    RecordManager* _rm;

    unique_ptr<Scanner> select_scanner(SelectStatement* stmt);
    Table select_exe(SelectStatement* stmt);

public:
    QueryExecutor(CatalogManager* cm, RecordManager* rm) : _cm(cm), _rm(rm) {};
    Table execute(unique_ptr<Statement> stmt);
};