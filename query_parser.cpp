#include "query_parser.h"

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
