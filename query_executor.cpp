#include "query_executor.h"

unique_ptr<Scanner> QueryExecutor::select_scanner(SelectStatement* stmt) {
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
            auto ph_rel = _storage_eng->get_relation(stmt->from->physical);
            if (!ph_rel) throw logic_error("relation not found");
            sc = _storage_eng->select_record(ph_rel.value(), Null());
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

QueryResult QueryExecutor::select_exe(SelectStatement* stmt) {
    QueryResult t;
    auto sc = select_scanner(stmt);
    t.relation = sc->rel_out();
    while (sc->next()) {
        t.records.push_back(move(sc->current()));
    }
    t.prompt = string_format("Query OK. %d row(s) in set.", t.records.size());
    return t;
}

QueryResult QueryExecutor::update_exe(UpdateStatement* stmt){
    return QueryResult();
}

QueryResult QueryExecutor::insert_exe(InsertStatement* stmt){
    return QueryResult();
}

QueryResult QueryExecutor::delete_exe(DeleteStatement* stmt){
    return QueryResult();
}


QueryResult QueryExecutor::execute(unique_ptr<Statement> stmt) {
    auto& tt = typeid(*stmt);
    if (tt == typeid(SelectStatement)) {
        return select_exe((SelectStatement*)stmt.get());
    }
    if (tt == typeid(UpdateStatement)) {
        return update_exe((UpdateStatement*)stmt.get());
    }
    if (tt == typeid(InsertStatement)) {
        return insert_exe((InsertStatement*)stmt.get());
    }
    if (tt == typeid(DeleteStatement)) {
        return delete_exe((DeleteStatement*)stmt.get());
    }
}
