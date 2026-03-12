#pragma once
#include <string>
#include <vector>
#include "core/Programs.hpp"

enum class NodeObjective { Sweep, Survive, Boss };
enum class NodeStatus    { Locked, Available, Cleared };

struct Node {
    int            id;
    std::string    name;
    NodeObjective  objective;
    int            tier;          // 1–4: governs trace tick rate and spawn table
    int            gridCol;
    int            gridRow;
    int            sweepTarget;   // ICE kills required (Sweep / Boss)
    float          surviveSeconds;// seconds to outlast (Survive)
    NodeStatus     status   = NodeStatus::Locked;
    std::vector<int> connections; // adjacent node IDs (bidirectional)
    bool           revealed = false; // SHADOW SCANNER: show on map but still locked
};
