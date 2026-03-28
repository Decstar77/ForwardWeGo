#pragma once

#include "atto_render_model.h"
#include "atto_render_material.h"

namespace atto {

    // =========================================================================
    // Material data (for serialization — stores paths instead of loaded textures)
    // =========================================================================

    struct MaterialData {
        Vec3        albedo = Vec3( 0.8f );
        f32         metalic = 0.0f;
        f32         roughness = 0.5f;
        std::string albedoTexturePath;
        std::string metalicTexturePath;
    };

    // =========================================================================
    // Material / serialization helpers (Assimp-free)
    // =========================================================================

    void SerializeMaterialData( Serializer & serializer, MaterialData & mat );
    Material MaterialDataToMaterial( const MaterialData & data );

    void SerializeBoneNode( Serializer & serializer, BoneNode & node );
    void SerializeAnimationChannel( Serializer & serializer, BoneAnimationChannel & channel );
    void SerializeAnimationClip( Serializer & serializer, AnimationClip & clip );

} // namespace atto
