#include "query_lexer.h"

using namespace std::regex_constants;

static TokenRegex regexes[] = {
    TokenRegex{TokenType::literal, regex("^(?:[0-9]+(?:.[0-9]+)?)|^'((?:''|[^'])*)'")}, // TODO: string
    TokenRegex{TokenType::keyword, regex("^(?:select|delete|update|insert|from|where|and|or|is|unique)")},
    TokenRegex{TokenType::op, regex("^(?:\\+|-|\\*|/|>=|<=|>|<|==|!=|=)")},
    TokenRegex{TokenType::identifier, regex("^[a-zA-Z_][a-zA-Z_0-9]*")},
    TokenRegex{TokenType::punctuation, regex("^[.,;\\*\\(\\)]")},
};

static regex regex_space("^\\s+");

vector<Token> QueryLexer::tokenize(const string& str) {
    vector<Token> tokens;
    int cursor = 0;
    const char* cur_str = str.c_str();
    while (cursor < str.length()) {
        Nullable<TokenType> cur_type;
        match_results<const char*> cur_match;

        // discard spaces
        if (regex_search(str.c_str() + cursor, cur_match, regex_space)) {
            cursor += cur_match.length();
        }

        for (auto& tr : regexes) {
            if (regex_search(str.c_str() + cursor, cur_match, tr.regex)) {
                cur_type = tr.type;
                cursor += cur_match.length();
                break;
            }
        }
        if (!cur_type) throw logic_error("Unsuccessful match.");
        tokens.push_back(Token{ cur_type.value(), cur_match.str() });
    }
    return tokens;
}
