#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "gia_djk.h"
#include "../gia_ipmnp/gia_ipmnp.h"

bool operator<(const dstNetv4_t& dst1, const dstNetv4_t& dst2) {
    return (dst1.net() & dst1.mask()) < (dst2.net() & dst2.mask());
};

bool operator==(const dstNetv4_t& dst1, const dstNetv4_t& dst2) {
    return (dst1.net == dst2.net) && (dst1.mask == dst2.mask);
};

extern const array <string,4> djkStStr {"idle", "calculations", "ready", "error in graph data" };

// добавляет один однонаправленный линк (смежность, adjacency) в граф
void GraphIPv4::addLink(nodeIDv4_t nodeID, linkAdjv4_t link) {
    if (state == graphStates::Idle) {
        state = graphStates::Populating;
        graph[nodeID].push_back(link);
        state = graphStates::Idle;
    }
};

// добавляет все однонаправленные линки из списка смежностей
void GraphIPv4::addLinks(vector <linkAdjStr_t> *table) {
    if (state == graphStates::Idle) {
        state = graphStates::Populating;
        for (auto && row : *table) {
            graph[v4mnp::to_u32i(row.NodeID)].push_back({v4mnp::to_u32i(row.localIP), v4mnp::to_u32i(row.mask), row.cost, v4mnp::to_u32i(row.neighIP), v4mnp::to_u32i(row.neighID)});
        }
        state = graphStates::Idle;
    }
}

// пополняет граф смежностей информацией из двоичного файла
void GraphIPv4::addLinksFromFile(const string &fn) {
     struct rec_t {
         u32i nodeID;
         struct {
             u32i localIP;
             u32i mask;
             u32i cost;
             u32i neighIP;
             u32i neighID;
         } la;
     } *readItem;
    char buff[sizeof(*readItem)];
    readItem = (rec_t*) buff;
    if (state == graphStates::Idle) {
        state = graphStates::Populating;
        ifstream ifile(fn, ios_base::in | ios_base::binary);
        if (ifile) {
            cout << "\nFile opened : ok." << endl;
            ifile.read(buff, sizeof(buff));
            while(!ifile.fail()) {
                graph[IPv4_Addr{readItem->nodeID}].push_back({
                                                              IPv4_Addr{readItem->la.localIP},
                                                              IPv4_Addr{readItem->la.mask},
                                                              readItem->la.cost,
                                                              IPv4_Addr{readItem->la.neighIP},
                                                              IPv4_Addr{readItem->la.neighID},
                                                             });
                ifile.read(buff, sizeof(buff));
            }
        } else {
            cout << "Error opening file." << endl;
        }
        state = graphStates::Idle;
    }
}

// отображает весь граф в консоль
void GraphIPv4::printGraph() {
    size_t linksCount {0};
    cout << "\nGraph database" << endl;
    for (auto && [oneNode, allLinks] : graph) {
        cout << "\n[ NodeId: " << oneNode.to_str() <<" ]" << endl;
        for (auto && oneLink : allLinks) {
            linksCount++;
            cout << "  > iface: " << right << setw(15) << oneLink.localIP.to_str() << "/" << v4mnp::mask_len(oneLink.mask()) << "    iface_cost: " << left << setw(5) << oneLink.cost <<
                "    remote_ip: " << setw(15) << oneLink.neighIP.to_str() << "    neigh.: [" << oneLink.neighID.to_str() << "]" << right << endl;
        }
    }
    cout << "---\nTotal nodes: " << graph.size() << ". Total adjacent links: " << linksCount << endl;
}


DjkIPv4::DjkIPv4(nodeIDv4_t srcID, GraphIPv4 *db, bool debug): rootID(srcID), graph(db) {
    debugMode = debug;
    RunCalc();
};


void DjkIPv4::RunCalc() {
    state = djkStates::Calculations;
    calcTree();
    if (state != djkStates::ErrInGraph) {
        fillNodesRI();
        fillRIB();
        state = djkStates::Ready;
    }
}


// если нужно подставить иную БД или rootID, но не хочется плодить другой объект дейкстры, то
// дожидаемся текущих рассчётов и вызываем reinit
void DjkIPv4::reinit(nodeIDv4_t srcID, GraphIPv4 *db, bool debug) {
    debugMode = debug;
    if (state != djkStates::Calculations) {
        rootID = srcID;
        graph = db;
        RunCalc();
    }
}


// инициализирует дерево начальными значениями
void DjkIPv4::initTree() {
    tree.clear();
    unseen.clear();
    for (auto node = graph->getIterBegin(); node != graph->getIterEnd(); node++ ) {
        tree[node->first].cumulCost = (node->first == rootID) ? 0 : MAX_COST; // для всех узлов, кроме исходного, максимальная стоимость
        unseen.insert(node->first);
    }
}

// отображает текущее состояние дерева в консоль
void DjkIPv4::printTree() {
    cout << endl;
    cout << "---------------------------------------------------------------------------------------------------------------" << endl;
    cout << "Current Tree:" << endl;
    cout << "---------------------|-------------------|----------------------|----------------------|----------------------|" << endl;
    cout << "          Child node |  Node cumul. cost |          Best parent |         Parent iface |          Child iface |" << endl;
    cout << "---------------------|-------------------|----------------------|----------------------|----------------------|" << endl;
    for (auto && [node, leaf] : tree) { // каждый узел дерева
        for (auto && [parent, ifPairs] : leaf.eqcoParents) { // все родители этого узла (т.к. может быть >1) и смежные пары интерфейсов
            for (auto && ifPair : ifPairs) {
                cout << setw(20) << node.to_str() << " : " << setw(17) << ((leaf.cumulCost == MAX_COST) ? "INFINITY" : to_string(leaf.cumulCost)) << " : "
                     << setw(20) << parent.to_str() << " : " << setw(20) << ifPair.parentIP.to_str() << " : " << setw(20) << ifPair.childIP.to_str() << " :" << endl;
            }
        }
    }
    cout << "---------------------------------------------------------------------------------------------------------------" << endl;
    cout << "Unseen nodes:" << endl;
    cout << "[ ";
    for (auto && nodeID : unseen) {
        cout << nodeID.to_str() << ", ";
    }
    cout << "]\n" << endl;
}


// выбирает из unseen узел с минимальным кумулятивным костом
nodeIDv4_t DjkIPv4::getMinCumCostNodeID() {
    u32i minCost {MAX_COST};
    nodeIDv4_t retValue {v4mnp::UNKNOWN_ADDR};
    for (auto && nodeID : unseen) {
        if (tree[nodeID].cumulCost <= minCost) {
            minCost = tree[nodeID].cumulCost;
            retValue = nodeID;
        }
    }
    return retValue;
}


// построение дерева из графа
void DjkIPv4::calcTree() {
    initTree();
    if (debugMode) printTree();
    if (tree.count(rootID) == 1) {
        ////////////////////////////////
        while (!unseen.empty()) {
            nodeIDv4_t currNodeID = getMinCumCostNodeID();
            if (tree[currNodeID].cumulCost != MAX_COST) { // проверка на MAX_COST, т.к. будет переполнение и проход через 0 в случае вычисления evalCost
                for (auto && link : *(graph->getLinks(currNodeID))) { // перебираем все линки родительского узла
                    if (debugMode) cout << "processing: " << left << setw(15) << currNodeID.to_str() << "; iface: " << setw(18) << left
                             << ((link.localIP.to_str() + "/" + to_string(v4mnp::mask_len(link.mask())))) << "; if_cost: " << setw(6) << link.cost << "; neigh.: " << setw(15);
                    if ((link.mask == v4mnp::LOOPBACK_MASK) or (link.neighIP == v4mnp::UNKNOWN_ADDR)) { // лупбеки и тупиковые сети в рассчётах не используются
                        if (debugMode) cout << " <loopback or stub is out of calculation>" << endl;
                        continue;
                    }
                    auto evalCost = tree[currNodeID].cumulCost + link.cost; // оценочная стоимость для каждого дочернего, сидящего за этим линком
                    if (debugMode) cout << link.neighID.to_str();
                    if ((tree[link.neighID].cumulCost >= evalCost)) {
                        if (debugMode) cout << " - eval. cost (" << evalCost << ") is better or equal !" << right << endl;;
                        if (tree[link.neighID].cumulCost > evalCost) {
                            tree[link.neighID].cumulCost = evalCost;
                            tree[link.neighID].eqcoParents.clear();
                        }
                        tree[link.neighID].eqcoParents[currNodeID].push_back({link.localIP, link.neighIP}); // child ip будут заполнены ниже вне цикла while
                    } else {
                        if (debugMode) cout << " - eval. cost (" << evalCost << ") is worse !" << right << endl;
                    }
                }
            } else { // если кумулятивный кост = MAX_COST -> нет полноценной двусторонней смежности -> удаляем узел из графа
                if (debugMode) cout << "\nRemoving " << currNodeID.to_str() << " due to cumulative cost == MAX_COST !" << endl;
                tree.erase(currNodeID);
            }
            unseen.erase(currNodeID); // обработали текущий узел -> убираем его из списка непосещённых
            if (debugMode) printTree();
        }
        ////////////////////////////////
    } else {
        state = djkStates::ErrInGraph;
    }
}


// пересчитывает кумулятивный кост маршрутной записи до сети == кум. кост до оригинатора + кост линка на оригинаторе
inline void DjkIPv4::recalcNetCost(roiVec &roivec, u32i linkCost) {
    for (auto && ri : roivec) {
        ri.cumulCost += linkCost;
    }
}


// пополняет RIB
void DjkIPv4::fillRIB() {
    auto graphDB = graph->getPtrNodes();
    for (auto && [origID, leaves] : tree) { // перебираем узлы из дерева
        for (auto && link : (*graphDB)[origID]) { // перебираем все линки узла
            auto netPfx = link.localIP() & link.mask(); // вычисляем сеть для каждого линка
            dstNetv4_t ribIndex {netPfx, link.mask}; // вычисляем индекс в RIB'е на основе сети и маски
            if (RIB.count(ribIndex) == 0) { // новая маршрутная запись
                RIB[ribIndex][origID] = nodesREI[origID];
                recalcNetCost(RIB[ribIndex][origID], link.cost);
            } else { // уже есть такая марш. запись -> сравним кумулятивный кост
                // перебираем всех originator'ов найденной маршрутной записи
                for (auto && [currOrgID, riVec] : RIB[ribIndex]) {
                    if ((tree[origID].cumulCost + link.cost) < riVec[0].cumulCost) { // новый кост лучше -> удаляем всех текущих originator'ов и добавляем новый
                        RIB[ribIndex].clear();
                        RIB[ribIndex][origID] = nodesREI[origID];
                        recalcNetCost(RIB[ribIndex][origID], link.cost);
                        break;
                    } else if ((tree[origID].cumulCost + link.cost) == riVec[0].cumulCost) { // добавляем ещё одного ECMP-originator'а
                        RIB[ribIndex][origID] = nodesREI[origID];
                        recalcNetCost(RIB[ribIndex][origID], link.cost);
                    }
                }
            }
        }
    }
}


void DjkIPv4::printREI() {
    cout << "----------------------------------------------------------------" << endl;
    cout << "\nTree nodes reachability:\n\n";
    for (auto && [nodeID, reivec] : nodesREI) {
        cout << "Node: [" << nodeID.to_str() << "]; cost [" << reivec[0].cumulCost << "]" << endl;
        for (auto && ri : reivec) {
            cout << "  > via iface: " << ri.localIP.to_str() << "; next-hop: " << ri.nexthopIP.to_str() << endl;
        }
    cout << endl;
    }
}


void DjkIPv4::printRIB() {
    cout << "----------------------------------------------------------------" << endl;
    cout << "\nRouter Information Base:\n\n";
    for (auto && [net, onri] : RIB) {
        cout << "Net: " <<  net.net.to_str() << "/" << v4mnp::mask_len(net.mask()) << endl;
        for (auto && [origNode, riVec] : onri) {
            for (auto && ri : riVec) {
                cout << "  > via iface: " << ri.localIP.to_str() << "; next-hop: " <<  ri.nexthopIP.to_str()
                     << "; cost: " << ri.cumulCost << "; origin.: " << origNode.to_str() << endl;
            }
        }
        cout << endl;
    }
}


// пополняет таблицу nodeRI маршрутами до всех узлов дерева
void DjkIPv4::fillNodesRI() {
    nodesREI.clear();
    for (auto && [nodeID, leaves] : tree) {
        auto routes = getRoutesToNode(nodeID);
        vector<routeInfo_t> roivec;
        for (auto && [nh, rt] : routes) { // перебираем все next-hop'ы, создавая из них set
            roivec.push_back({rt.localIP, rt.cumulCost, nh});
        }
        nodesREI[nodeID] = roivec;
    }
}


// рекурсия пути : выходим из dst и идём, пока не достигнем node.parent == UNKNOWN_NODE_ID
void DjkIPv4::getPath(const reiVec &reivec, vector<reiVec> &paths) {
    auto childID {reivec.back().parentID};
    for (auto && [pID, pairs] : tree[childID].eqcoParents) { // pID - parent ID
        if (pID() == v4mnp::UNKNOWN_ADDR) { // достигли реверсивного конца (т.е. SRC)
            paths.push_back(reivec); // записать получившийся вектор-путь в paths
            break;
        } else {
            for (auto && onePair : pairs) { // перебор интерфейсов очередного родителя
                reiVec newreivec = reivec; // место развилки, поэтому делаем новый вектор
                newreivec.push_back({childID, pID, onePair.parentIP, onePair.childIP}); // цепляем к концу route_info-вектора очередную маршрутную инфо
                getPath(newreivec, paths); // рекурсия, ныряем в очередной pID
            }
        }
    }
};


// с помощью обратной трассировки от dst к src рассчитывает все возможные пути
void DjkIPv4::calcPathsToNode(nodeIDv4_t dstNodeID, vector<reiVec> &paths) {
    getPath({{IPv4_Addr{v4mnp::UNKNOWN_ADDR}, dstNodeID, IPv4_Addr{v4mnp::UNKNOWN_ADDR}, IPv4_Addr{v4mnp::UNKNOWN_ADDR}}}, paths); // init reachInfo : childID = UNKNOWN_NODE_ID, parentID = dstNodeID
    if (debugMode) {
        cout << "----------------------------------------------------------------" << endl;
        cout << "\nAmount of paths to \"" << dstNodeID.to_str() << "\", using reverse trace : " << paths.size() << endl;
    }
    if (debugMode) {
        for (auto && onePath : paths) {
            cout << "\nPath #: " << endl;
            for (auto itPath = onePath.rbegin(); itPath != onePath.rend(); itPath++) {
                cout << " node: " << itPath->parentID.to_str() << "; via iface: " << itPath->parentIP.to_str() << "; nexthop: " << itPath->nexthopIP.to_str()
                         << "; nexthop node ID: " << itPath->childID.to_str() << endl;
            }
        }
        cout << endl;
    }
}


// возвращает все возможные маршруты до dst
map<nextHopV4_t, routeThru_t> DjkIPv4::getRoutesToNode(nodeIDv4_t dstNodeID) {
    map <IPv4_Addr,routeThru_t> retValue;
    vector <reiVec> allPathsToDst;
    calcPathsToNode(dstNodeID, allPathsToDst);
    if (tree.count(dstNodeID) != 0) {
        for (auto && onePath : allPathsToDst) {
            if (retValue.count(onePath.back().nexthopIP) >= 0) { // помещаем только уникальные next-hop'ы
                retValue[onePath.back().nexthopIP] = {onePath.back().parentIP, tree[dstNodeID].cumulCost};
            }
        }
    }
    return retValue;
}
