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
    void reset();   // rebuild map for a new run

    int startNode() const { return 0; }

private:
    std::vector<Node> m_nodes;
    void buildDungeon();
};
