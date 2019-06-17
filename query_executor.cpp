#include "query_executor.h"
#include <stack>

bool QueryExecutor::value_less(Nullable<Type> t, Nullable<Value> v1, Nullable<Value> v2)
{
	bool ret = false;
	if (v1.null() || v2.null())
	{
		if (!v1.null() || !v2.null())
		{
			ret = true;
		}
	}
	else
	{
		if (t->tag == Type::Tag::INT)
		{
			ret = v1->INT < v2->INT;
		}
		else if (t->tag == Type::Tag::FLOAT)
		{
			ret = v1->FLOAT < v2->FLOAT;
		}
		else if (t->tag == Type::Tag::CHAR)
		{
			ret = v1->CHAR < v2->CHAR;
		}
	}
	return false;
}

Nullable<IndexUsage> QueryExecutor::search_index(BinaryExpression* exp, Relation& relation)
{
	vector<IndexUsage> candidates;
	stack<BinaryExpression*> s;
	BinaryExpression* pcur = exp;

	while (pcur || !s.empty())
	{
		// preorder traverse
		BinaryExpression::Operator op = pcur->get_op();
		if (op == BinaryExpression::Operator::OR)
		{
			// no index should be used if there is or
			candidates.clear();
			break;
		}
		else if (op == BinaryExpression::Operator::AND)
		{
			// eliminate AND in else
		}
		else
		{
			bool inverse = false;
			FieldExpression* ptr_field = NULL;
			ConstantExpression* ptr_val = NULL;
			int candidate_index = 0;
			if (typeid(*(pcur->get_r1())) == typeid(FieldExpression) &&
				typeid(*(pcur->get_r2())) == typeid(ConstantExpression))
			{
				ptr_field = dynamic_cast<FieldExpression*>(pcur->get_r1());
				ptr_val = dynamic_cast<ConstantExpression*>(pcur->get_r2());
			}
			else if (typeid(*(pcur->get_r2())) == typeid(FieldExpression) &&
				typeid(*(pcur->get_r1())) == typeid(ConstantExpression))
			{
				ptr_field = dynamic_cast<FieldExpression*>(pcur->get_r2());
				ptr_val = dynamic_cast<ConstantExpression*>(pcur->get_r1());
				inverse = true;
			}

			for (candidate_index = 0; candidate_index < candidates.size(); ++candidate_index)
			{
				if (candidates[candidate_index].field_index == ptr_field->get_field())
				{
					break;
				}
			}
			if (candidate_index == candidates.size())
			{
				candidates.push_back(IndexUsage{
					ptr_field->get_field(),
					Null(), Null(),
					false, false
				});
			}
			
			if (ptr_field)
			{
				if (op == BinaryExpression::Operator::GE)
				{
					if (inverse && value_less(ptr_val->type(), ptr_val->get_val(), candidates[candidate_index].to))
					{
						// a >= X
						candidates[candidate_index].to = ptr_val->get_val();
					}
					else if (value_less(ptr_val->type(), candidates[candidate_index].from, ptr_val->get_val()))
					{
						// X >= a
						candidates[candidate_index].from = ptr_val->get_val();
					}
				}
				else if (op == BinaryExpression::Operator::GT)
				{
					// exclusive ignored
					if (inverse && value_less(ptr_val->type(), ptr_val->get_val(), candidates[candidate_index].to))
					{
						// a > X
						candidates[candidate_index].to = ptr_val->get_val();
					}
					else if (value_less(ptr_val->type(), candidates[candidate_index].from, ptr_val->get_val()))
					{
						// X > a
						candidates[candidate_index].from = ptr_val->get_val();
					}
				}
				else if (op == BinaryExpression::Operator::LE)
				{
					if (inverse && value_less(ptr_val->type(), candidates[candidate_index].from, ptr_val->get_val()))
					{
						// a <= X
						candidates[candidate_index].from = ptr_val->get_val();
					}
					else if (value_less(ptr_val->type(), ptr_val->get_val(), candidates[candidate_index].to))
					{
						// X <= a
						candidates[candidate_index].to = ptr_val->get_val();
					}
				}
				else if (op == BinaryExpression::Operator::LT)
				{
					// exclusive ignored
					if (inverse && value_less(ptr_val->type(), candidates[candidate_index].from, ptr_val->get_val()))
					{
						// a <= X
						candidates[candidate_index].from = ptr_val->get_val();
					}
					else if (value_less(ptr_val->type(), ptr_val->get_val(), candidates[candidate_index].to))
					{
						// X <= a
						candidates[candidate_index].to = ptr_val->get_val();
					}
				}
				else if (op == BinaryExpression::Operator::EQ)
				{
					candidates[candidate_index].from = candidates[candidate_index].to = ptr_val->get_val();
				}
			}
			// op not AND
			pcur = NULL;
		}

		if (pcur)
		{
			s.push(pcur);
		}
		if (typeid(*(pcur->get_r1())) == typeid(BinaryExpression))
		{
			// search for left subtree
			pcur = dynamic_cast<BinaryExpression*>(pcur->get_r1());
		}
		else
		{
			pcur = NULL;
		}

		while (!pcur && !s.empty())
		{
			pcur = s.top();
			s.pop();
			if (typeid(*(pcur->get_r2())) == typeid(BinaryExpression))
			{
				// search for right subtree
				pcur = dynamic_cast<BinaryExpression*>(pcur->get_r2());
			}
			else
			{
				pcur = NULL;
			}
		}
	}

	Nullable<IndexUsage> ret = Null();
	for (int i = 0; i < candidates.size(); ++i)
	{
		if (relation.fields[candidates[i].field_index].has_index)
		{
			ret = candidates[i];
			break;
		}
	}
	return ret;
}

unique_ptr<Scanner> QueryExecutor::select_scanner(SelectStatement* stmt) {
    unique_ptr<Scanner> sc;
    if (!stmt->from) {
        vector<Record> v;
        v.push_back(Record());
        sc = unique_ptr<Scanner>(new ContainerScanner<vector<Record>>(move(v)));
    }
    else {
        if (stmt->from->type == SelectSource::Type::physical) {
            auto ph_rel = _storage_eng->get_relation(stmt->from->physical);
            if (!ph_rel) throw logic_error("relation not found");
			if (typeid(*(stmt->where.get())) == typeid(BinaryExpression))
			{
				// use index if binary expression
				sc = _storage_eng->select_record(ph_rel.value(),
					search_index(dynamic_cast<BinaryExpression*>(stmt->where.get()), ph_rel.value()));
			}
			else
			{
				// don't if unary or else
				sc = _storage_eng->select_record(ph_rel.value(), Null());
			}
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

QueryResult QueryExecutor::insert_exe(InsertStatement* stmt){
	QueryResult q;
	Nullable<Relation> relation = _storage_eng->get_relation(stmt->into);
	if (!relation)
	{
		throw logic_error("Relation not found.");
	}
	else if (relation->fields.size() < stmt->fields.size())
	{
		throw logic_error("Statement field size mismatch.");
	}
	Record r;
	for (pair<Type, Value> p : stmt->values)
	{
		r.values.push_back(p.second);
	}
	_storage_eng->insert_record(relation.value(), move(r));
	q.prompt = string_format("Query OK. 1 row inserted.");
    return q;
}

QueryResult QueryExecutor::delete_exe(DeleteStatement* stmt){
	unique_ptr<Scanner> sc;
	Nullable<Relation> relation = _storage_eng->get_relation(stmt->relation);
	int count = 0;
	if (!relation)
	{
		throw logic_error("relation not found");
	}
	// index			
	if (typeid(*(stmt->where.get())) == typeid(BinaryExpression))
	{
		// use index if binary expression
		sc = _storage_eng->select_record(relation.value(),
			search_index(dynamic_cast<BinaryExpression*>(stmt->where.get()), relation.value()));
	}
	else
	{
		// don't if unary or else
		sc = _storage_eng->select_record(relation.value(), Null());
	}
	if (stmt->where)
	{
		sc = unique_ptr<Scanner>(new FilterScanner(move(sc), move(stmt->where)));
	}

	vector<Record> records;
	while (sc->next())
	{
		records.push_back(move(sc->current()));
		++count;
	}
	for (int i = 0; i < records.size(); ++i)
	{
		_storage_eng->delete_record(relation.value(), records[i]);
	}
	QueryResult q;
	q.prompt = string_format("Query OK. %d rows deleted", count);
    return q;
}

QueryResult QueryExecutor::update_exe(UpdateStatement* stmt) {
	unique_ptr<Scanner> sc;
	Nullable<Relation> relation = _storage_eng->get_relation(stmt->relation);
	int count = 0;
	if (!relation)
	{
		throw logic_error("relation not found");
	}
	// index			
	if (typeid(*(stmt->where.get())) == typeid(BinaryExpression))
	{
		// use index if binary expression
		sc = _storage_eng->select_record(relation.value(),
			search_index(dynamic_cast<BinaryExpression*>(stmt->where.get()), relation.value()));
	}
	else
	{
		// don't if unary or else
		sc = _storage_eng->select_record(relation.value(), Null());
	}
	if (stmt->where)
	{
		sc = unique_ptr<Scanner>(new FilterScanner(move(sc), move(stmt->where)));
	}

	// waste too much space, need improvement
	vector<Record> records;
	while (sc->next())
	{
		records.push_back(move(sc->current()));
		++count;
	}
	for (int i = 0; i < records.size(); ++i)
	{
		_storage_eng->delete_record(relation.value(), records[i]);
	}
	QueryResult q;
	q.prompt = string_format("Query OK. %d rows deleted", count);
	return q;
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
