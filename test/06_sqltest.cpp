#include "test.h"
#include "../record_manager.h"
#include "../catalog_manager.h"
#include "../query_executor.h"
#include "../console_util.h"

TEST_CASE(table_select_0_sql) {
	StorageEngine eng;
	QueryExecutor executor(&eng);
	vector<string> exprs = get_exprs_in_file(executor, "sql/table-select-0.sql");
	for (string& str : exprs)
	{
		cout << "Executing command: " << str << endl;
		QueryResult result = executor.execute(QueryParser().parse(QueryLexer().tokenize(str)));
		if (result.relation.fields.size() != 0)
		{
			// Show select results
			disp_records(result);
		}
		cout << result.prompt << endl << endl;
		assert(!result.failed, "Query failed");
	}
}