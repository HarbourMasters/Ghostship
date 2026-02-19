#pragma once

#include <unordered_map>

#include "sm64.h"
typedef int32_t GraphNodeID;

class GraphNodeManager {
public:
    static GraphNode* GetNode(GraphNodeID id);
    static GraphNodeID GetNodeID(GraphNode* graphNode);
};