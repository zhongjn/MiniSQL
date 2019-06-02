#include "block_manager.h"
#include <iostream>

using namespace std;

int main() {
    BlockManager* bm = new BlockManager(10);

    {
        BlockGuard g1(bm, "data/d1.dat", 0);
        memset(g1.addr, 99, 1000);
        g1.set_modified();
    }

    {
        BlockGuard g1(bm, "data/d1.dat", 1);
        memset(g1.addr, 8, 1000);
        g1.set_modified();
    }

    bm->flush();
    return 0;
}
