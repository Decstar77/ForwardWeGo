#include "game_entity.h"

namespace atto {
    Mat4 Entity::GetModelMatrix() const {
        Mat4 modelMatrix = Mat4( 1.0f );
        modelMatrix = glm::translate( modelMatrix, position ) * Mat4( orientation );
        return modelMatrix;
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
}
