#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include <iomanip>
#include "../gia_ipmnp/gia_ipmnp.h"

using namespace std;

using addrv4_t = IPv4_Addr;
using nhv4_t = IPv4_Addr;
using nodeIDv4_t = IPv4_Addr;
using origIDv4_t = IPv4_Addr;

struct anortr_t { // another router
    nodeIDv4_t neighID;
    addrv4_t localIP;
    u32i cost;
};

struct stubnet_t { // stub network
    addrv4_t net;
    addrv4_t mask;
    u32i cost;
};

struct lpbk_t { // loopback (variant of stub ip with /32 mask)
    addrv4_t ip;
    u32i cost;
};

using lpbks = vector <lpbk_t>;
using stubs = vector <stubnet_t>;
using neighs = vector <anortr_t>;

struct lsdbrec_t {
    neighs neighs;
    stubs stubs;
    lpbks lpbks;
};

map <origIDv4_t, lsdbrec_t> LSDB;

void parseASRLog(string &fn) {
    ifstream ifile(fn, ios_base::in);
    if (ifile) {
        // char* buff = (char*) malloc(256);
        char buff[256];
        origIDv4_t lsID {u32i(0)};
        cout << "// Opened ok." << endl;
        ifile.getline(buff, sizeof(buff), '\n');
        while(!ifile.fail()) {
            string ts = buff; // temporary string
            if (ts.substr(0,17) == "  Link State ID: ") {
                lsID = v4mnp::to_u32i(ts.substr(17,ts.size()-1)); // set current ls id
            } else
                if (ts.substr(0,38) == "     (Link ID) Neighboring Router ID: ") {
                    nodeIDv4_t neighID = v4mnp::to_u32i(ts.substr(38,ts.size()-1));
                    ifile.getline(buff, sizeof(buff), '\n');
                    if (!ifile.fail()) {
                        ts = buff;
                        if (ts.substr(0,43) == "     (Link Data) Router Interface address: ") {
                            addrv4_t localIP = v4mnp::to_u32i(ts.substr(43,ts.size()-1));
                            ifile.getline(buff, sizeof(buff), '\n');
                            ifile.getline(buff, sizeof(buff), '\n');
                            if (!ifile.fail()) {
                                ts = buff;
                                if (ts.substr(0,22) == "       TOS 0 Metrics: ") {
                                    u32i cost = stoi(ts.substr(22,ts.size()-1));
                                    LSDB[lsID].neighs.push_back({neighID, localIP, cost});
                                }
                            }
                        }
                    }
                } else
                    if (ts.substr(0,38) == "     (Link ID) Network/subnet number: ") {
                        addrv4_t net = v4mnp::to_u32i(ts.substr(38,ts.size()-1));
                        ifile.getline(buff, sizeof(buff), '\n');
                        if (!ifile.fail()) {
                            ts = buff;
                            if (ts.substr(0,31) == "     (Link Data) Network Mask: ") {
                                addrv4_t mask = v4mnp::to_u32i(ts.substr(31,ts.size()-1));
                                ifile.getline(buff, sizeof(buff), '\n');
                                ifile.getline(buff, sizeof(buff), '\n');
                                if (!ifile.fail()) {
                                    ts = buff;
                                    if (ts.substr(0,22) == "       TOS 0 Metrics: ") {
                                        u32i cost = stoi(ts.substr(22,ts.size()-1));
                                        (mask == 0xFFFFFFFF) ? LSDB[lsID].lpbks.push_back({net, cost}) : LSDB[lsID].stubs.push_back({net, mask, cost});
                                    }
                                }
                            }
                        }
                    }
            ifile.getline(buff, sizeof(buff), '\n');
        };
        // free(buff);
        ifile.close();
    }
}

void printLSDB() {
    for (auto && [lsID, rec] : LSDB) {
        cout << "\nLink State ID: " << lsID.to_str() << endl;
        // перебор соседей
        for (auto && anortr : rec.neighs) {
            cout << "  > neigh.: " << setw(15) << left << anortr.neighID.to_str() << " via: " << anortr.localIP.to_str() << " cost: " << "" << right << endl;
        }
        // перебор стаб-сетей
        for (auto && net : rec.stubs) {
            cout << "  > stub net: " << setw(18) << left << net.net.to_str() + "/" + to_string(v4mnp::mask_len(net.mask())) << " if_cost: " << net.cost << right << endl;
        }
        // перебор лупбеков
        for (auto && lpbk : rec.lpbks) {
            cout << "  > lpbk: " << setw(17) << left << lpbk.ip.to_str() << " if_cost: " << lpbk.cost << right << endl;
        }
    }
}

struct linkAdjv4_t {
    addrv4_t localIP;
    addrv4_t mask;
    u32i cost; // local link cost
    addrv4_t neighIP;
    nodeIDv4_t neighID;
};

using linkAdjv4Vec = vector <linkAdjv4_t>;

map <nodeIDv4_t, linkAdjv4Vec> binAdjcy;


// возвращает маску для локального ip, если есть соотв. stub-сеть, иначе возвр. "0.0.0.0"
addrv4_t getMask(origIDv4_t origID, addrv4_t ip) {
    addrv4_t mostSpc {u32i(0x00000000)};
    for (auto && stub : LSDB[origID].stubs) {
        if (((ip & stub.mask) == stub.net) && (stub.mask > mostSpc)) {
            mostSpc = stub.mask;
        }
    }
    return mostSpc;
}


addrv4_t getNH(origIDv4_t origID, nodeIDv4_t targetID, addrv4_t ip, addrv4_t mask) {
    for (auto && neigh : LSDB[origID].neighs) {
        if ((neigh.neighID == targetID) && ((neigh.localIP & mask) == (ip & mask))) {
            return neigh.localIP;
        }
    }
    return IPv4_Addr{u32i(0)};
}


void createAdjTable() {
    for (auto && [lsID, rec] : LSDB) {
        // перебор лупбеков, это самое простое, т.к. не надо вычислять противоположный ip и id соседа
        for (auto && lpbk : rec.lpbks) {
            binAdjcy[lsID].push_back({lpbk.ip, 0xFFFFFFFF, lpbk.cost, lpbk.ip, lpbk.ip});
        }
        // перебор соседей
        for (auto && neigh : rec.neighs) {
            addrv4_t mask = getMask(lsID, neigh.localIP);
            if (mask != 0x00000000) {
                addrv4_t nhIP = getNH(neigh.neighID, lsID, neigh.localIP, mask);
                if (nhIP != 0x00000000) {
                    binAdjcy[lsID].push_back({neigh.localIP, mask, neigh.cost, nhIP, neigh.neighID});
                }
            }
        }
    }
}


struct writeItem_t {
    nodeIDv4_t nodeID;
    addrv4_t localIP;
    addrv4_t mask;
    u32i cost; // local link cost
    addrv4_t neighIP;
    nodeIDv4_t neighID;
};

void dumpAdjToFile(string &fn) {
    ofstream ofile(fn, ios_base::out | ios_base::binary | ios_base::trunc);
    if (ofile) {
        cout << "\n// File creation ok.\n// Writing...\n" << endl;
        for (auto && [nodeID, laVec] : binAdjcy) {
            for (auto && la : laVec) {
                writeItem_t wi {nodeID, la.localIP, la.mask, la.cost, la.neighIP, la.neighID};
                ofile.write((char*)&wi, sizeof(writeItem_t));
            }
        }
    }
}

void printAdjTable() {
    cout << "\n// Adjacency table: \n" << endl;
    for (auto && [nodeID, laVec] : binAdjcy) {
        for (auto && la : laVec) {
            cout << "{\"" << nodeID.to_str() << "\", \"" << la.localIP.to_str() << "\", \"" << la.mask.to_str()
                 << "\", " << la.cost << ", \"" << la.neighIP.to_str() << "\", \"" << la.neighID.to_str() << "\"}," << endl;
        }
    }
}


int main() {
    string srcFN, dstFN;
    cout << "[ XR OSPF database converter ]\n" << endl;
    cout << "Login to your Cisco XR router and issue comamnd in cli :\n\n 'show ospf database router'\n" << endl;
    cout << "Then collect output to file. Supports only LSA Type 0.\n" << endl;
    cout << "Now specify this filename  : ";
    cin >> srcFN;
    cout << "Specify ouput binary file : ";
    cin >> dstFN;
    parseASRLog(srcFN);
    cout << "// Nodes amount: " << LSDB.size() << endl;
    printLSDB();
    createAdjTable();
    printAdjTable();
    dumpAdjToFile(dstFN);
    cout << "// Stop." << endl;
    cout << "// Now you can use generated file in 'gia_djk' project as input data!" << endl;
    return 0;
}
