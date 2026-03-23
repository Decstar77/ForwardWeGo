#include "game_entity.h"

namespace atto {
    Mat4 Entity::GetModelMatrix() const {
        Mat4 modelMatrix = Mat4( 1.0f );
        modelMatrix = glm::translate( modelMatrix, position ) * Mat4( orientation );
        return modelMatrix;
    }

    void Entity::RotateXLocal( const f32 amount ) {
        const Mat3 r = Mat3( glm::rotate( Mat4( 1 ), amount, Vec3( orientation[0] ) ) );
        orientation = r * orientation;
    }

    void Entity::RotateYLocal( const f32 amount ) {
        const Mat3 r = Mat3( glm::rotate( Mat4( 1 ), amount, Vec3( orientation[0] ) ) );
        orientation = r * orientation;
    }

    void Entity::RotateZLocal( const f32 amount ) {
        const Mat3 r = Mat3( glm::rotate( Mat4( 1 ), amount, Vec3( orientation[2] ) ) );
        orientation = r * orientation;
    }

    void Entity::RotateXGlobal( const f32 amount ) {
        const Mat3 r = Mat3( glm::rotate( Mat4( 1 ), amount, Vec3( 1, 0, 0 ) ) );
        orientation = r * orientation;
        position = r * position;
    }

    void Entity::RotateYGlobal( const f32 amount ) {
        const Mat3 r = Mat3( glm::rotate( Mat4( 1 ), amount, Vec3( 0, 1, 0 ) ) );
        orientation = r * orientation;
        position = r * position;
    }

    void Entity::RotateZGlobal( const f32 amount ) {
        const Mat3 r = Mat3( glm::rotate( Mat4( 1 ), amount, Vec3( 0, 0, 1 ) ) );
        orientation = r * orientation;
        position = r * position;
    }

    void Entity::Serialize( Serializer & serializer ) {
        // @SPEED: We should use a SmallString for this but the serializer doesn't support it yet
        if ( serializer.IsSaving() ) {
            std::string typeStr = EntityTypeToString( type );
            serializer( "Type", typeStr );
        }
        else {
            std::string typeStr;
            serializer( "Type", typeStr );
            type = StringToEntityType( typeStr.c_str() );
        }

        serializer( "SpawnId", spawnId );
        serializer( "Position", position );
        serializer( "Orientation", orientation );
    }

    void Entity::DebugDrawBounds( Renderer &renderer ) {
        const AlignedBox box = GetBounds();
        renderer.DebugAlignedBox( box );
    }

    void Entity::DebugDrawCollider( Renderer &renderer ) {
        const Box box = GetCollider();
        renderer.DebugBox( box );
    }
}
