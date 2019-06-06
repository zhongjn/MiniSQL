# CMakeList.txt : CMake project for MiniSQL, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

if (MSVC)
	add_compile_options(/GR)
endif()

set(project_headers
	"block_manager.h"
	"catalog_manager.h"
	"common.h"
	"expression.h"
	"files.h"
	"index_manager.h"
	"lru.h"
	"nullable.h"
	"query_executor.h"
	"query_lexer.h"
	"query_parser.h"
	"record_manager.h"
	"scanner.h"
)

set(project_sources 
    "block_manager.cpp"
	"catalog_manager.cpp"
	"common.cpp"
	"expression.cpp"
	"index_manager.cpp"
	"query_executor.cpp"
	"query_lexer.cpp"
	"query_parser.cpp"
	"record_manager.cpp"
	"scanner.cpp"
)

set(tests
	"test/01_catalog.cpp"
	"test/02_record.cpp"
	"test/03_scanner.cpp"
	"test/04_parser.cpp"
	"test/05_executor.cpp"
)

add_executable (MiniSQL "main.cpp" ${project_headers} ${project_sources})

# Tests
add_executable (TestEntry 	"test/test.h" "test/test.cpp" ${project_headers} ${project_sources} ${tests})