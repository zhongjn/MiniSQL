#include "test.h"
vector<pair<string, void(*)()>> test_cases;

using namespace std;

int main() {
    int cur = 1;
    int len = test_cases.size();
    for (auto& p : test_cases) {
        auto& test = p.first;
        cout << "[" << cur << "/" << len << "] " << test << " : ";
        void (*fp)() = p.second;
        try {
            fp();
            cout << "Passed" << endl;
        }
        catch (AssertionError & ex) {
            cout << "Assertion failed! " << ex.what() << endl;
        }
        catch (exception & ex) {
            cout << "Unhandled error! " << ex.what() << endl;
        }
        cur++;
    }
    return 0;
}