#include "query_parser.h"

void QueryParser::assert(bool p) {
    if (!p) {
        stringstream ss;
        ss << "Syntax error. pos=" << _pos;
        throw logic_error(ss.str());
    }
}

bool QueryParser::consume(Token& match, initializer_list<const char*> contents) {
    if (_pos >= _tokens.size()) return false;
    for (auto str : contents)
    {
        if (_tokens[_pos].content == str)
        {
            match = _tokens[_pos];
            _pos++;
            return true;
        }
    }
    return false;
}

bool QueryParser::consume(Token& match, initializer_list<TokenType> types) {
    if (_pos >= _tokens.size()) return false;
    for (auto t : types)
    {
        if (_tokens[_pos].type == t)
        {
            match = _tokens[_pos];
            _pos++;
            return true;
        }
    }
    return false;
}

BinaryExpression::Operator QueryParser::binary_op(const string& op) {
    using Op = BinaryExpression::Operator;
    static unordered_map<string, Op> lookup = {
        { "==", Op::EQ },
        { "=", Op::EQ },
        { "is", Op::EQ },
        { "!=", Op::NE },
        { "<", Op::LT },
        { ">", Op::GT },
        { "<=", Op::LE },
        { ">=", Op::GE },
        { "+", Op::ADD },
        { "-", Op::SUB },
        { "*", Op::MUL },
        { "/", Op::DIV },
        { "and", Op::AND },
        { "or", Op::OR }
    };
    return lookup.at(op);
}

pair<Type, Value> QueryParser::literal(const string& lit) {
    if (lit.front() == '\'' && lit.back() == '\'') {
        // string
        string value = lit.substr(1, lit.length() - 2);
        return pair<Type, Value>(Type::create_CHAR(value.length() + 1), Value::create_CHAR(move(value)));
    }
    else if (lit.find('.', 0) != string::npos) {
        // float
        float value;
        stringstream ss(lit);
        ss >> value;
        return pair<Type, Value>(Type::create_FLOAT(), Value::create_FLOAT(value));
    }
    else {
        // int
        int value;
        stringstream ss(lit);
        ss >> value;
        return pair<Type, Value>(Type::create_INT(), Value::create_INT(value));
    }
}


bool QueryParser::left_binary_op_seq(ExprGen child, unique_ptr<Expression> & expr, initializer_list<const char*> ops) {
    int save = _pos;
    if (reset(save) && (this->*child)(expr)) {
        Token op;
        while (consume(op, ops)) {
            unique_ptr<Expression> next;
            assert((this->*child)(next));
            expr = unique_ptr<Expression>(new BinaryExpression(binary_op(op.content), move(expr), move(next)));
        }
        return true;
    }
    return false;
}

bool QueryParser::factor(unique_ptr<Expression> & expr) {
    int save = _pos;
    Token match;
    if (reset(save) && consume(match, TokenType::literal)) {
        auto lit = literal(match.content);
        expr = unique_ptr<Expression>(new ConstantExpression(lit.first, lit.second));
        return true;
    }
    else if (reset(save) && consume(match, TokenType::identifier)) {
        expr = unique_ptr<Expression>(new FieldExpression(match.content));
        return true;
    }
    else if (reset(save) && consume("(") && expression(expr) && consume(")")) {
        return true;
    }
    return false;
}

bool QueryParser::uniary(unique_ptr<Expression> & expr) {
    if (consume("-") && factor(expr)) {
        expr = unique_ptr<Expression>(new UniaryExpression(UniaryExpression::Operator::NEG, move(expr)));
        return true;
    }
    return factor(expr);
}


bool QueryParser::string_identifier(string & string)
{
    Token t;
    if (consume(t, TokenType::identifier)) {
        string = t.content;
        return true;
    }
    return false;
}

bool QueryParser::insert_value(pair<Type, Value> & value)
{
    Token t;
    assert(consume(t, TokenType::literal));
    value = literal(t.content);
    return true;
}

bool QueryParser::insert_list(vector<string> & fields)
{
    bool first = true;
    while (1) {
        if (first || consume(",")) {
            string f;
            assert(string_identifier(f));
            fields.push_back(move(f));
        }
        else {
            break;
        }
        first = false;
    }
    return fields.size() > 0;
}

bool QueryParser::insert_list(vector<pair<Type, Value>> & values)
{
    bool first = true;
    while (1) {
        if (first || consume(",")) {
            pair<Type, Value> v;
            assert(insert_value(v));
            values.push_back(move(v));
        }
        else {
            break;
        }
        first = false;
    }
    return values.size() > 0;
}

bool QueryParser::insert_stmt_inner(unique_ptr<InsertStatement> & stmt)
{
    int save = _pos;
    if (reset(save) && consume("insert") && consume("into")) {
        string into;
        vector<string> fields;
        vector<pair<Type, Value>> values;

        assert(string_identifier(into));
        if (consume("(") && insert_list(fields) && consume(")")) {
            stmt->fields = move(fields);
        }
        assert(consume("values"));
        assert(consume("(") && insert_list(values) && consume(")"));

        stmt = unique_ptr<InsertStatement>(new InsertStatement());
        stmt->into = move(into);
        stmt->values = move(values);
        return true;
    }
    return false;
}

bool QueryParser::insert_stmt(unique_ptr<Statement> & stmt)
{
    unique_ptr<InsertStatement> insert;
    if (insert_stmt_inner(insert)) {
        stmt = static_cast_unique_ptr<Statement>(move(insert));
        return true;
    }
    return false;
}

bool QueryParser::select_field(SelectField & f) {
    int start, end;
    assert(expression(f.expr, start, end));

    bool as = consume("as");
    Token t_alias;
    if (consume(t_alias, TokenType::identifier)) {
        f.expr->name = t_alias.content;
    }
    else {
        assert(!as);
        stringstream ss_name;
        for (int i = start; i < end; i++) {
            ss_name << _tokens[i].content;
        }
        f.expr->name = ss_name.str();
    }

    return true;
}

bool QueryParser::select_list(vector<SelectField> & fields) {
    bool first = true;
    while (1) {
        if (first || consume(",")) {
            SelectField f;
            assert(select_field(f));
            fields.push_back(move(f));
        }
        else {
            break;
        }
        first = false;
    }
    return fields.size() > 0;
}

bool QueryParser::select_source(SelectSource & source) {
    int save = _pos;
    if (consume("(") && select_stmt_inner(source.subquery) && consume(")")) {
        source.type = SelectSource::Type::subquery;
        return true;
    }
    Token rel_physical;
    if (consume(rel_physical, TokenType::identifier)) {
        source.type = SelectSource::Type::physical;
        source.physical = rel_physical.content;
        return true;
    }
    return false;
}

bool QueryParser::select_stmt_inner(unique_ptr<SelectStatement> & stmt) {
    int save = _pos;
    if (reset(save) && consume("select")) {
        Nullable<vector<SelectField>> select;
        unique_ptr<Expression> where;
        Nullable<SelectSource> from;
        if (!consume("*")) {
            select = vector<SelectField>();
            select_list(select.value());
        }
        if (consume("from")) {
            from = SelectSource();
            assert(select_source(from.value()));
        }
        if (consume("where")) {
            assert(expression(where));
        }
        stmt = unique_ptr<SelectStatement>(new SelectStatement());
        stmt->select = move(select);
        stmt->from = move(from);
        stmt->where = move(where);
        return true;
    }
    return false;
}

bool QueryParser::select_stmt(unique_ptr<Statement> & stmt) {
    unique_ptr<SelectStatement> select;
    if (select_stmt_inner(select)) {
        stmt = static_cast_unique_ptr<Statement>(move(select));
        return true;
    }
    return false;
}

bool QueryParser::update_field(UpdateField & f)
{
    Token t;
    assert(consume(t, TokenType::identifier));
    f.item = t.content;
    assert(consume("="));
    assert(expression(f.expr));
    return true;
}

bool QueryParser::update_list(vector<UpdateField> & fields)
{
    bool first = true;
    while (1) {
        if (first || consume(",")) {
            UpdateField f;
            assert(update_field(f));
            fields.push_back(move(f));
        }
        else {
            break;
        }
        first = false;
    }
    return fields.size() > 0;
}

bool QueryParser::update_stmt_inner(unique_ptr<UpdateStatement> & stmt)
{
    int save = _pos;
    if (reset(save) && consume("update")) {
        string table;
        vector<UpdateField> set;
        unique_ptr<Expression> where;

        assert(string_identifier(table));
        assert(consume("set"));
        assert(update_list(set));
        if (consume("where")) {
            assert(expression(where));
        }
        stmt = unique_ptr<UpdateStatement>(new UpdateStatement());
        stmt->table = move(table);
        stmt->set = move(set);
        stmt->where = move(where);
        return true;
    }
    return false;
}

bool QueryParser::update_stmt(unique_ptr<Statement> & stmt)
{
    unique_ptr<UpdateStatement> update;
    if (update_stmt_inner(update)) {
        stmt = static_cast_unique_ptr<Statement>(move(update));
        return true;
    }
    return false;
}

bool QueryParser::delete_stmt_inner(unique_ptr<DeleteStatement> & stmt)
{
    int save = _pos;
    if (reset(save) && consume("delete") && consume("from")) {
        string relation;
        unique_ptr<Expression> where;

        assert(string_identifier(relation));

        if (consume("where")) {
            assert(expression(where));
        }
        stmt = unique_ptr<DeleteStatement>(new DeleteStatement());
        stmt->relation = move(relation);
        stmt->where = move(where);
        return true;
    }
    return false;
}

bool QueryParser::delete_stmt(unique_ptr<Statement> & stmt)
{
    unique_ptr<DeleteStatement> delet;
    if (delete_stmt_inner(delet)) {
        stmt = static_cast_unique_ptr<Statement>(move(delet));
        return true;
    }
    return false;
}

bool QueryParser::create_table_field(CreateTableField & field)
{
    Token t;
    assert(consume(t, TokenType::identifier));
    field.name = t.content;
    assert(consume(t, TokenType::identifier));
    if (t.content == "int") {
        field.type = Type::create_INT();
    }
    else if (t.content == "float") {
        field.type = Type::create_FLOAT();
    }
    else if (t.content == "char") {
        Token t0;
        assert(consume("(") && consume(t0, TokenType::literal) && consume(")"));
        field.type = Type::create_CHAR(literal(t0.content).second.INT);
    }
    else {
        assert(false);
    }
    if (consume(t, TokenType::keyword)) {
        field.limit = t.content;
    }
    return true;
}

bool QueryParser::create_table_list(vector<CreateTableField> & fields)
{
    bool first = true;
    while (1) {
        if (first || (consume(",") && !consume("primary"))) {
            CreateTableField f;
            assert(create_table_field(f));
            fields.push_back(move(f));
        }
        else {
            break;
        }
        first = false;
    }
    return fields.size() > 0;
}

bool QueryParser::create_table_stmt_inner(unique_ptr<CreateTableStatement> & stmt)
{
    int save = _pos;
    if (reset(save) && consume("create") && consume("table")) {
        string table;
        vector<CreateTableField> fields;
        string key;

        assert(string_identifier(table));
        assert(consume("("));
        assert(create_table_list(fields));
        assert(consume("key") && consume("(") && string_identifier(key) && consume(")"));
        assert(consume(")"));

        stmt = unique_ptr<CreateTableStatement>(new CreateTableStatement());
        stmt->table = move(table);
        stmt->fields = move(fields);
        stmt->key = move(key);
        return true;
    }
    return false;
}

bool QueryParser::create_table_stmt(unique_ptr<Statement> & stmt)
{
    unique_ptr<CreateTableStatement> create_table;
    if (create_table_stmt_inner(create_table)) {
        stmt = static_cast_unique_ptr<Statement>(move(create_table));
        return true;
    }
    return false;
}

bool QueryParser::drop_table_stmt_inner(unique_ptr<DropTableStatement> & stmt)
{
    int save = _pos;
    if (reset(save) && consume("drop") && consume("table")) {
        Token t;
        assert(consume(t, TokenType::identifier));
        stmt = unique_ptr<DropTableStatement>(new DropTableStatement());
        stmt->table = move(t.content);
        return true;
    }
    return false;
}

bool QueryParser::drop_table_stmt(unique_ptr<Statement> & stmt)
{
    unique_ptr<DropTableStatement> drop_table;
    if (drop_table_stmt_inner(drop_table)) {
        stmt = static_cast_unique_ptr<Statement>(move(drop_table));
        return true;
    }
    return false;
}

bool QueryParser::create_index_stmt_inner(unique_ptr<CreateIndexStatement> & stmt)
{
    int save = _pos;
    if (reset(save) && consume("create") && consume("index")) {
        string indexname;
        string table;
        string attribution;
        assert(string_identifier(indexname));
        assert(consume("on"));
        assert(string_identifier(table));
        assert(consume("(") && string_identifier(attribution) && consume(")"));
        stmt = unique_ptr<CreateIndexStatement>(new CreateIndexStatement());
        stmt->indexname = move(indexname);
        stmt->table = move(table);
        stmt->attribution = move(attribution);
        return true;
    }
    return false;
}

bool QueryParser::create_index_stmt(unique_ptr<Statement> & stmt)
{
    unique_ptr<CreateIndexStatement> create_index;
    if (create_index_stmt_inner(create_index)) {
        stmt = static_cast_unique_ptr<Statement>(move(create_index));
        return true;
    }
    return false;
}

bool QueryParser::drop_index_stmt_inner(unique_ptr<DropIndexStatement> & stmt)
{
    int save = _pos;
    if (reset(save) && consume("drop") && consume("index")) {
        string indexname;
        assert(string_identifier(indexname));
        stmt = unique_ptr<DropIndexStatement>(new DropIndexStatement());
        stmt->indexname = move(indexname);
        return true;
    }
    return false;
}

bool QueryParser::drop_index_stmt(unique_ptr<Statement> & stmt)
{
    unique_ptr<DropIndexStatement> drop_index;
    if (drop_index_stmt_inner(drop_index)) {
        stmt = static_cast_unique_ptr<Statement>(move(drop_index));
        return true;
    }
    return false;
}

bool QueryParser::stmt(unique_ptr<Statement> & s) {
    int save = _pos;
    if (reset(save) && select_stmt(s)) {
        return true;
    }
    else if (reset(save) && update_stmt(s)) {
        return true;
    }
    else if (reset(save) && delete_stmt(s)) {
        return true;
    }
    else if (reset(save) && insert_stmt(s)) {
        return true;
    }
    else if (reset(save) && create_table_stmt(s)) {
        return true;
    }
    else if (reset(save) && drop_table_stmt(s)) {
        return true;
    }
    else if (reset(save) && create_index_stmt(s)) {
        return true;
    }
    else if (reset(save) && drop_index_stmt(s)) {
        return true;
    }
    return false;
}

unique_ptr<Statement> QueryParser::parse(vector<Token> tokens) {
    _tokens = move(tokens);
    _pos = 0;
    unique_ptr<Statement> s;
    assert(stmt(s));
    assert(_pos == _tokens.size());
    return s;
}
