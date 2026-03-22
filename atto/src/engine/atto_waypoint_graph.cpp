#include "atto_waypoint_graph.h"
#include "atto_renderer.h"

#include <algorithm>
#include <limits>

namespace atto {

    WaypointGraph::WaypointGraph() {
    }

    void WaypointGraph::AddWaypoint( Vec3 position ) {
        WaypointNode & node = nodes.emplace_back();
        node.index    = static_cast<i32>( nodes.size() ) - 1;
        node.position = position;
    }

    void WaypointGraph::ConnectWaypoints( i32 nodeAIndex, i32 nodeBIndex ) {
        if ( nodeAIndex < 0 || nodeAIndex >= GetNodeCount() ) { return; }
        if ( nodeBIndex < 0 || nodeBIndex >= GetNodeCount() ) { return; }
        if ( nodeAIndex == nodeBIndex ) { return; }

        WaypointNode & nodeA = nodes[ nodeAIndex ];
        WaypointNode & nodeB = nodes[ nodeBIndex ];

        f32 dist = Distance( nodeA.position, nodeB.position );

        ATTO_ASSERT( nodeA.edges.IsFull() == false, "Waypoint node is full" );
        ATTO_ASSERT( nodeB.edges.IsFull() == false, "Waypoint node is full" );

        // A -> B
        if ( !nodeA.edges.IsFull() ) {
            bool alreadyConnected = false;
            for ( i32 i = 0; i < nodeA.edges.GetCount(); i++ ) {
                if ( nodeA.edges[ i ].nodeBIndex == nodeBIndex ) {
                    alreadyConnected = true;
                    break;
                }
            }
            if ( !alreadyConnected ) {
                WaypointEdge & edgeAB = nodeA.edges.AddEmpty();
                edgeAB.nodeAIndex = nodeAIndex;
                edgeAB.nodeBIndex = nodeBIndex;
                edgeAB.distance   = dist;
                edgeAB.cost       = dist;
                edgeAB.enabled    = true;
            }
        }

        // B -> A (bidirectional)
        if ( !nodeB.edges.IsFull() ) {
            bool alreadyConnected = false;
            for ( i32 i = 0; i < nodeB.edges.GetCount(); i++ ) {
                if ( nodeB.edges[ i ].nodeBIndex == nodeAIndex ) {
                    alreadyConnected = true;
                    break;
                }
            }
            if ( !alreadyConnected ) {
                WaypointEdge & edgeBA = nodeB.edges.AddEmpty();
                edgeBA.nodeAIndex = nodeBIndex;
                edgeBA.nodeBIndex = nodeAIndex;
                edgeBA.distance   = dist;
                edgeBA.cost       = dist;
                edgeBA.enabled    = true;
            }
        }
    }

    void WaypointGraph::DisconnectWaypoints( i32 nodeAIndex, i32 nodeBIndex ) {
        if ( nodeAIndex < 0 || nodeAIndex >= GetNodeCount() ) { return; }
        if ( nodeBIndex < 0 || nodeBIndex >= GetNodeCount() ) { return; }

        auto removeEdge = [&]( WaypointNode & node, i32 targetIndex ) {
            for ( i32 i = 0; i < node.edges.GetCount(); i++ ) {
                if ( node.edges[ i ].nodeBIndex == targetIndex ) {
                    node.edges.RemoveIndex( i );
                    break;
                }
            }
        };

        removeEdge( nodes[ nodeAIndex ], nodeBIndex );
        removeEdge( nodes[ nodeBIndex ], nodeAIndex );
    }

    void WaypointGraph::RemoveNode( i32 index ) {
        if ( index < 0 || index >= GetNodeCount() ) { return; }

        // Remove all edges referencing this node from other nodes
        for ( WaypointNode & node : nodes ) {
            for ( i32 i = node.edges.GetCount() - 1; i >= 0; i-- ) {
                if ( node.edges[ i ].nodeBIndex == index ) {
                    node.edges.RemoveIndex( i );
                }
            }
        }

        nodes.erase( nodes.begin() + index );

        // Remap all indices that shifted due to the removal
        for ( i32 i = 0; i < GetNodeCount(); i++ ) {
            nodes[ i ].index = i;
            for ( i32 e = 0; e < nodes[ i ].edges.GetCount(); e++ ) {
                WaypointEdge & edge = nodes[ i ].edges[ e ];
                edge.nodeAIndex = i;
                if ( edge.nodeBIndex > index ) {
                    edge.nodeBIndex--;
                }
            }
        }
    }

    void WaypointGraph::Clear() {
        nodes.clear();
    }

    i32 WaypointGraph::FindNearestNode( Vec3 position ) const {
        i32 best     = -1;
        f32 bestDist = std::numeric_limits<f32>::max();
        for ( i32 i = 0; i < GetNodeCount(); i++ ) {
            f32 d = Distance( nodes[ i ].position, position );
            if ( d < bestDist ) {
                bestDist = d;
                best     = i;
            }
        }
        return best;
    }

    std::vector<i32> WaypointGraph::FindPath( i32 startIndex, i32 goalIndex ) {
        const i32 n = GetNodeCount();
        if ( startIndex < 0 || startIndex >= n ) { return {}; }
        if ( goalIndex  < 0 || goalIndex  >= n ) { return {}; }
        if ( startIndex == goalIndex ) { return { startIndex }; }

        struct NodeData {
            f32  g      = std::numeric_limits<f32>::max();
            f32  f      = std::numeric_limits<f32>::max();
            i32  prev   = -1;
            bool closed = false;
        };

        std::vector<NodeData> data( n );
        data[ startIndex ].g = 0.0f;
        data[ startIndex ].f = Distance( nodes[ startIndex ].position, nodes[ goalIndex ].position );

        // Min-heap on f, stored as node indices
        std::vector<i32> open;
        open.push_back( startIndex );

        auto cmp = [&]( i32 a, i32 b ) {
            return data[ a ].f > data[ b ].f;
        };

        std::make_heap( open.begin(), open.end(), cmp );

        while ( !open.empty() ) {
            std::pop_heap( open.begin(), open.end(), cmp );
            i32 current = open.back();
            open.pop_back();

            if ( data[ current ].closed ) { continue; }
            data[ current ].closed = true;

            if ( current == goalIndex ) { break; }

            const WaypointNode & node = nodes[ current ];
            for ( i32 e = 0; e < node.edges.GetCount(); e++ ) {
                const WaypointEdge & edge = node.edges[ e ];
                if ( !edge.enabled ) { continue; }

                i32 nb = edge.nodeBIndex;
                if ( data[ nb ].closed ) { continue; }

                f32 tentativeG = data[ current ].g + edge.cost;
                if ( tentativeG < data[ nb ].g ) {
                    data[ nb ].g    = tentativeG;
                    data[ nb ].f    = tentativeG + Distance( nodes[ nb ].position, nodes[ goalIndex ].position );
                    data[ nb ].prev = current;
                    open.push_back( nb );
                    std::push_heap( open.begin(), open.end(), cmp );
                }
            }
        }

        if ( data[ goalIndex ].prev == -1 ) { return {}; }

        // Reconstruct path by walking prev pointers
        std::vector<i32> path;
        for ( i32 cur = goalIndex; cur != -1; cur = data[ cur ].prev ) {
            path.push_back( cur );
        }
        std::reverse( path.begin(), path.end() );
        return path;
    }

    void WaypointGraph::DebugDraw( Renderer & renderer ) const {
        for ( const WaypointNode & node : nodes ) {
            const f32  r = 0.15f;
            const Vec3 & p = node.position;
            renderer.DebugLine( p - Vec3( r, 0, 0 ), p + Vec3( r, 0, 0 ), Vec3( 0.2f, 0.8f, 0.2f ) );
            renderer.DebugLine( p - Vec3( 0, r, 0 ), p + Vec3( 0, r, 0 ), Vec3( 0.2f, 0.8f, 0.2f ) );
            renderer.DebugLine( p - Vec3( 0, 0, r ), p + Vec3( 0, 0, r ), Vec3( 0.2f, 0.8f, 0.2f ) );

            for ( i32 e = 0; e < node.edges.GetCount(); e++ ) {
                const WaypointEdge & edge = node.edges[ e ];
                if ( edge.nodeBIndex > node.index ) { // draw each undirected edge once
                    Vec3 colour = edge.enabled ? Vec3( 0.2f, 0.6f, 1.0f ) : Vec3( 0.5f, 0.5f, 0.5f );
                    renderer.DebugLine( node.position, nodes[ edge.nodeBIndex ].position, colour );
                }
            }
        }
    }

} // namespace atto
