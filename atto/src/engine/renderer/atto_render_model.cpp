#include "atto_render_model.h"
#include "atto_render_model_helpers.h"
#include "../atto_engine.h"
#include "../atto_log.h"

#include <glad/glad.h>

namespace atto {

    // =========================================================================
    // Mesh (static, unskinned)
    // =========================================================================

    void Mesh::Create( const std::vector<Vertex> & vertices, const std::vector<u32> & indices ) {
        indexCount = static_cast<i32>(indices.size());

        if ( !vertices.empty() ) {
            bounds.min = vertices[0].position;
            bounds.max = vertices[0].position;
            for ( const Vertex & v : vertices ) {
                bounds.min = glm::min( bounds.min, v.position );
                bounds.max = glm::max( bounds.max, v.position );
            }
        }

        glGenVertexArrays( 1, &vao );
        glGenBuffers( 1, &vbo );
        glGenBuffers( 1, &ebo );

        glBindVertexArray( vao );

        glBindBuffer( GL_ARRAY_BUFFER, vbo );
        glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( Vertex ), vertices.data(), GL_STATIC_DRAW );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( u32 ), indices.data(), GL_STATIC_DRAW );

        // Position (location = 0)
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void *)offsetof( Vertex, position ) );
        glEnableVertexAttribArray( 0 );

        // Normal (location = 1)
        glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void *)offsetof( Vertex, normal ) );
        glEnableVertexAttribArray( 1 );

        // TexCoords (location = 2)
        glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void *)offsetof( Vertex, texCoords ) );
        glEnableVertexAttribArray( 2 );

        glBindVertexArray( 0 );
    }

    void Mesh::Destroy() {
        if ( ebo != 0 ) { glDeleteBuffers( 1, &ebo ); ebo = 0; }
        if ( vbo != 0 ) { glDeleteBuffers( 1, &vbo ); vbo = 0; }
        if ( vao != 0 ) { glDeleteVertexArrays( 1, &vao ); vao = 0; }
        indexCount = 0;
        bounds = {};
    }

    void Mesh::Draw( Shader * shader ) const {
        const bool hasTexture = material.albedoTexture != nullptr && material.albedoTexture->IsValid();
        shader->SetInt( "uHasAlbedoTexture", hasTexture ? 1 : 0 );
        if ( hasTexture ) {
            material.albedoTexture->Bind( 0 );
            shader->SetInt( "uAlbedoTexture", 0 );
        } else {
            shader->SetVec3( "uObjectColor", material.albedo );
        }

        glBindVertexArray( vao );
        glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0 );
        glBindVertexArray( 0 );
    }

    // =========================================================================
    // SkinnedMesh (with bone weights)
    // =========================================================================

    void AnimatedMesh::Create( const std::vector<AnimationVertex> & vertices, const std::vector<u32> & indices ) {
        indexCount = static_cast<i32>(indices.size());

        glGenVertexArrays( 1, &vao );
        glGenBuffers( 1, &vbo );
        glGenBuffers( 1, &ebo );

        glBindVertexArray( vao );

        glBindBuffer( GL_ARRAY_BUFFER, vbo );
        glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( AnimationVertex ), vertices.data(), GL_STATIC_DRAW );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( u32 ), indices.data(), GL_STATIC_DRAW );

        // Position (location = 0)
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( AnimationVertex ), (void *)offsetof( AnimationVertex, position ) );
        glEnableVertexAttribArray( 0 );

        // Normal (location = 1)
        glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( AnimationVertex ), (void *)offsetof( AnimationVertex, normal ) );
        glEnableVertexAttribArray( 1 );

        // TexCoords (location = 2)
        glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( AnimationVertex ), (void *)offsetof( AnimationVertex, texCoords ) );
        glEnableVertexAttribArray( 2 );

        // BoneIDs (location = 3) - integer attribute
        glVertexAttribIPointer( 3, MAX_BONES_PER_VERTEX, GL_INT, sizeof( AnimationVertex ), (void *)offsetof( AnimationVertex, boneIDs ) );
        glEnableVertexAttribArray( 3 );

        // BoneWeights (location = 4)
        glVertexAttribPointer( 4, MAX_BONES_PER_VERTEX, GL_FLOAT, GL_FALSE, sizeof( AnimationVertex ), (void *)offsetof( AnimationVertex, boneWeights ) );
        glEnableVertexAttribArray( 4 );

        glBindVertexArray( 0 );
    }

    void AnimatedMesh::Destroy() {
        if ( ebo != 0 ) { glDeleteBuffers( 1, &ebo ); ebo = 0; }
        if ( vbo != 0 ) { glDeleteBuffers( 1, &vbo ); vbo = 0; }
        if ( vao != 0 ) { glDeleteVertexArrays( 1, &vao ); vao = 0; }
        indexCount = 0;
    }

    void AnimatedMesh::Draw( Shader * shader ) const {
        shader->SetVec3( "uObjectColor", material.albedo );

        glBindVertexArray( vao );
        glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0 );
        glBindVertexArray( 0 );
    }

    void SerializeMaterialData( Serializer & serializer, MaterialData & mat ) {
        serializer( "Albedo", mat.albedo );
        serializer( "Metalic", mat.metalic );
        serializer( "Roughness", mat.roughness );
        serializer( "AlbedoTexture", mat.albedoTexturePath );
        serializer( "MetalicTexture", mat.metalicTexturePath );
    }

    Material MaterialDataToMaterial( const MaterialData & data ) {
        Material mat;
        mat.albedo = data.albedo;
        mat.metalic = data.metalic;
        mat.roughness = data.roughness;
        if ( !data.albedoTexturePath.empty() ) {
            mat.albedoTexture = Engine::Get().GetRenderer().GetOrLoadTexture( data.albedoTexturePath.c_str() );
        }
        if ( !data.metalicTexturePath.empty() ) {
            mat.metalicTexture = Engine::Get().GetRenderer().GetOrLoadTexture( data.metalicTexturePath.c_str() );
        }
        return mat;
    }

    void StaticModel::ComputeBounds() {
        if ( !meshes.empty() ) {
            bounds = meshes[0].GetBounds();
            for ( i32 i = 1; i < GetMeshCount(); i++ ) {
                const AlignedBox & mb = meshes[i].GetBounds();
                bounds.min = glm::min( bounds.min, mb.min );
                bounds.max = glm::max( bounds.max, mb.max );
            }
        }
    }

    void StaticModel::CreateFromMesh( const std::vector<Vertex> & vertices, const std::vector<u32> & indices ) {
        Destroy();
        Mesh mesh;
        mesh.Create( vertices, indices );
        meshes.push_back( std::move( mesh ) );
        ComputeBounds();
    }

    void StaticModel::Destroy() {
        for ( auto & mesh : meshes ) {
            mesh.Destroy();
        }
        LOG_INFO( "Destroyed model '%s' (%d meshes)", path.GetCStr(), GetMeshCount() );
        meshes.clear();
        bounds = {};
    }

    void StaticModel::Draw( Shader * shader ) const {
        for ( const auto & mesh : meshes ) {
            mesh.Draw( shader );
        }
    }

    void StaticModel::Serialize( Serializer & serializer ) {
        Destroy();

        serializer( "Path", path );

        i32 meshCount = 0;
        serializer( "MeshCount", meshCount );

        for ( i32 i = 0; i < meshCount; i++ ) {
            std::vector<u8> vertexBytes;
            std::vector<u8> indexBytes;
            MaterialData matData;

            serializer( "Vertices", vertexBytes );
            serializer( "Indices", indexBytes );
            SerializeMaterialData( serializer, matData );

            i32 vertCount = static_cast<i32>( vertexBytes.size() / sizeof( Vertex ) );
            i32 idxCount  = static_cast<i32>( indexBytes.size() / sizeof( u32 ) );

            std::vector<Vertex> vertices( vertCount );
            std::memcpy( vertices.data(), vertexBytes.data(), vertexBytes.size() );

            std::vector<u32> indices( idxCount );
            std::memcpy( indices.data(), indexBytes.data(), indexBytes.size() );

            Mesh mesh;
            mesh.Create( vertices, indices );
            mesh.SetMaterial( MaterialDataToMaterial( matData ) );
            meshes.push_back( std::move( mesh ) );
        }

        ComputeBounds();
    }

    // =========================================================================
    // AnimatedModel loading helpers
    // =========================================================================

    AnimationVertex::AnimationVertex() {
        for ( i32 i = 0; i < MAX_BONES_PER_VERTEX; i++ ) {
            boneIDs[i] = -1;
            boneWeights[i] = 0.0f;
        }
    }

    void AnimationVertex::AddBoneData( i32 boneID, f32 weight ) {
        for ( i32 i = 0; i < MAX_BONES_PER_VERTEX; i++ ) {
            if ( boneIDs[i] < 0 ) {
                boneIDs[i] = boneID;
                boneWeights[i] = weight;
                return;
            }
        }

        ATTO_ASSERT( false, "AnimationVertex: Exceeded maximum bones per vertex" );
    }

    void AnimatedModel::Destroy() {
        for ( auto & mesh : meshes ) {
            mesh.Destroy();
        }
        meshes.clear();
        boneInfoMap.clear();
        boneCounter = 0;
        rootNode = BoneNode{};
        globalInverseTransform = Mat4( 1.0f );
        animations.clear();
        path.Clear();
    }

    void AnimatedModel::Draw( Shader * shader ) const {
        for ( const auto & mesh : meshes ) {
            mesh.Draw( shader );
        }
    }

    void AnimatedModel::DebugPrint() const {
        // Print bones
        LOG_INFO( "BoneCount: %d", GetBoneCount() );
        for ( const auto & bonePair : boneInfoMap ) {
            const std::string & boneName = bonePair.first;
            const BoneInfo & boneInfo = bonePair.second;
            LOG_INFO( "  Bone: '%s' (id=%d)", boneName.c_str(), boneInfo.id );
        }

        // Print animations
        LOG_INFO( "AnimationCount: %d", GetAnimationCount() );
        for ( i32 i = 0; i < GetAnimationCount(); i++ ) {
            const AnimationClip & clip = animations[i];
            LOG_INFO( "  Animation[%d]: '%s' (duration: %.2f, ticks/s: %.2f, channels: %d)",
                i, clip.name.c_str(), clip.duration, clip.ticksPerSecond, (i32)clip.channels.size()
            );
            for ( const auto & channel : clip.channels ) {
                LOG_INFO( "    Channel: '%s' (pos: %zu keys, rot: %zu keys, scale: %zu keys)",
                    channel.boneName.c_str(),
                    channel.positionKeys.size(),
                    channel.rotationKeys.size(),
                    channel.scaleKeys.size()
                );
            }
        }
    }

    const i32 AnimatedModel::GetAnimationIndex( const char * name ) const {
        ATTO_ASSERT( name != nullptr, "Name is null" );
        for ( i32 i = 0; i < static_cast<i32>( animations.size() ); i++ ) {
            if ( animations[i].name == name ) {
                return i;
            }
        }
        return -1;
    }

    // =========================================================================
    // AnimatedModel serialization helpers
    // =========================================================================

    void SerializeBoneNode( Serializer & serializer, BoneNode & node ) {
        serializer( "Name", node.name );
        serializer( "Transform", node.transformation );
        i32 childCount = static_cast<i32>( node.children.size() );
        serializer( "ChildCount", childCount );
        if ( serializer.IsLoading() ) {
            node.children.resize( childCount );
        }
        for ( i32 i = 0; i < childCount; i++ ) {
            SerializeBoneNode( serializer, node.children[i] );
        }
    }

    void SerializeAnimationChannel( Serializer & serializer, BoneAnimationChannel & channel ) {
        serializer( "BoneName", channel.boneName );

        // Position keyframes as raw bytes (POD: f32 time + Vec3 position = 16 bytes)
        if ( serializer.IsSaving() ) {
            std::vector<u8> posBytes( channel.positionKeys.size() * sizeof( PositionKeyframe ) );
            std::memcpy( posBytes.data(), channel.positionKeys.data(), posBytes.size() );
            serializer( "PosKeys", posBytes );
        }
        else {
            std::vector<u8> posBytes;
            serializer( "PosKeys", posBytes );
            channel.positionKeys.resize( posBytes.size() / sizeof( PositionKeyframe ) );
            std::memcpy( channel.positionKeys.data(), posBytes.data(), posBytes.size() );
        }

        // Rotation keyframes (POD: f32 time + Quat rotation = 20 bytes)
        if ( serializer.IsSaving() ) {
            std::vector<u8> rotBytes( channel.rotationKeys.size() * sizeof( RotationKeyframe ) );
            std::memcpy( rotBytes.data(), channel.rotationKeys.data(), rotBytes.size() );
            serializer( "RotKeys", rotBytes );
        }
        else {
            std::vector<u8> rotBytes;
            serializer( "RotKeys", rotBytes );
            channel.rotationKeys.resize( rotBytes.size() / sizeof( RotationKeyframe ) );
            std::memcpy( channel.rotationKeys.data(), rotBytes.data(), rotBytes.size() );
        }

        // Scale keyframes (POD: f32 time + Vec3 scale = 16 bytes)
        if ( serializer.IsSaving() ) {
            std::vector<u8> scaleBytes( channel.scaleKeys.size() * sizeof( ScaleKeyframe ) );
            std::memcpy( scaleBytes.data(), channel.scaleKeys.data(), scaleBytes.size() );
            serializer( "ScaleKeys", scaleBytes );
        }
        else {
            std::vector<u8> scaleBytes;
            serializer( "ScaleKeys", scaleBytes );
            channel.scaleKeys.resize( scaleBytes.size() / sizeof( ScaleKeyframe ) );
            std::memcpy( channel.scaleKeys.data(), scaleBytes.data(), scaleBytes.size() );
        }
    }

    void SerializeAnimationClip( Serializer & serializer, AnimationClip & clip ) {
        serializer( "Name", clip.name );
        serializer( "Duration", clip.duration );
        serializer( "TicksPerSecond", clip.ticksPerSecond );
        i32 channelCount = static_cast<i32>( clip.channels.size() );
        serializer( "ChannelCount", channelCount );
        if ( serializer.IsLoading() ) {
            clip.channels.resize( channelCount );
        }
        for ( i32 i = 0; i < channelCount; i++ ) {
            SerializeAnimationChannel( serializer, clip.channels[i] );
        }
    }

    void AnimatedModel::Serialize( Serializer & serializer ) {
        Destroy();

        serializer( "Path", path );

        // Meshes
        i32 meshCount = 0;
        serializer( "MeshCount", meshCount );

        for ( i32 i = 0; i < meshCount; i++ ) {
            std::vector<u8> vertexBytes;
            std::vector<u8> indexBytes;
            MaterialData matData;

            serializer( "Vertices", vertexBytes );
            serializer( "Indices", indexBytes );
            SerializeMaterialData( serializer, matData );

            i32 vertCount = static_cast<i32>( vertexBytes.size() / sizeof( AnimationVertex ) );
            i32 idxCount  = static_cast<i32>( indexBytes.size() / sizeof( u32 ) );

            std::vector<AnimationVertex> vertices( vertCount );
            std::memcpy( vertices.data(), vertexBytes.data(), vertexBytes.size() );

            std::vector<u32> indices( idxCount );
            std::memcpy( indices.data(), indexBytes.data(), indexBytes.size() );

            AnimatedMesh mesh;
            mesh.Create( vertices, indices );
            mesh.SetMaterial( MaterialDataToMaterial( matData ) );
            meshes.push_back( std::move( mesh ) );
        }

        // Bone info map
        i32 boneMapCount = 0;
        serializer( "BoneMapCount", boneMapCount );
        for ( i32 i = 0; i < boneMapCount; i++ ) {
            std::string boneName;
            BoneInfo info;
            serializer( "BoneName", boneName );
            serializer( "BoneId", info.id );
            serializer( "BoneOffset", info.offsetMatrix );
            boneInfoMap[boneName] = info;
        }

        serializer( "BoneCounter", boneCounter );
        serializer( "GlobalInverseTransform", globalInverseTransform );

        // Bone hierarchy
        SerializeBoneNode( serializer, rootNode );

        // Animations
        i32 animCount = 0;
        serializer( "AnimCount", animCount );
        if ( serializer.IsLoading() ) {
            animations.resize( animCount );
        }
        for ( i32 i = 0; i < animCount; i++ ) {
            SerializeAnimationClip( serializer, animations[i] );
        }
    }

    // =========================================================================
    // Animator
    // =========================================================================

    Animator::Animator() {
        for ( i32 i = 0; i < MAX_BONES; i++ ) {
            finalBoneMatrices[i] = Mat4( 1.0f );
        }
    }

    void Animator::PlayAnimation( const AnimatedModel & model, const char * animationName, bool loop, f32 blendTime ) {
        const i32 index = model.GetAnimationIndex( animationName );
        ATTO_ASSERT( index >= 0, "Invalid animation index" );
        PlayAnimation( model, index, loop, blendTime );
    }

    void Animator::PlayAnimation( const AnimatedModel & animModel, i32 animationIndex, bool loop, f32 blendTime ) {
        // Save current state for crossfade blending
        if ( currentClip && blendTime > 0.0f ) {
            prevClip = currentClip;
            prevTime = currentTime;
            prevLooping = looping;
            blendDuration = blendTime;
            blendTimer = 0.0f;
        }
        else {
            prevClip = nullptr;
            blendDuration = 0.0f;
            blendTimer = 0.0f;
        }

        model = &animModel;
        looping = loop;
        currentTime = 0.0f;

        const auto & animations = model->GetAnimations();
        if ( animationIndex >= 0 && animationIndex < static_cast<i32>( animations.size() ) ) {
            currentClip = &animations[animationIndex];
        }
        else {
            currentClip = nullptr;
            LOG_WARN( "Animation index %d out of range (model has %d animations)", animationIndex, static_cast<i32>( animations.size() ) );
        }
    }

    void Animator::Update( f32 dt ) {
        if ( !currentClip || !model ) {
            return;
        }

        // Advance crossfade blend
        if ( prevClip && blendDuration > 0.0f ) {
            blendTimer += dt;
            if ( blendTimer >= blendDuration ) {
                blendTimer = blendDuration;
                prevClip = nullptr; // Blend complete
            }
        }

        // Advance previous animation (keeps playing during blend)
        if ( prevClip ) {
            prevTime += dt * prevClip->ticksPerSecond;
            if ( prevLooping ) {
                prevTime = fmod( prevTime, prevClip->duration );
            }
            else {
                if ( prevTime > prevClip->duration ) {
                    prevTime = prevClip->duration;
                }
            }
        }

        // Advance current animation
        f32 ticksPerSecond = currentClip->ticksPerSecond;
        currentTime += dt * ticksPerSecond;

        if ( looping ) {
            currentTime = fmod( currentTime, currentClip->duration );
        }
        else {
            if ( currentTime > currentClip->duration ) {
                currentTime = currentClip->duration;
            }
        }

        CalculateBoneTransform( model->GetRootNode(), Mat4( 1.0f ) );
    }

    void Animator::CalculateBoneTransform( const BoneNode & node, const Mat4 & parentTransform ) {
        Mat4 nodeTransform = node.transformation;

        const BoneAnimationChannel * channel = FindChannel( currentClip, node.name );
        if ( channel ) {
            Vec3 position = InterpolatePosition( currentTime, *channel );
            Quat rotation = InterpolateRotation( currentTime, *channel );
            Vec3 scale    = InterpolateScale( currentTime, *channel );

            // Crossfade blend with previous animation
            if ( prevClip && blendDuration > 0.0f ) {
                f32 t = blendTimer / blendDuration; // 0 = full prev, 1 = full current
                const BoneAnimationChannel * prevChannel = FindChannel( prevClip, node.name );
                if ( prevChannel ) {
                    Vec3 prevPos   = InterpolatePosition( prevTime, *prevChannel );
                    Quat prevRot   = InterpolateRotation( prevTime, *prevChannel );
                    Vec3 prevScale = InterpolateScale( prevTime, *prevChannel );

                    position = Lerp( prevPos, position, t );
                    rotation = glm::normalize( glm::slerp( prevRot, rotation, t ) );
                    scale    = Lerp( prevScale, scale, t );
                }
            }

            Mat4 translationMatrix = glm::translate( Mat4( 1.0f ), position );
            Mat4 rotationMatrix    = glm::mat4_cast( rotation );
            Mat4 scaleMatrix       = glm::scale( Mat4( 1.0f ), scale );

            nodeTransform = translationMatrix * rotationMatrix * scaleMatrix;
        }

        Mat4 globalTransform = parentTransform * nodeTransform;

        const auto & boneInfoMap = model->GetBoneInfoMap();
        auto it = boneInfoMap.find( node.name );
        if ( it != boneInfoMap.end() ) {
            const BoneInfo & boneInfo = it->second;
            finalBoneMatrices[boneInfo.id] = model->GetGlobalInverseTransform() * globalTransform * boneInfo.offsetMatrix;
        }

        for ( const auto & child : node.children ) {
            CalculateBoneTransform( child, globalTransform );
        }
    }

    const BoneAnimationChannel * Animator::FindChannel( const AnimationClip * clip, const std::string & nodeName ) {
        if ( !clip ) return nullptr;

        for ( const auto & channel : clip->channels ) {
            if ( channel.boneName == nodeName ) {
                return &channel;
            }
        }
        return nullptr;
    }

    Vec3 Animator::InterpolatePosition( f32 animTime, const BoneAnimationChannel & channel ) {
        const auto & keys = channel.positionKeys;
        if ( keys.size() == 1 ) {
            return keys[0].position;
        }

        if ( animTime >= keys.back().time ) {
            return keys.back().position;
        }

        i32 index = 0;
        for ( i32 i = 0; i < static_cast<i32>( keys.size() ) - 1; i++ ) {
            if ( animTime < keys[i + 1].time ) {
                index = i;
                break;
            }
        }

        i32 nextIndex = index + 1;
        f32 deltaTime = keys[nextIndex].time - keys[index].time;
        f32 factor = (deltaTime > 0.0f) ? (animTime - keys[index].time) / deltaTime : 0.0f;
        factor = Clamp( factor, 0.0f, 1.0f );

        return Lerp( keys[index].position, keys[nextIndex].position, factor );
    }

    Quat Animator::InterpolateRotation( f32 animTime, const BoneAnimationChannel & channel ) {
        const auto & keys = channel.rotationKeys;
        if ( keys.size() == 1 ) {
            return glm::normalize( keys[0].rotation );
        }

        if ( animTime >= keys.back().time ) {
            return glm::normalize( keys.back().rotation );
        }

        i32 index = 0;
        for ( i32 i = 0; i < static_cast<i32>( keys.size() ) - 1; i++ ) {
            if ( animTime < keys[i + 1].time ) {
                index = i;
                break;
            }
        }

        i32 nextIndex = index + 1;
        f32 deltaTime = keys[nextIndex].time - keys[index].time;
        f32 factor = (deltaTime > 0.0f) ? (animTime - keys[index].time) / deltaTime : 0.0f;
        factor = Clamp( factor, 0.0f, 1.0f );

        return glm::normalize( glm::slerp( keys[index].rotation, keys[nextIndex].rotation, factor ) );
    }

    Vec3 Animator::InterpolateScale( f32 animTime, const BoneAnimationChannel & channel ) {
        const auto & keys = channel.scaleKeys;
        if ( keys.size() == 1 ) {
            return keys[0].scale;
        }

        if ( animTime >= keys.back().time ) {
            return keys.back().scale;
        }

        i32 index = 0;
        for ( i32 i = 0; i < static_cast<i32>( keys.size() ) - 1; i++ ) {
            if ( animTime < keys[i + 1].time ) {
                index = i;
                break;
            }
        }

        i32 nextIndex = index + 1;
        f32 deltaTime = keys[nextIndex].time - keys[index].time;
        f32 factor = (deltaTime > 0.0f) ? (animTime - keys[index].time) / deltaTime : 0.0f;
        factor = Clamp( factor, 0.0f, 1.0f );

        return Lerp( keys[index].scale, keys[nextIndex].scale, factor );
    }

    // =========================================================================
    // Brush
    // =========================================================================

    void Brush::ToStaticModel( StaticModel & model ) const {
        const Vec3 & c = center;
        const Vec3 & h = halfExtents;

        // 6 faces, 4 vertices each = 24 vertices, 6 indices each = 36 indices
        std::vector<Vertex> vertices( 24 );
        std::vector<u32> indices( 36 );

        const f32 s = textureScale > 0.0001f ? textureScale : 1.0f;

        // World-space tiled UVs: project position onto face plane, divide by scale
        auto setFace = [&]( i32 face, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, Vec3 n, i32 uAxis, i32 vAxis )
            {
                i32 base = face * 4;
                vertices[base + 0] = { p0, n, Vec2( p0[uAxis] / s, p0[vAxis] / s ) };
                vertices[base + 1] = { p1, n, Vec2( p1[uAxis] / s, p1[vAxis] / s ) };
                vertices[base + 2] = { p2, n, Vec2( p2[uAxis] / s, p2[vAxis] / s ) };
                vertices[base + 3] = { p3, n, Vec2( p3[uAxis] / s, p3[vAxis] / s ) };

                i32 idx = face * 6;
                indices[idx + 0] = base + 0;
                indices[idx + 1] = base + 1;
                indices[idx + 2] = base + 2;
                indices[idx + 3] = base + 0;
                indices[idx + 4] = base + 2;
                indices[idx + 5] = base + 3;
            };

        // +Z face (front):  project onto X,Y
        setFace( 0,
            c + Vec3( -h.x, -h.y, +h.z ),
            c + Vec3( +h.x, -h.y, +h.z ),
            c + Vec3( +h.x, +h.y, +h.z ),
            c + Vec3( -h.x, +h.y, +h.z ),
            Vec3( 0, 0, 1 ), 0, 1
        );
        // -Z face (back):   project onto X,Y
        setFace( 1,
            c + Vec3( +h.x, -h.y, -h.z ),
            c + Vec3( -h.x, -h.y, -h.z ),
            c + Vec3( -h.x, +h.y, -h.z ),
            c + Vec3( +h.x, +h.y, -h.z ),
            Vec3( 0, 0, -1 ), 0, 1
        );
        // +X face (right):  project onto Z,Y
        setFace( 2,
            c + Vec3( +h.x, -h.y, +h.z ),
            c + Vec3( +h.x, -h.y, -h.z ),
            c + Vec3( +h.x, +h.y, -h.z ),
            c + Vec3( +h.x, +h.y, +h.z ),
            Vec3( 1, 0, 0 ), 2, 1
        );
        // -X face (left):   project onto Z,Y
        setFace( 3,
            c + Vec3( -h.x, -h.y, -h.z ),
            c + Vec3( -h.x, -h.y, +h.z ),
            c + Vec3( -h.x, +h.y, +h.z ),
            c + Vec3( -h.x, +h.y, -h.z ),
            Vec3( -1, 0, 0 ), 2, 1
        );
        // +Y face (top):    project onto X,Z
        setFace( 4,
            c + Vec3( -h.x, +h.y, +h.z ),
            c + Vec3( +h.x, +h.y, +h.z ),
            c + Vec3( +h.x, +h.y, -h.z ),
            c + Vec3( -h.x, +h.y, -h.z ),
            Vec3( 0, 1, 0 ), 0, 2
        );
        // -Y face (bottom): project onto X,Z
        setFace( 5,
            c + Vec3( -h.x, -h.y, -h.z ),
            c + Vec3( +h.x, -h.y, -h.z ),
            c + Vec3( +h.x, -h.y, +h.z ),
            c + Vec3( -h.x, -h.y, +h.z ),
            Vec3( 0, -1, 0 ), 0, 2
        );

        model.CreateFromMesh( vertices, indices );
    }

    void Brush::Serialize( Serializer & serializer ) {
        i32 typeInt = static_cast<i32>( type );
        serializer( "type", typeInt );
        type = static_cast<BrushType>( typeInt );
        serializer( "center", center );
        serializer( "halfExtents", halfExtents );
        serializer( "texturePath", texturePath );
        serializer( "textureScale", textureScale );
    }

    bool Brush::IsPointInside( Vec3 point ) const {
        return  point.x >= center.x - halfExtents.x && point.x <= center.x + halfExtents.x &&
            point.y >= center.y - halfExtents.y && point.y <= center.y + halfExtents.y &&
            point.z >= center.z - halfExtents.z && point.z <= center.z + halfExtents.z;
    }

    bool Brush::IsPointInside( Vec3 point, i32 hAxis, i32 vAxis ) const {
        return  point[hAxis] >= center[hAxis] - halfExtents[hAxis] && point[hAxis] <= center[hAxis] + halfExtents[hAxis] &&
            point[vAxis] >= center[vAxis] - halfExtents[vAxis] && point[vAxis] <= center[vAxis] + halfExtents[vAxis];
    }

} // namespace atto
