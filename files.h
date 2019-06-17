#pragma once
#include <string>
#include "common.h"
using namespace std;

class Files {
public:
    static string scheme() {
        return "data/scheme.dat";
    }
	static string relation(const string& name) {
		return "data/" + name + ".dat";
	}
	static string index(const string& name, int field_index) {
		return "data/index_" + name + to_string(field_index) + ".dat";
	}
		
};