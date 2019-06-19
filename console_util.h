#pragma once
#include "query_executor.h"

void execute_expr(QueryExecutor& executor, string expr);
void disp_records(QueryResult& result);
void draw_line(int* max, int size);
void execute_file(QueryExecutor& executor, string filename);