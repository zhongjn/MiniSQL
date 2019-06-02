#pragma once
#include <regex>
#include <vector>
#include <string>
#include <stdexcept>
#include "nullable.h"

using namespace std;

enum class TokenType {
    literal,
    keyword,
    op,
    identifier,
    punctuation
};

struct Token {
    TokenType type;
    string content;
};

struct TokenRegex {
    TokenType type;
    regex regex;
};

class QueryLexer {
public:
    vector<Token> tokenize(const string& str);
};