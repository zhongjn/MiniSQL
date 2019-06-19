#pragma once
#include "query_executor.h"

bool execute_safe_print(QueryExecutor& executor, const string& expr);
// void disp_records(QueryResult& result);
void draw_line(int* max, int size);
void execute_file(QueryExecutor& executor, const string& filename);