#pragma once

#include "atto_core.h"
#include "atto_math.h"
#include "atto_containers.h"

namespace atto {

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

        const WaypointNode &    GetWaypoint( i32 index ) const { return nodes[index]; }
        WaypointNode &          GetWaypoint( i32 index ) { return nodes[index]; }
        i32                     GetNodeCount() const { return static_cast<i32>(nodes.size()); }

        std::vector<i32>        FindPath( i32 nodeAIndex, i32 nodeBIndex );

    private:
        std::vector<WaypointNode> nodes;
    };
}



