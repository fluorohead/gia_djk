#include <iostream>
#include <string>

#include "gia_djk.h"
#include "../gia_ipmnp/gia_ipmnp.h"

using namespace std;

extern vector <linkAdjStr_t> adjVec;
extern const array <string,6> djkStStr;

int main(int argc, char *argv[])
{

    if (argc == 4) {
        auto graph01 = new GraphIPv4;
        bool debugMode = (string(argv[3]) == "debug") ? true : false;

        // graph01->addLinks(&adjVec);
        graph01->addLinksFromFile(argv[1]);

        // graph01->addName(v4mnp::stou32("10.0.0.1"), "S__10.0.0.1");
        // graph01->addName(v4mnp::stou32("172.16.0.1"), "A__172.16.0.1");
        // graph01->addName(v4mnp::stou32("192.168.168.7"), "B__192.168.168.7");
        // graph01->addName(v4mnp::stou32("10.0.0.3"), "C__10.0.0.3");
        // graph01->addName(v4mnp::stou32("192.168.0.5"), "D__192.168.0.5");
        // graph01->addName(v4mnp::stou32("172.16.172.10"), "E__172.16.172.10");
        // graph01->addName(v4mnp::stou32("10.0.0.2"), "F__10.0.0.2");
        // graph01->addName(v4mnp::stou32("10.0.0.4"), "G__10.0.0.4");
        // graph01->addName(v4mnp::stou32("172.16.172.66"), "X__172.16.172.66");
        // graph01->addName(v4mnp::stou32("0.0.0.0"), "N/A");

        if (debugMode) graph01->printGraph();
        
        auto djk = new DjkIPv4(v4mnp::to_u32i(argv[2]), graph01, debugMode);
        if (debugMode) djk->printREI();
        djk->printRIB();
        cout << "Dijkstra FSM response : " << djkStStr[(u32i)djk->getState()] << endl;
        delete djk;
        delete graph01;
    } else {
        cout << "\nSyntax : djk_ipv4.exe <db_filename> <root_node> <debug_mode>\ndebug_mode = [debug|nodebug]" << endl;
    }

    return 0;
}
