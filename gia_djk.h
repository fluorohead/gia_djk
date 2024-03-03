#ifndef GIA_DJK_H
#define GIA_DJK_H

#include <vector>
#include <map>
#include <string>
#include <set>
#include "../gia_ipmnp/gia_ipmnp.h"

using namespace std;

using nextHopV4_t = IPv4_Addr;
using nodeIDv4_t  = IPv4_Addr;
using origIDv4_t  = IPv4_Addr;

constexpr u32i MAX_COST = 65535;

struct dstNetv4_t {
    IPv4_Addr net;
    IPv4_Addr mask;
    friend bool operator<(const dstNetv4_t&, const dstNetv4_t&);
    friend bool operator==(const dstNetv4_t&, const dstNetv4_t&);
};

struct linkAdjStr_t {
    string NodeID;
    string localIP;
    string mask;
    u32i cost; // local link cost
    string neighIP;
    string neighID;
};

struct linkAdjv4_t {
    IPv4_Addr localIP;
    IPv4_Addr mask;
    u32i cost; // local link cost
    IPv4_Addr neighIP;
    nodeIDv4_t neighID;
};

struct reachInfo_t {
    nodeIDv4_t childID;
    nodeIDv4_t parentID;
    IPv4_Addr parentIP; // на родителе (egress-port id)
    IPv4_Addr nexthopIP; // на дочернем ingress-port
};

struct adjPairv4_t {
    IPv4_Addr parentIP;
    IPv4_Addr childIP;
};

struct leafv4_t {
    u32i cumulCost {MAX_COST};
    map <nodeIDv4_t, vector<adjPairv4_t>> eqcoParents {{v4mnp::UNKNOWN_ADDR, {{v4mnp::UNKNOWN_ADDR, v4mnp::UNKNOWN_ADDR}}}}; // хэш с [ID родителей]."списком смежных интерфейсов"
};

struct routeInfo_t {
    IPv4_Addr localIP; // local egress-port id
    u32i cumulCost; // cumulative cost to dst
    IPv4_Addr nexthopIP; // next-hop via egress-port
};

struct routeThru_t {
    IPv4_Addr localIP; // local egress-port id
    u32i cumulCost; // cumulative cost to dst
};

using laVec  = vector <linkAdjv4_t>;
using reiVec = vector <reachInfo_t>;
using roiVec = vector <routeInfo_t>;

enum class graphStates {Idle, Populating};
enum class djkStates {Idle, Calculations, Ready, ErrInGraph};

////// OSPF Data Base for IPv4
class GraphIPv4 {
    map <nodeIDv4_t,laVec> graph; // хэш-таблица узлов, построенная из данных о стаб-сетях и соседях
    map <nodeIDv4_t,string> hostnames; // хэш-таблица символьных имён
    graphStates state {graphStates::Idle};

public:
    GraphIPv4() {};
    void addLink(nodeIDv4_t nodeID, linkAdjv4_t link); // add one adjacency link
    void addLinks(vector <linkAdjStr_t> *table); // load adjacency links from table
    void addLinksFromFile(const string &fn); // load adjacency table from binary file
    void addName(nodeIDv4_t nodeID, const string &hostname) { hostnames[nodeID] = hostname; }; // add hostname
    void printGraph();
    graphStates getState() { return state; };
    size_t getNodesCount() { return graph.size(); };
    string idToName(nodeIDv4_t nodeID) {return hostnames[nodeID]; }; // nodeid to hostname resolver
    map<nodeIDv4_t,laVec> *getPtrNodes() { return &graph; };
    laVec *getLinks(nodeIDv4_t nodeID) { return &graph[nodeID]; };
    map<nodeIDv4_t,laVec>::iterator getIterBegin() { return graph.begin(); };
    map<nodeIDv4_t,laVec>::iterator getIterEnd() { return graph.end(); };
};


////// Dijkstra Algorithm for IPv4
class DjkIPv4 {
    djkStates state {djkStates::Idle};
    GraphIPv4 *graph;
    nodeIDv4_t rootID;
    map <nodeIDv4_t,leafv4_t> tree;
    set <nodeIDv4_t> unseen;
    void RunCalc();
    void initTree();
    void calcTree();
    void fillNodesRI();
    void fillRIB();
    nodeIDv4_t getMinCumCostNodeID();
    void getPath(const reiVec &reivec, vector<reiVec> &paths);
    void calcPathsToNode(nodeIDv4_t dstNodeID, vector<reiVec> &paths);
    void recalcNetCost(roiVec &roivec, u32i linkCost);
    map<nextHopV4_t,routeThru_t> getRoutesToNode(nodeIDv4_t dstNodeID);
    map <nodeIDv4_t, roiVec> nodesREI; // маршрутная инфа достижимости каждого узла из корня дерева
    map <dstNetv4_t, map<origIDv4_t, roiVec>> RIB; // router information base (итоговая таблица маршрутов ко всем сетям и лупбекам)
    bool debugMode {false};

public:
    DjkIPv4(nodeIDv4_t srcID, GraphIPv4 *db, bool debug);
    void reinit(nodeIDv4_t srcID, GraphIPv4 *db, bool debug);
    void printTree();
    void printREI();
    void printRIB();
    djkStates getState() { return state; };
    void setDebug() { debugMode = true; };
    void unsetDebug() { debugMode = false; };
};

#endif // GIA_DJK_H
