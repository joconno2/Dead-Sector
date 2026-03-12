#include "NodeMap.hpp"

NodeMap::NodeMap() {
    buildDungeon();
}

void NodeMap::buildDungeon() {
    //  Dungeon grid (col, row) — Zelda-style room layout:
    //
    //             (1,0)=ENTRY
    //               |
    //  (0,1)=ARCH-(1,1)=DATA-(2,1)=VAULT
    //     |           |          |
    //  (0,2)=SEC  (1,2)=HUB  (2,2)=ICENEST
    //     |           |
    //  (0,3)=FIRE-(1,3)=CORE
    //               |
    //           (1,4)=MAINFRAME

    // fields: id, name, objective, tier, col, row, sweepTarget, surviveSeconds, status, connections
    m_nodes = {
        { 0, "SYS-ENTRY",   NodeObjective::Sweep,   1, 1, 0,  6,  0.f, NodeStatus::Available, {2}      },
        { 1, "ARCHIVE-A",   NodeObjective::Survive,  1, 0, 1,  0, 30.f, NodeStatus::Locked,   {4}      },
        { 2, "DATASTORE",   NodeObjective::Sweep,    1, 1, 1,  8,  0.f, NodeStatus::Locked,   {1,3,5}  },
        { 3, "VAULT-7",     NodeObjective::Survive,  1, 2, 1,  0, 35.f, NodeStatus::Locked,   {6}      },
        { 4, "SECURITY",    NodeObjective::Sweep,    2, 0, 2, 12,  0.f, NodeStatus::Locked,   {7}      },
        { 5, "HUB-BETA",    NodeObjective::Sweep,    2, 1, 2, 10,  0.f, NodeStatus::Locked,   {7,8}    },
        { 6, "ICE-NEST",    NodeObjective::Sweep,    2, 2, 2, 15,  0.f, NodeStatus::Locked,   {8}      },
        { 7, "FIREWALL",    NodeObjective::Survive,  3, 0, 3,  0, 50.f, NodeStatus::Locked,   {8}      },
        { 8, "CORE-ACCESS", NodeObjective::Sweep,    3, 1, 3, 20,  0.f, NodeStatus::Locked,   {9}      },
        { 9, "MAINFRAME",   NodeObjective::Boss,     4, 1, 4, 30,  0.f, NodeStatus::Locked,   {}       },
    };
}

void NodeMap::reset() {
    m_nodes.clear();
    buildDungeon();
}

void NodeMap::clearNode(int id) {
    if (id < 0 || id >= (int)m_nodes.size()) return;
    m_nodes[id].status = NodeStatus::Cleared;
    for (int nb : m_nodes[id].connections) {
        if (m_nodes[nb].status == NodeStatus::Locked)
            m_nodes[nb].status = NodeStatus::Available;
    }
}
