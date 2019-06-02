#include <stdio.h>
#include <iostream>
#include <regex>
#include <typeinfo>
#include "query_lexer.h"


using namespace std;


void match(const regex& r, const char* str) {
    match_results<const char*> mr;
    while (regex_search(str, mr, r)) {
        str = mr.suffix().first;
    }
}

struct A {
    int t;
    // virtual void f() {}
};

struct B : public A {
    int tt;
    // void f() {}
};

int main()
{
    auto tokens = QueryLexer().tokenize("select count(*) from test;");
    A* test = new B();
    auto& tt = typeid(int);
    auto& tt2 = typeid(*test);
    auto n = tt2.name();
    return 0;
}