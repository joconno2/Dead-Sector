#include "NodeMap.hpp"

NodeMap::NodeMap() {
    buildWorld0();
}

// ---------------------------------------------------------------------------
// World 0 — SECTOR ALPHA  (15 nodes, Manticore boss)
//
// (0,0)=GHOST[EVT]  (1,0)=ENTRY  (2,0)=ANOMALY[EVT]
//                      |
// (0,1)=ARCH-(1,1)=DATA-(2,1)=VAULT-(3,1)=CACHE-A[EXT]
//    |          |          |               |
// (0,2)=SEC  (1,2)=HUB  (2,2)=ICENEST  (3,2)=RELAY-B[EXT]
//    |          |          |
// (0,3)=FIRE-(1,3)=CORE  (2,3)=CRYPT
//               |
//           (1,4)=MAINFRAME[BOSS]
//
// fields: id, name, objective, tier, col, row,
//         sweepTarget, surviveSeconds, extractTarget, status, connections
// ---------------------------------------------------------------------------
void NodeMap::buildWorld0() {
    m_nodes = {
        { 0, "SYS-ENTRY",   NodeObjective::Sweep,    1, 1, 0,  6,  0.f,  0, NodeStatus::Available, {2,13,14}      },
        { 1, "ARCHIVE-A",   NodeObjective::Survive,  1, 0, 1,  0, 30.f,  0, NodeStatus::Locked,   {2,4,13}       },
        { 2, "DATASTORE",   NodeObjective::Sweep,    1, 1, 1,  8,  0.f,  0, NodeStatus::Locked,   {0,1,3,5}      },
        { 3, "VAULT-7",     NodeObjective::Survive,  1, 2, 1,  0, 35.f,  0, NodeStatus::Locked,   {2,6,10,14}    },
        { 4, "SECURITY",    NodeObjective::Sweep,    2, 0, 2, 12,  0.f,  0, NodeStatus::Locked,   {1,7}          },
        { 5, "HUB-BETA",    NodeObjective::Sweep,    2, 1, 2, 10,  0.f,  0, NodeStatus::Locked,   {2,7,8}        },
        { 6, "ICE-NEST",    NodeObjective::Sweep,    2, 2, 2, 15,  0.f,  0, NodeStatus::Locked,   {3,8,12}       },
        { 7, "FIREWALL",    NodeObjective::Survive,  3, 0, 3,  0, 50.f,  0, NodeStatus::Locked,   {4,5,8}        },
        { 8, "CORE-ACCESS", NodeObjective::Sweep,    3, 1, 3, 20,  0.f,  0, NodeStatus::Locked,   {5,6,7,9,12}   },
        { 9, "MAINFRAME",   NodeObjective::Boss,     4, 1, 4, 30,  0.f,  0, NodeStatus::Locked,   {8}            },
        {10, "CACHE-A",     NodeObjective::Extract,  1, 3, 1,  0,  0.f,  8, NodeStatus::Locked,   {3,11}         },
        {11, "RELAY-B",     NodeObjective::Extract,  2, 3, 2,  0,  0.f, 12, NodeStatus::Locked,   {10,6}         },
        {12, "CRYPT",       NodeObjective::Sweep,    2, 2, 3, 14,  0.f,  0, NodeStatus::Locked,   {6,8}          },
        {13, "GHOST-NODE",  NodeObjective::Event,    1, 0, 0,  0,  0.f,  0, NodeStatus::Locked,   {0,1}          },
        {14, "ANOMALY-X",   NodeObjective::Event,    1, 2, 0,  0,  0.f,  0, NodeStatus::Locked,   {0,3}          },
    };
}

// ---------------------------------------------------------------------------
// World 1 — DEEP NET  (13 nodes, Archon boss)
//
// (0,0)=DARK[EVT]  (1,0)=SUBNET  (2,0)=PHANTOM[EVT]
//                     |
// (0,1)=RELAY-(1,1)=NEXUS-(2,1)=GHOST-(3,1)=DATA-VAULT[EXT]
//    |          |            |               |
// (0,2)=TRAP (1,2)=CLUSTER (2,2)=CIPHER  (3,2)=ECHO-CACHE[EXT]
//    |          |
// (0,3)=DEADZONE (1,3)=ARCHON-NODE[BOSS]
// ---------------------------------------------------------------------------
void NodeMap::buildWorld1() {
    m_nodes = {
        { 0, "SUBNET",      NodeObjective::Sweep,    2, 1, 0,  8,  0.f,  0, NodeStatus::Available, {2,11,12}      },
        { 1, "RELAY-A",     NodeObjective::Survive,  2, 0, 1,  0, 25.f,  0, NodeStatus::Locked,   {2,4,11}       },
        { 2, "NEXUS",       NodeObjective::Sweep,    2, 1, 1, 12,  0.f,  0, NodeStatus::Locked,   {0,1,3,5}      },
        { 3, "GHOST-NET",   NodeObjective::Sweep,    2, 2, 1, 10,  0.f,  0, NodeStatus::Locked,   {2,6,8,12}     },
        { 4, "TRAP-NODE",   NodeObjective::Survive,  3, 0, 2,  0, 35.f,  0, NodeStatus::Locked,   {1,7,10}       },
        { 5, "CLUSTER",     NodeObjective::Sweep,    3, 1, 2, 18,  0.f,  0, NodeStatus::Locked,   {2,7}          },
        { 6, "CIPHER",      NodeObjective::Sweep,    3, 2, 2, 16,  0.f,  0, NodeStatus::Locked,   {3,9}          },
        { 7, "ARCHON-NODE", NodeObjective::Boss,     4, 1, 3, 25,  0.f,  0, NodeStatus::Locked,   {4,5}          },
        { 8, "DATA-VAULT",  NodeObjective::Extract,  2, 3, 1,  0,  0.f, 10, NodeStatus::Locked,   {3,9}          },
        { 9, "ECHO-CACHE",  NodeObjective::Extract,  3, 3, 2,  0,  0.f, 14, NodeStatus::Locked,   {8,6}          },
        {10, "DEADZONE",    NodeObjective::Sweep,    3, 0, 3, 20,  0.f,  0, NodeStatus::Locked,   {4}            },
        {11, "DARK-RELAY",  NodeObjective::Event,    2, 0, 0,  0,  0.f,  0, NodeStatus::Locked,   {0,1}          },
        {12, "PHANTOM-SIG", NodeObjective::Event,    2, 2, 0,  0,  0.f,  0, NodeStatus::Locked,   {0,3}          },
    };
}

// ---------------------------------------------------------------------------
// World 2 — THE CORE  (10 nodes, Vortex boss)
//
// (0,0)=BULWARK-(1,0)=TERMINUS-(2,0)=BREACH-(3,0)=ECHO-CORE[EXT]
//      |              |           |               |
// (0,1)=MATRIX   (1,1)=SENTINEL (2,1)=NULL-ECHO[EVT]  (3,1)=NULL-SECTOR[EXT]
//      |                |
// (0,2)=VOID-POCKET[EVT] (1,2)=VORTEX-CORE[BOSS]
// ---------------------------------------------------------------------------
void NodeMap::buildWorld2() {
    m_nodes = {
        { 0, "BULWARK",      NodeObjective::Sweep,    3, 0, 0, 18,  0.f,  0, NodeStatus::Available, {1,3}          },
        { 1, "TERMINUS",     NodeObjective::Survive,  3, 1, 0,  0, 30.f,  0, NodeStatus::Locked,   {0,2,4}        },
        { 2, "BREACH",       NodeObjective::Sweep,    3, 2, 0, 20,  0.f,  0, NodeStatus::Locked,   {1,6,8}        },
        { 3, "MATRIX",       NodeObjective::Survive,  4, 0, 1,  0, 40.f,  0, NodeStatus::Locked,   {0,4,9}        },
        { 4, "SENTINEL",     NodeObjective::Sweep,    4, 1, 1, 25,  0.f,  0, NodeStatus::Locked,   {1,3,5,7,8}    },
        { 5, "VORTEX-CORE",  NodeObjective::Boss,     4, 1, 2, 30,  0.f,  0, NodeStatus::Locked,   {4}            },
        { 6, "ECHO-CORE",    NodeObjective::Extract,  3, 3, 0,  0,  0.f, 12, NodeStatus::Locked,   {2,7}          },
        { 7, "NULL-SECTOR",  NodeObjective::Extract,  4, 3, 1,  0,  0.f, 16, NodeStatus::Locked,   {6,4}          },
        { 8, "NULL-ECHO",    NodeObjective::Event,    3, 2, 1,  0,  0.f,  0, NodeStatus::Locked,   {2,4}          },
        { 9, "VOID-POCKET",  NodeObjective::Event,    4, 0, 2,  0,  0.f,  0, NodeStatus::Locked,   {3}            },
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
