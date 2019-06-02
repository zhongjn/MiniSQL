#pragma once
#include <string>
using namespace std;

class Files {
public:
    static string scheme() {
        return "data/scheme.dat";
    }
    static string relation(const string& name) {
        return "data/" + name + ".dat";
    }
};