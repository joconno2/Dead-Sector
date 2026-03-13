#pragma once
#include "world/Node.hpp"
#include <vector>

class NodeMap {
public:
    NodeMap();

    const std::vector<Node>& nodes() const { return m_nodes; }
    const Node& node(int id)  const { return m_nodes[id]; }
    Node&       node(int id)        { return m_nodes[id]; }

    void clearNode(int id);
    void reset(int worldId = 0);   // rebuild map for the given world
    void revealAll();              // unlock all Locked nodes (REVEAL_MAP shop upgrade)

    int startNode() const { return 0; }

private:
    std::vector<Node> m_nodes;
    void buildWorld0();  // SECTOR ALPHA — entry zone, Manticore boss
    void buildWorld1();  // DEEP NET     — hostile territory, Archon boss
    void buildWorld2();  // THE CORE     — final zone, Vortex boss
};
