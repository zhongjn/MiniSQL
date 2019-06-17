#include <stdio.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <typeinfo>
#include "query_lexer.h"
#include "query_parser.h"
#include "query_executor.h"


using namespace std;


void match(const regex& r, const char* str) {
    match_results<const char*> mr;
    while (regex_search(str, mr, r)) {
        str = mr.suffix().first;
    }
}

QueryResult execute_expr(QueryExecutor& executor, string expr);
void disp_records(QueryResult& result);

int main(void)
{
	QueryExecutor executor = QueryExecutor(new StorageEngine());
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
				ifstream ifs = ifstream(str, ios::in);
				if (!ifs.good())
				{
					cout << "Fail to open file" << endl;
				}
				else
				{
					string temp;
					while (true)
					{
						ss_expr = stringstream();
						while (true)
						{
							c = ifs.get();
							if (ifs.eof())
							{
								break;
							}
							if (c == ';')
							{
								break;
							}
							ss_expr << c;
						}
						if (ifs.eof())
						{
							break;
						}
						cout << "Executing command: " << ss_expr.str() << endl;
						QueryResult result = execute_expr(executor, ss_expr.str());
						if (result.relation.fields.size() != 0)
						{
							// Show select results
							disp_records(result);
						}
						cout << result.prompt << endl << endl;
					}
				}
			}
		}
		else
		{
			str = ss_expr.str();
			cout << "Executing command: " << str << endl;
			QueryResult result = execute_expr(executor, str);
			if (result.relation.fields.size() != 0)
			{
				// Show select results
				disp_records(result);
			}
			cout << result.prompt << endl << endl;
		}
		while (getchar() != '\n');
	}

	return 0;
}

QueryResult execute_expr(QueryExecutor& executor, string expr)
{
	return executor.execute(QueryParser().parse(QueryLexer().tokenize(expr)));
}

void disp_records(QueryResult& result)
{

}
