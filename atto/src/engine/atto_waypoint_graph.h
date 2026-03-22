#pragma once

#include "atto_core.h"
#include "atto_math.h"
#include "atto_containers.h"

#include <vector>

namespace atto {

    class Renderer;

    struct WaypointEdge {
        i32 nodeAIndex;
        i32 nodeBIndex;
        f32 distance;
        f32 cost;
        bool enabled;
    };

    struct WaypointNode {
        i32 index;
        Vec3 position;
        FixedList<WaypointEdge, 16> edges;
    };

    class WaypointGraph {
    public:
                                WaypointGraph();

        void                    AddWaypoint( Vec3 position );
        void                    ConnectWaypoints( i32 nodeAIndex, i32 nodeBIndex );
        void                    DisconnectWaypoints( i32 nodeAIndex, i32 nodeBIndex );
        void                    RemoveNode( i32 index );
        void                    Clear();

        const WaypointNode &    GetWaypoint( i32 index ) const { return nodes[index]; }
        WaypointNode &          GetWaypoint( i32 index ) { return nodes[index]; }
        i32                     GetNodeCount() const { return static_cast<i32>(nodes.size()); }

        // Returns the index of the node closest to the given position, or -1 if empty.
        i32                     FindNearestNode( Vec3 position ) const;

        // A* pathfinding. Returns an ordered list of node indices from start to goal,
        // inclusive. Returns empty if no path exists.
        std::vector<i32>        FindPath( i32 startIndex, i32 goalIndex );

        void                    DebugDraw( Renderer & renderer ) const;

    private:
        std::vector<WaypointNode> nodes;
    };
}
