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

    unique_ptr<Scanner> select_scanner(SelectStatement* stmt) {
        unique_ptr<Scanner> sc;
        Relation rel_from;
        if (!stmt->from) {
            vector<Record> v;
            v.push_back(Record());
            sc = unique_ptr<Scanner>(new ContainerScanner<vector<Record>>(move(v)));
        }
        else {
            if (stmt->from->type == SelectSource::Type::physical) {
                // TODO: index
                auto ph_rel = _cm->get_relation(stmt->from->physical);
                if (!ph_rel) throw logic_error("relation not found");
                sc = _rm->select_record(ph_rel.value(), nullptr);
            }
            else {
                sc = select_scanner(stmt->from->subquery.get());
            }
        }

        if (stmt->where) {
            sc = unique_ptr<Scanner>(new FilterScanner(move(sc), move(stmt->where)));
        }

        vector<unique_ptr<Expression>> fields;
        for (auto& sf : stmt->select) {
            fields.push_back(move(sf.expr));
        }

        return unique_ptr<Scanner>(new ProjectScanner(move(sc), move(fields)));
    }

    Table select_exe(SelectStatement* stmt) {
        Table t;
        auto sc = select_scanner(stmt);
        t.relation = sc->rel_out();
        while (sc->next()) {
            t.records.push_back(move(sc->current()));
        }
        return t;
    }


public:
    QueryExecutor(CatalogManager* cm, RecordManager* rm) : _cm(cm), _rm(rm) {};

    Table execute(unique_ptr<Statement> stmt) {
        auto& tt = typeid(*stmt);
        if (tt == typeid(SelectStatement)) {
            return select_exe((SelectStatement*)stmt.get());
        }
    }
};