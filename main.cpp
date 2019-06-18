#include <stdio.h>
#include <iostream>
#include <iomanip>
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
void draw_line(int* max, int size);

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
						str = ss_expr.str();
						cout << "Executing command: " << str << endl;
						try
						{
							QueryResult result = execute_expr(executor, str);
							if (result.relation.fields.size() != 0)
							{
								// Show select results
								disp_records(result);
							}
							cout << result.prompt << endl << endl;
						}
						catch (const std::exception& e)
						{
							cout << e.what() << endl;
						}
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
	int* max = new int[result.relation.fields.size()];
	for (int i = 0; i < result.relation.fields.size(); ++i)
	{
		max[i] = 2 + result.relation.fields[i].name.size();
		if (max[i] < 8)
		{
			max[i] = 8;
		}
	}
	for (int i = 0; i < result.records.size(); ++i)
	{
		for (int j = 0; j < result.relation.fields.size(); ++j)
		{
			if (result.relation.fields[j].type.tag == Type::Tag::CHAR)
			{
				if (max[j] < result.records[i].values[j].CHAR.size() + 2)
				{
					max[j] = result.records[i].values[j].CHAR.size() + 2;
				}
			}
		}
	}
	draw_line(max, result.relation.fields.size());
	for (int i = 0; i < result.relation.fields.size(); i++)
	{
		cout << "| " << setw(max[i]) << setiosflags(ios::left) << setfill(' ') << result.relation.fields[i].name << ' ';
	}
	cout << '|' << endl;
	draw_line(max, result.relation.fields.size());
	for (int i = 0; i < result.records.size(); i++)
	{
		for (int j = 0; j < result.relation.fields.size(); j++)
		{
			cout << "| " << setw(max[j]) << setiosflags(ios::left) << setfill(' ');
			if (result.relation.fields[j].type.tag == Type::Tag::INT)
			{
				cout << ' ' << result.records[i].values[j].INT << ' ';
			}
			else if (result.relation.fields[j].type.tag == Type::Tag::FLOAT)
			{
				cout << ' ' << result.records[i].values[j].FLOAT << ' ';
			}
			else if (result.relation.fields[j].type.tag == Type::Tag::CHAR)
			{
				cout << ' ' << result.records[i].values[j].CHAR << ' ';
			}
		}
		cout << '|' << endl;
	}
	draw_line(max, result.relation.fields.size());
	delete[] max;
}

void draw_line(int* max, int size)
{
	for (int i = 0; i < size; i++)
	{
		cout << "+-";
		for (int j = 0; j <= max[i]; j++)
		{
			cout << '-';
		}
	}
	cout << '+' << endl;
}
