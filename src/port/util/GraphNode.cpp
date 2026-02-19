#include "GraphNode.h"

#include "sm64.h"

GraphNode* GraphNodeManager::GetNode(GraphNodeID id) {
    return gLoadedGraphNodes[id];
}

GraphNodeID GraphNodeManager::GetNodeID(GraphNode* graphNode) {
    for (int i = 0; i < 256; i++) {
        if (gLoadedGraphNodes[i] == graphNode) {
            return i;
        }
    }
    return -1;
}