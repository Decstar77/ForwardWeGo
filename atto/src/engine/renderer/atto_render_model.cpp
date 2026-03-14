#include "atto_render_model.h"
#include "../atto_log.h"

#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/texture.h>
#include <assimp/mesh.h>
#include <assimp/anim.h>

namespace atto {

    // =========================================================================
    // Assimp helpers
    // =========================================================================

    static inline Mat4 AiMat4ToGlm( const aiMatrix4x4 & m ) {
        return Mat4(
            m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4
        );
    }

    static inline Vec3 AiVec3ToGlm( const aiVector3D & v ) {
        return Vec3( v.x, v.y, v.z );
    }

    static inline Quat AiQuatToGlm( const aiQuaternion & q ) {
        return Quat( q.w, q.x, q.y, q.z );
    }

    // =========================================================================
    // Mesh (static, unskinned)
    // =========================================================================

    void Mesh::Create( const std::vector<Vertex> & vertices, const std::vector<u32> & indices ) {
        indexCount = static_cast<i32>(indices.size());

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
    }

    void Mesh::Draw() const {
        glBindVertexArray( vao );
        glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0 );
        glBindVertexArray( 0 );
    }

    // =========================================================================
    // SkinnedMesh (with bone weights)
    // =========================================================================

    void SkinnedMesh::Create( const std::vector<SkinnedVertex> & vertices, const std::vector<u32> & indices ) {
        indexCount = static_cast<i32>(indices.size());

        glGenVertexArrays( 1, &vao );
        glGenBuffers( 1, &vbo );
        glGenBuffers( 1, &ebo );

        glBindVertexArray( vao );

        glBindBuffer( GL_ARRAY_BUFFER, vbo );
        glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( SkinnedVertex ), vertices.data(), GL_STATIC_DRAW );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( u32 ), indices.data(), GL_STATIC_DRAW );

        // Position (location = 0)
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( SkinnedVertex ), (void *)offsetof( SkinnedVertex, position ) );
        glEnableVertexAttribArray( 0 );

        // Normal (location = 1)
        glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( SkinnedVertex ), (void *)offsetof( SkinnedVertex, normal ) );
        glEnableVertexAttribArray( 1 );

        // TexCoords (location = 2)
        glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( SkinnedVertex ), (void *)offsetof( SkinnedVertex, texCoords ) );
        glEnableVertexAttribArray( 2 );

        // BoneIDs (location = 3) - integer attribute
        glVertexAttribIPointer( 3, MAX_BONES_PER_VERTEX, GL_INT, sizeof( SkinnedVertex ), (void *)offsetof( SkinnedVertex, boneIDs ) );
        glEnableVertexAttribArray( 3 );

        // BoneWeights (location = 4)
        glVertexAttribPointer( 4, MAX_BONES_PER_VERTEX, GL_FLOAT, GL_FALSE, sizeof( SkinnedVertex ), (void *)offsetof( SkinnedVertex, boneWeights ) );
        glEnableVertexAttribArray( 4 );

        glBindVertexArray( 0 );
    }

    void SkinnedMesh::Destroy() {
        if ( ebo != 0 ) { glDeleteBuffers( 1, &ebo ); ebo = 0; }
        if ( vbo != 0 ) { glDeleteBuffers( 1, &vbo ); vbo = 0; }
        if ( vao != 0 ) { glDeleteVertexArrays( 1, &vao ); vao = 0; }
        indexCount = 0;
    }

    void SkinnedMesh::Draw() const {
        glBindVertexArray( vao );
        glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0 );
        glBindVertexArray( 0 );
    }

    // =========================================================================
    // StaticModel loading helpers
    // =========================================================================

    static Mesh ProcessMesh( aiMesh * mesh, const aiScene * scene, f32 scale ) {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;

        vertices.reserve( mesh->mNumVertices );
        for ( u32 i = 0; i < mesh->mNumVertices; i++ ) {
            Vertex vertex = {};

            vertex.position.x = mesh->mVertices[i].x * scale;
            vertex.position.y = mesh->mVertices[i].y * scale;
            vertex.position.z = mesh->mVertices[i].z * scale;

            if ( mesh->HasNormals() ) {
                vertex.normal.x = mesh->mNormals[i].x;
                vertex.normal.y = mesh->mNormals[i].y;
                vertex.normal.z = mesh->mNormals[i].z;
            }

            if ( mesh->mTextureCoords[0] ) {
                vertex.texCoords.x = mesh->mTextureCoords[0][i].x;
                vertex.texCoords.y = mesh->mTextureCoords[0][i].y;
            }

            vertices.push_back( vertex );
        }

        for ( u32 i = 0; i < mesh->mNumFaces; i++ ) {
            aiFace & face = mesh->mFaces[i];
            for ( u32 j = 0; j < face.mNumIndices; j++ ) {
                indices.push_back( face.mIndices[j] );
            }
        }

        Mesh result;
        result.Create( vertices, indices );
        return result;
    }

    static void PrintMaterials( const aiScene * scene ) {
        for ( u32 i = 0; i < scene->mNumMaterials; i++ ) {
            const aiMaterial * material = scene->mMaterials[i];
            aiString name;
            material->Get( AI_MATKEY_NAME, name );
            printf( "Material: %s\n", name.C_Str() );

            u32 numDiffuse = material->GetTextureCount( aiTextureType_DIFFUSE );
            for ( u32 t = 0; t < numDiffuse; ++t ) {
                aiString texPath;
                if ( material->GetTexture( aiTextureType_DIFFUSE, t, &texPath ) == AI_SUCCESS ) {
                    printf( "  Diffuse Texture: %s\n", texPath.C_Str() );
                }
            }

            u32 numSpecular = material->GetTextureCount( aiTextureType_SPECULAR );
            for ( u32 t = 0; t < numSpecular; ++t ) {
                aiString texPath;
                if ( material->GetTexture( aiTextureType_SPECULAR, t, &texPath ) == AI_SUCCESS ) {
                    printf( "  Specular Texture: %s\n", texPath.C_Str() );
                }
            }

            u32 numNormals = material->GetTextureCount( aiTextureType_NORMALS );
            for ( u32 t = 0; t < numNormals; ++t ) {
                aiString texPath;
                if ( material->GetTexture( aiTextureType_NORMALS, t, &texPath ) == AI_SUCCESS ) {
                    printf( "  Normal Texture: %s\n", texPath.C_Str() );
                }
            }
            u32 numBump = material->GetTextureCount( aiTextureType_HEIGHT );
            for ( u32 t = 0; t < numBump; ++t ) {
                aiString texPath;
                if ( material->GetTexture( aiTextureType_HEIGHT, t, &texPath ) == AI_SUCCESS ) {
                    printf( "  Bump Map: %s\n", texPath.C_Str() );
                }
            }
        }
    }

    static void ProcessNode( aiNode * node, const aiScene * scene, std::vector<Mesh> & meshes, f32 scale ) {
        for ( u32 i = 0; i < node->mNumMeshes; i++ ) {
            aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back( ProcessMesh( mesh, scene, scale ) );
        }

        for ( u32 i = 0; i < node->mNumChildren; i++ ) {
            ProcessNode( node->mChildren[i], scene, meshes, scale );
        }
    }

    void StaticModel::LoadFromFile( const char * filePath, f32 scale ) {
        Assimp::Importer importer;

        const aiScene * scene = importer.ReadFile( filePath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace
        );

        if ( !scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode ) {
            LOG_ERROR( "Assimp: %s", importer.GetErrorString() );
            return;
        }

        PrintMaterials( scene );
        ProcessNode( scene->mRootNode, scene, meshes, scale );

        LOG_INFO( "Loaded model '%s' (%d meshes)", filePath, GetMeshCount() );
    }

    void StaticModel::CreateFromMesh( const std::vector<Vertex> & vertices, const std::vector<u32> & indices ) {
        Destroy();
        Mesh mesh;
        mesh.Create( vertices, indices );
        meshes.push_back( std::move( mesh ) );
    }

    void StaticModel::Destroy() {
        for ( auto & mesh : meshes ) {
            mesh.Destroy();
        }
        meshes.clear();
    }

    void StaticModel::Draw() const {
        for ( const auto & mesh : meshes ) {
            mesh.Draw();
        }
    }

    // =========================================================================
    // AnimatedModel loading helpers
    // =========================================================================

    static void ExtractBoneData(
        aiMesh * mesh,
        std::vector<SkinnedVertex> & vertices,
        std::unordered_map<std::string, BoneInfo> & boneInfoMap,
        i32 & boneCounter
    ) {
        for ( u32 boneIdx = 0; boneIdx < mesh->mNumBones; boneIdx++ ) {
            aiBone * bone = mesh->mBones[boneIdx];
            std::string boneName( bone->mName.C_Str() );

            if ( boneInfoMap.find( boneName ) == boneInfoMap.end() ) {
                BoneInfo info;
                info.id = boneCounter;
                info.offsetMatrix = AiMat4ToGlm( bone->mOffsetMatrix );
                boneInfoMap[boneName] = info;
                boneCounter++;
            }

            i32 boneID = boneInfoMap[boneName].id;

            for ( u32 weightIdx = 0; weightIdx < bone->mNumWeights; weightIdx++ ) {
                u32 vertexID = bone->mWeights[weightIdx].mVertexId;
                f32 weight = bone->mWeights[weightIdx].mWeight;

                ATTO_ASSERT( vertexID < vertices.size(), "Bone weight vertex ID out of range" );
                vertices[vertexID].AddBoneData( boneID, weight );
            }
        }
    }

    static SkinnedMesh ProcessSkinnedMesh(
        aiMesh * mesh,
        const aiScene * scene,
        f32 scale,
        std::unordered_map<std::string, BoneInfo> & boneInfoMap,
        i32 & boneCounter
    ) {
        std::vector<SkinnedVertex> vertices;
        std::vector<u32> indices;

        vertices.resize( mesh->mNumVertices );
        for ( u32 i = 0; i < mesh->mNumVertices; i++ ) {
            SkinnedVertex & vertex = vertices[i];

            vertex.position.x = mesh->mVertices[i].x * scale;
            vertex.position.y = mesh->mVertices[i].y * scale;
            vertex.position.z = mesh->mVertices[i].z * scale;

            if ( mesh->HasNormals() ) {
                vertex.normal.x = mesh->mNormals[i].x;
                vertex.normal.y = mesh->mNormals[i].y;
                vertex.normal.z = mesh->mNormals[i].z;
            }

            if ( mesh->mTextureCoords[0] ) {
                vertex.texCoords.x = mesh->mTextureCoords[0][i].x;
                vertex.texCoords.y = mesh->mTextureCoords[0][i].y;
            }
        }

        for ( u32 i = 0; i < mesh->mNumFaces; i++ ) {
            aiFace & face = mesh->mFaces[i];
            for ( u32 j = 0; j < face.mNumIndices; j++ ) {
                indices.push_back( face.mIndices[j] );
            }
        }

        ExtractBoneData( mesh, vertices, boneInfoMap, boneCounter );

        SkinnedMesh result;
        result.Create( vertices, indices );
        return result;
    }

    static void ProcessSkinnedNode(
        aiNode * node,
        const aiScene * scene,
        std::vector<SkinnedMesh> & meshes,
        f32 scale,
        std::unordered_map<std::string, BoneInfo> & boneInfoMap,
        i32 & boneCounter
    ) {
        for ( u32 i = 0; i < node->mNumMeshes; i++ ) {
            aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back( ProcessSkinnedMesh( mesh, scene, scale, boneInfoMap, boneCounter ) );
        }

        for ( u32 i = 0; i < node->mNumChildren; i++ ) {
            ProcessSkinnedNode( node->mChildren[i], scene, meshes, scale, boneInfoMap, boneCounter );
        }
    }

    static BoneNode BuildBoneHierarchy( const aiNode * node ) {
        BoneNode boneNode;
        boneNode.name = node->mName.C_Str();
        boneNode.transformation = AiMat4ToGlm( node->mTransformation );

        boneNode.children.reserve( node->mNumChildren );
        for ( u32 i = 0; i < node->mNumChildren; i++ ) {
            boneNode.children.push_back( BuildBoneHierarchy( node->mChildren[i] ) );
        }

        return boneNode;
    }

    static void ExtractAnimations( const aiScene * scene, std::vector<AnimationClip> & animations ) {
        animations.reserve( scene->mNumAnimations );

        for ( u32 animIdx = 0; animIdx < scene->mNumAnimations; animIdx++ ) {
            const aiAnimation * anim = scene->mAnimations[animIdx];

            AnimationClip clip;
            clip.name = anim->mName.C_Str();
            clip.duration = static_cast<f32>(anim->mDuration);
            clip.ticksPerSecond = anim->mTicksPerSecond != 0.0
                ? static_cast<f32>(anim->mTicksPerSecond)
                : 25.0f;

            clip.channels.reserve( anim->mNumChannels );
            for ( u32 chIdx = 0; chIdx < anim->mNumChannels; chIdx++ ) {
                const aiNodeAnim * nodeAnim = anim->mChannels[chIdx];

                BoneAnimationChannel channel;
                channel.boneName = nodeAnim->mNodeName.C_Str();

                channel.positionKeys.reserve( nodeAnim->mNumPositionKeys );
                for ( u32 k = 0; k < nodeAnim->mNumPositionKeys; k++ ) {
                    PositionKeyframe key;
                    key.time = static_cast<f32>(nodeAnim->mPositionKeys[k].mTime);
                    key.position = AiVec3ToGlm( nodeAnim->mPositionKeys[k].mValue );
                    channel.positionKeys.push_back( key );
                }

                channel.rotationKeys.reserve( nodeAnim->mNumRotationKeys );
                for ( u32 k = 0; k < nodeAnim->mNumRotationKeys; k++ ) {
                    RotationKeyframe key;
                    key.time = static_cast<f32>(nodeAnim->mRotationKeys[k].mTime);
                    key.rotation = AiQuatToGlm( nodeAnim->mRotationKeys[k].mValue );
                    channel.rotationKeys.push_back( key );
                }

                channel.scaleKeys.reserve( nodeAnim->mNumScalingKeys );
                for ( u32 k = 0; k < nodeAnim->mNumScalingKeys; k++ ) {
                    ScaleKeyframe key;
                    key.time = static_cast<f32>(nodeAnim->mScalingKeys[k].mTime);
                    key.scale = AiVec3ToGlm( nodeAnim->mScalingKeys[k].mValue );
                    channel.scaleKeys.push_back( key );
                }

                clip.channels.push_back( std::move( channel ) );
            }

            animations.push_back( std::move( clip ) );
        }
    }

    void AnimatedModel::LoadFromFile( const char * filePath, f32 scale ) {
        Destroy();

        Assimp::Importer importer;

        const aiScene * scene = importer.ReadFile( filePath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_LimitBoneWeights
        );

        if ( !scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode ) {
            LOG_ERROR( "Assimp: %s", importer.GetErrorString() );
            return;
        }

        globalInverseTransform = glm::inverse( AiMat4ToGlm( scene->mRootNode->mTransformation ) );

        ProcessSkinnedNode( scene->mRootNode, scene, meshes, scale, boneInfoMap, boneCounter );
        rootNode = BuildBoneHierarchy( scene->mRootNode );
        ExtractAnimations( scene, animations );

        LOG_INFO( "Loaded animated model '%s' (%d meshes, %d bones, %d animations)",
            filePath, GetMeshCount(), GetBoneCount(), GetAnimationCount() );

        for ( i32 i = 0; i < GetAnimationCount(); i++ ) {
            const AnimationClip & clip = animations[i];
            LOG_INFO( "  Animation[%d]: '%s' (duration: %.1f, ticks/s: %.1f, channels: %d)",
                i, clip.name.c_str(), clip.duration, clip.ticksPerSecond,
                static_cast<i32>(clip.channels.size()) );
        }
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
    }

    void AnimatedModel::Draw() const {
        for ( const auto & mesh : meshes ) {
            mesh.Draw();
        }
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

        auto setFace = [&]( i32 face, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, Vec3 n ) {
            i32 base = face * 4;
            vertices[base + 0] = { p0, n, Vec2( 0.0f, 0.0f ) };
            vertices[base + 1] = { p1, n, Vec2( 1.0f, 0.0f ) };
            vertices[base + 2] = { p2, n, Vec2( 1.0f, 1.0f ) };
            vertices[base + 3] = { p3, n, Vec2( 0.0f, 1.0f ) };

            i32 idx = face * 6;
            indices[idx + 0] = base + 0;
            indices[idx + 1] = base + 1;
            indices[idx + 2] = base + 2;
            indices[idx + 3] = base + 0;
            indices[idx + 4] = base + 2;
            indices[idx + 5] = base + 3;
        };

        // +Z face (front)
        setFace( 0,
            c + Vec3( -h.x, -h.y, +h.z ),
            c + Vec3( +h.x, -h.y, +h.z ),
            c + Vec3( +h.x, +h.y, +h.z ),
            c + Vec3( -h.x, +h.y, +h.z ),
            Vec3( 0, 0, 1 )
        );
        // -Z face (back)
        setFace( 1,
            c + Vec3( +h.x, -h.y, -h.z ),
            c + Vec3( -h.x, -h.y, -h.z ),
            c + Vec3( -h.x, +h.y, -h.z ),
            c + Vec3( +h.x, +h.y, -h.z ),
            Vec3( 0, 0, -1 )
        );
        // +X face (right)
        setFace( 2,
            c + Vec3( +h.x, -h.y, +h.z ),
            c + Vec3( +h.x, -h.y, -h.z ),
            c + Vec3( +h.x, +h.y, -h.z ),
            c + Vec3( +h.x, +h.y, +h.z ),
            Vec3( 1, 0, 0 )
        );
        // -X face (left)
        setFace( 3,
            c + Vec3( -h.x, -h.y, -h.z ),
            c + Vec3( -h.x, -h.y, +h.z ),
            c + Vec3( -h.x, +h.y, +h.z ),
            c + Vec3( -h.x, +h.y, -h.z ),
            Vec3( -1, 0, 0 )
        );
        // +Y face (top)
        setFace( 4,
            c + Vec3( -h.x, +h.y, +h.z ),
            c + Vec3( +h.x, +h.y, +h.z ),
            c + Vec3( +h.x, +h.y, -h.z ),
            c + Vec3( -h.x, +h.y, -h.z ),
            Vec3( 0, 1, 0 )
        );
        // -Y face (bottom)
        setFace( 5,
            c + Vec3( -h.x, -h.y, -h.z ),
            c + Vec3( +h.x, -h.y, -h.z ),
            c + Vec3( +h.x, -h.y, +h.z ),
            c + Vec3( -h.x, -h.y, +h.z ),
            Vec3( 0, -1, 0 )
        );

        model.CreateFromMesh( vertices, indices );
    }

    void Brush::Serialize( Serializer & serializer ) {
        serializer( "center", center );
        serializer( "halfExtents", halfExtents );
    }

} // namespace atto
