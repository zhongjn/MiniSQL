#include <stdio.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <typeinfo>
#include "query_lexer.h"
#include "query_parser.h"
#include "query_executor.h"
#include "console_util.h"


using namespace std;


void match(const regex& r, const char* str) {
    match_results<const char*> mr;
    while (regex_search(str, mr, r)) {
        str = mr.suffix().first;
    }
}

int main(void)
{
	StorageEngine eng;
	QueryExecutor executor = QueryExecutor(&eng);
	string str;
	stringstream ss_expr;
	char c;
	cout << "MiniSQL v1.0" << endl;
	while (true)
	{
		str = "";
		cout << ">>>";
		ss_expr = stringstream();
		while (true)
		{
			c = getchar();
			if (c == ';')
			{
				break;
			}
			if (c == '\n')
			{
				cout << "-->";
			}
			ss_expr << c;
		}
		ss_expr >> str;
		if (str.size() == 0)
		{
			continue;
		}
		if (str == "exit")
		{
			break;
		}
		else if (str == "exe")
		{
			if (ss_expr.eof())
			{
				cout << "Please specify the file address." << endl;
			}
			else
			{
				ss_expr >> str;
				cout << "Executing file: " << str << endl;
				execute_file(executor, str);
			}
		}
		else
		{
			str = ss_expr.str();
			cout << "Executing command: " << str << endl;
			execute_expr(executor, str);
		}
		while (getchar() != '\n');
	}

	return 0;
}
