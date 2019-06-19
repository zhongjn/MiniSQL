#include "test.h"
#include "../record_manager.h"
#include "../catalog_manager.h"
#include "../query_executor.h"
#include "../console_util.h"
#include <windows.h>

TEST_CASE(table_select_0_sql) {
	remove("data/catalog.dat");
	StorageEngine eng;
	QueryExecutor executor(&eng);
    cout << endl;
	assert(execute_file(executor, "sql/table-select-0.sql"), "");
}