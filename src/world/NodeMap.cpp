#include "NodeMap.hpp"

NodeMap::NodeMap() {
    buildWorld0();
}

// ---------------------------------------------------------------------------
// World 0 — SECTOR ALPHA
// Classic entry-level dungeon with 10 nodes, Manticore boss.
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
// ---------------------------------------------------------------------------
void NodeMap::buildWorld0() {
    // fields: id, name, objective, tier, col, row, sweepTarget, surviveSeconds, status, connections
    m_nodes = {
        { 0, "SYS-ENTRY",   NodeObjective::Sweep,   1, 1, 0,  6,  0.f, NodeStatus::Available, {2}        },
        { 1, "ARCHIVE-A",   NodeObjective::Survive,  1, 0, 1,  0, 30.f, NodeStatus::Locked,   {2,4}      },
        { 2, "DATASTORE",   NodeObjective::Sweep,    1, 1, 1,  8,  0.f, NodeStatus::Locked,   {0,1,3,5}  },
        { 3, "VAULT-7",     NodeObjective::Survive,  1, 2, 1,  0, 35.f, NodeStatus::Locked,   {2,6}      },
        { 4, "SECURITY",    NodeObjective::Sweep,    2, 0, 2, 12,  0.f, NodeStatus::Locked,   {1,7}      },
        { 5, "HUB-BETA",    NodeObjective::Sweep,    2, 1, 2, 10,  0.f, NodeStatus::Locked,   {2,7,8}    },
        { 6, "ICE-NEST",    NodeObjective::Sweep,    2, 2, 2, 15,  0.f, NodeStatus::Locked,   {3,8}      },
        { 7, "FIREWALL",    NodeObjective::Survive,  3, 0, 3,  0, 50.f, NodeStatus::Locked,   {4,5,8}    },
        { 8, "CORE-ACCESS", NodeObjective::Sweep,    3, 1, 3, 20,  0.f, NodeStatus::Locked,   {5,6,7,9}  },
        { 9, "MAINFRAME",   NodeObjective::Boss,     4, 1, 4, 30,  0.f, NodeStatus::Locked,   {8}        },
    };
}

// ---------------------------------------------------------------------------
// World 1 — DEEP NET
// Smaller but harder dungeon, 8 nodes, Archon boss.
// Tier values are all +1 vs World 0 equivalents; diffMult 1.4× applied in MapScene.
//
//          (1,0)=SUBNET
//            |
//  (0,1)=RELAY-(1,1)=NEXUS-(2,1)=GHOST
//     |          |            |
//  (0,2)=TRAP  (1,2)=CLUSTER (2,2)=CIPHER
//                |
//            (1,3)=ARCHON-NODE
// ---------------------------------------------------------------------------
void NodeMap::buildWorld1() {
    m_nodes = {
        { 0, "SUBNET",       NodeObjective::Sweep,   2, 1, 0,  8,  0.f, NodeStatus::Available, {2}        },
        { 1, "RELAY-A",      NodeObjective::Survive,  2, 0, 1,  0, 25.f, NodeStatus::Locked,   {2,4}      },
        { 2, "NEXUS",        NodeObjective::Sweep,    2, 1, 1, 12,  0.f, NodeStatus::Locked,   {0,1,3,5}  },
        { 3, "GHOST-NET",    NodeObjective::Sweep,    2, 2, 1, 10,  0.f, NodeStatus::Locked,   {2,6}      },
        { 4, "TRAP-NODE",    NodeObjective::Survive,  3, 0, 2,  0, 35.f, NodeStatus::Locked,   {1,7}      },
        { 5, "CLUSTER",      NodeObjective::Sweep,    3, 1, 2, 18,  0.f, NodeStatus::Locked,   {2,7}      },
        { 6, "CIPHER",       NodeObjective::Sweep,    3, 2, 2, 16,  0.f, NodeStatus::Locked,   {3}        },
        { 7, "ARCHON-NODE",  NodeObjective::Boss,     4, 1, 3, 25,  0.f, NodeStatus::Locked,   {4,5}      },
    };
}

// ---------------------------------------------------------------------------
// World 2 — THE CORE
// Short but brutal, 6 nodes, Vortex boss.
// All nodes are tier 3–4; diffMult 1.8× applied in MapScene.
//
//  (0,0)=BULWARK-(1,0)=TERMINUS-(2,0)=BREACH
//       |              |
//  (0,1)=MATRIX  (1,1)=SENTINEL
//                      |
//                  (1,2)=VORTEX-CORE
// ---------------------------------------------------------------------------
void NodeMap::buildWorld2() {
    m_nodes = {
        { 0, "BULWARK",      NodeObjective::Sweep,   3, 0, 0, 18,  0.f, NodeStatus::Available, {1,3}      },
        { 1, "TERMINUS",     NodeObjective::Survive,  3, 1, 0,  0, 30.f, NodeStatus::Locked,   {0,2,4}    },
        { 2, "BREACH",       NodeObjective::Sweep,    3, 2, 0, 20,  0.f, NodeStatus::Locked,   {1}        },
        { 3, "MATRIX",       NodeObjective::Survive,  4, 0, 1,  0, 40.f, NodeStatus::Locked,   {0,4}      },
        { 4, "SENTINEL",     NodeObjective::Sweep,    4, 1, 1, 25,  0.f, NodeStatus::Locked,   {1,3,5}    },
        { 5, "VORTEX-CORE",  NodeObjective::Boss,     4, 1, 2, 30,  0.f, NodeStatus::Locked,   {4}        },
    };
}

// ---------------------------------------------------------------------------

void NodeMap::reset(int worldId) {
    m_nodes.clear();
    switch (worldId) {
        case 1:  buildWorld1(); break;
        case 2:  buildWorld2(); break;
        default: buildWorld0(); break;
    }
}

void NodeMap::revealAll() {
    for (auto& n : m_nodes)
        n.revealed = true;
}

void NodeMap::clearNode(int id) {
    if (id < 0 || id >= (int)m_nodes.size()) return;
    m_nodes[id].status = NodeStatus::Cleared;
    for (int nb : m_nodes[id].connections) {
        if (m_nodes[nb].status == NodeStatus::Locked)
            m_nodes[nb].status = NodeStatus::Available;
    }
}
