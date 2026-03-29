#include "atto_assets.h"
#include "atto_log.h"
#include "atto_ui.h"
#include "renderer/atto_render_model_helpers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/std_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype/stb_truetype.h"

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis/stb_vorbis.c>

#include <audio/AudioFile.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/anim.h>

#include <functional>

#include "atto_engine.h"

namespace atto {

    // =========================================================================
    // Texture raw loading (stb_image)
    // =========================================================================

    bool AssetManager::LoadTextureDataRaw( const char * filePath, Serializer & serializer ) {
        int w, h, channels;
        stbi_uc * data = stbi_load( filePath, &w, &h, &channels, STBI_rgb_alpha );
        if ( !data ) {
            LOG_ERROR( "Failed to load texture image '%s'", filePath );
            return false;
        }

        const size_t dataSize = w * h * 4; // 4 bytes per pixel (RGBA)
        std::vector<u8> textureData( data, data + dataSize );

        stbi_image_free( data );

        // Must match the order in Texture::Serialize
        LargeString path = LargeString::FromLiteral( filePath );
        serializer( "Path", path );
        serializer( "Width", w );
        serializer( "Height", h );
        serializer( "Channels", channels );
        serializer( "Data", textureData );

        return true;
    }

    // =========================================================================
    // Assimp helpers
    // =========================================================================

    static Mat4 AiMat4ToGlm(const aiMatrix4x4 & m ) {
        return Mat4(
            m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4
        );
    }

    static Vec3 AiVec3ToGlm( const aiVector3D & v ) {
        return Vec3( v.x, v.y, v.z );
    }

    static Quat AiQuatToGlm( const aiQuaternion & q ) {
        return Quat( q.w, q.x, q.y, q.z );
    }

    // =========================================================================
    // Material data (for serialization — stores paths instead of loaded textures)
    // =========================================================================

    static MaterialData ExtractMaterialData( aiMesh * mesh, const aiScene * scene ) {
        MaterialData mat;

        if ( mesh->mMaterialIndex >= scene->mNumMaterials ) {
            return mat;
        }

        const aiMaterial * aiMat = scene->mMaterials[mesh->mMaterialIndex];

        aiColor4D baseColor;
        if ( aiMat->Get( AI_MATKEY_BASE_COLOR, baseColor ) == AI_SUCCESS ) {
            mat.albedo = Vec3( baseColor.r, baseColor.g, baseColor.b );
        }
        else {
            aiColor4D diffuse;
            if ( aiMat->Get( AI_MATKEY_COLOR_DIFFUSE, diffuse ) == AI_SUCCESS ) {
                mat.albedo = Vec3( diffuse.r, diffuse.g, diffuse.b );
            }
        }

        ai_real metallic = 0.0f;
        if ( aiMat->Get( AI_MATKEY_METALLIC_FACTOR, metallic ) == AI_SUCCESS ) {
            mat.metalic = static_cast<f32>( metallic );
        }

        ai_real roughness = 0.5f;
        if ( aiMat->Get( AI_MATKEY_ROUGHNESS_FACTOR, roughness ) == AI_SUCCESS ) {
            mat.roughness = static_cast<f32>( roughness );
        }

        aiString texPath;
        if ( aiMat->GetTexture( aiTextureType_DIFFUSE, 0, &texPath ) ) {
            mat.albedoTexturePath = texPath.C_Str();
        }
        else if ( aiMat->GetTexture( aiTextureType_BASE_COLOR, 0, &texPath ) ) {
            mat.albedoTexturePath = texPath.C_Str();
        }

        if ( aiMat->GetTexture( aiTextureType_SPECULAR, 0, &texPath ) ) {
            mat.metalicTexturePath = texPath.C_Str();
        }

        return mat;
    }

    static void ExtractBoneData( aiMesh * mesh, std::vector<AnimationVertex> & vertices, std::unordered_map<std::string, BoneInfo> & boneInfoMap, i32 & boneCounter ) {
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
            clip.duration = static_cast<f32>( anim->mDuration );
            clip.ticksPerSecond = anim->mTicksPerSecond != 0.0
                ? static_cast<f32>( anim->mTicksPerSecond )
                : 25.0f;

            clip.channels.reserve( anim->mNumChannels );
            for ( u32 chIdx = 0; chIdx < anim->mNumChannels; chIdx++ ) {
                const aiNodeAnim * nodeAnim = anim->mChannels[chIdx];

                BoneAnimationChannel channel;
                channel.boneName = nodeAnim->mNodeName.C_Str();

                channel.positionKeys.reserve( nodeAnim->mNumPositionKeys );
                for ( u32 k = 0; k < nodeAnim->mNumPositionKeys; k++ ) {
                    PositionKeyframe key;
                    key.time = static_cast<f32>( nodeAnim->mPositionKeys[k].mTime );
                    key.position = AiVec3ToGlm( nodeAnim->mPositionKeys[k].mValue );
                    channel.positionKeys.push_back( key );
                }

                channel.rotationKeys.reserve( nodeAnim->mNumRotationKeys );
                for ( u32 k = 0; k < nodeAnim->mNumRotationKeys; k++ ) {
                    RotationKeyframe key;
                    key.time = static_cast<f32>( nodeAnim->mRotationKeys[k].mTime );
                    key.rotation = AiQuatToGlm( nodeAnim->mRotationKeys[k].mValue );
                    channel.rotationKeys.push_back( key );
                }

                channel.scaleKeys.reserve( nodeAnim->mNumScalingKeys );
                for ( u32 k = 0; k < nodeAnim->mNumScalingKeys; k++ ) {
                    ScaleKeyframe key;
                    key.time = static_cast<f32>( nodeAnim->mScalingKeys[k].mTime );
                    key.scale = AiVec3ToGlm( nodeAnim->mScalingKeys[k].mValue );
                    channel.scaleKeys.push_back( key );
                }

                clip.channels.push_back( std::move( channel ) );
            }

            animations.push_back( std::move( clip ) );
        }
    }

    // =========================================================================
    // StaticModel raw loading (Assimp)
    // =========================================================================

    bool AssetManager::LoadStaticModelDataRaw( const char * filePath, Serializer & serializer ) {
        Assimp::Importer importer;

        const aiScene * scene = importer.ReadFile( filePath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace
        );

        if ( !scene || ( scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ) || !scene->mRootNode ) {
            LOG_ERROR( "Assimp: %s", importer.GetErrorString() );
            return false;
        }

        LargeString path = LargeString::FromLiteral( filePath );
        serializer( "Path", path );

        // Collect all meshes from node tree
        struct MeshDataEntry {
            std::vector<Vertex> vertices;
            std::vector<u32>    indices;
            MaterialData        material;
        };

        std::vector<MeshDataEntry> meshEntries;

        // Lambda to recursively collect mesh data
        std::function<void( aiNode * )> collectMeshes = [&]( aiNode * node ) {
            for ( u32 i = 0; i < node->mNumMeshes; i++ ) {
                aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];
                MeshDataEntry entry;

                entry.vertices.reserve( mesh->mNumVertices );
                for ( u32 v = 0; v < mesh->mNumVertices; v++ ) {
                    Vertex vertex = {};
                    vertex.position.x = mesh->mVertices[v].x;
                    vertex.position.y = mesh->mVertices[v].y;
                    vertex.position.z = mesh->mVertices[v].z;

                    if ( mesh->HasNormals() ) {
                        vertex.normal.x = mesh->mNormals[v].x;
                        vertex.normal.y = mesh->mNormals[v].y;
                        vertex.normal.z = mesh->mNormals[v].z;
                    }

                    if ( mesh->mTextureCoords[0] ) {
                        vertex.texCoords.x = mesh->mTextureCoords[0][v].x;
                        vertex.texCoords.y = mesh->mTextureCoords[0][v].y;
                    }

                    entry.vertices.push_back( vertex );
                }

                for ( u32 f = 0; f < mesh->mNumFaces; f++ ) {
                    aiFace & face = mesh->mFaces[f];
                    for ( u32 j = 0; j < face.mNumIndices; j++ ) {
                        entry.indices.push_back( face.mIndices[j] );
                    }
                }

                entry.material = ExtractMaterialData( mesh, scene );
                meshEntries.push_back( std::move( entry ) );
            }
            for ( u32 i = 0; i < node->mNumChildren; i++ ) {
                collectMeshes( node->mChildren[i] );
            }
        };

        collectMeshes( scene->mRootNode );

        i32 meshCount = static_cast<i32>( meshEntries.size() );
        serializer( "MeshCount", meshCount );

        for ( const MeshDataEntry & entry : meshEntries ) {
            std::vector<u8> vertexBytes( entry.vertices.size() * sizeof( Vertex ) );
            std::memcpy( vertexBytes.data(), entry.vertices.data(), vertexBytes.size() );

            std::vector<u8> indexBytes( entry.indices.size() * sizeof( u32 ) );
            std::memcpy( indexBytes.data(), entry.indices.data(), indexBytes.size() );

            serializer( "Vertices", vertexBytes );
            serializer( "Indices", indexBytes );

            MaterialData mat = entry.material;
            SerializeMaterialData( serializer, mat );
        }

        LOG_INFO( "Loaded model data '%s' (%d meshes)", filePath, meshCount );
        return true;
    }

    // =========================================================================
    // AnimatedModel raw loading (Assimp)
    // =========================================================================

    bool AssetManager::LoadAnimatedModelDataRaw( const char * filePath, Serializer & serializer ) {
        Assimp::Importer importer;

        const aiScene * scene = importer.ReadFile( filePath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_LimitBoneWeights
        );

        if ( !scene || ( scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ) || !scene->mRootNode ) {
            LOG_ERROR( "Assimp: %s", importer.GetErrorString() );
            return false;
        }

        LargeString pathStr = LargeString::FromLiteral( filePath );
        serializer( "Path", pathStr );

        // Collect skinned meshes
        struct SkinnedMeshEntry {
            std::vector<AnimationVertex> vertices;
            std::vector<u32>             indices;
            MaterialData                 material;
        };

        std::vector<SkinnedMeshEntry> meshEntries;
        std::unordered_map<std::string, BoneInfo> boneMap;
        i32 boneCount = 0;

        std::function<void( aiNode * )> collectMeshes = [&]( aiNode * node ) {
            for ( u32 i = 0; i < node->mNumMeshes; i++ ) {
                aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];
                SkinnedMeshEntry entry;

                entry.vertices.resize( mesh->mNumVertices );
                for ( u32 v = 0; v < mesh->mNumVertices; v++ ) {
                    AnimationVertex & vertex = entry.vertices[v];
                    vertex.position.x = mesh->mVertices[v].x;
                    vertex.position.y = mesh->mVertices[v].y;
                    vertex.position.z = mesh->mVertices[v].z;

                    if ( mesh->HasNormals() ) {
                        vertex.normal.x = mesh->mNormals[v].x;
                        vertex.normal.y = mesh->mNormals[v].y;
                        vertex.normal.z = mesh->mNormals[v].z;
                    }

                    if ( mesh->mTextureCoords[0] ) {
                        vertex.texCoords.x = mesh->mTextureCoords[0][v].x;
                        vertex.texCoords.y = mesh->mTextureCoords[0][v].y;
                    }
                }

                for ( u32 f = 0; f < mesh->mNumFaces; f++ ) {
                    aiFace & face = mesh->mFaces[f];
                    for ( u32 j = 0; j < face.mNumIndices; j++ ) {
                        entry.indices.push_back( face.mIndices[j] );
                    }
                }

                // Extract bone data
                ExtractBoneData( mesh, entry.vertices, boneMap, boneCount );

                // Normalize weights
                for ( AnimationVertex & v : entry.vertices ) {
                    f32 totalWeight = 0.0f;
                    for ( i32 b = 0; b < MAX_BONES_PER_VERTEX; b++ ) {
                        if ( v.boneIDs[b] >= 0 ) {
                            totalWeight += v.boneWeights[b];
                        }
                    }
                    if ( totalWeight > 0.0f && Abs( totalWeight - 1.0f ) > 1e-6f ) {
                        f32 invTotal = 1.0f / totalWeight;
                        for ( i32 b = 0; b < MAX_BONES_PER_VERTEX; b++ ) {
                            v.boneWeights[b] *= invTotal;
                        }
                    }
                }

                entry.material = ExtractMaterialData( mesh, scene );
                meshEntries.push_back( std::move( entry ) );
            }
            for ( u32 i = 0; i < node->mNumChildren; i++ ) {
                collectMeshes( node->mChildren[i] );
            }
        };

        collectMeshes( scene->mRootNode );

        // Write meshes
        i32 meshCount = static_cast<i32>( meshEntries.size() );
        serializer( "MeshCount", meshCount );

        for ( const SkinnedMeshEntry & entry : meshEntries ) {
            std::vector<u8> vertexBytes( entry.vertices.size() * sizeof( AnimationVertex ) );
            std::memcpy( vertexBytes.data(), entry.vertices.data(), vertexBytes.size() );

            std::vector<u8> indexBytes( entry.indices.size() * sizeof( u32 ) );
            std::memcpy( indexBytes.data(), entry.indices.data(), indexBytes.size() );

            serializer( "Vertices", vertexBytes );
            serializer( "Indices", indexBytes );

            MaterialData mat = entry.material;
            SerializeMaterialData( serializer, mat );
        }

        // Write bone info map
        i32 boneMapCount = static_cast<i32>( boneMap.size() );
        serializer( "BoneMapCount", boneMapCount );
        for ( const auto & pair : boneMap ) {
            std::string boneName = pair.first;
            BoneInfo info = pair.second;
            serializer( "BoneName", boneName );
            serializer( "BoneId", info.id );
            serializer( "BoneOffset", info.offsetMatrix );
        }

        serializer( "BoneCounter", boneCount );

        Mat4 globalInverse = glm::inverse( AiMat4ToGlm( scene->mRootNode->mTransformation ) );
        serializer( "GlobalInverseTransform", globalInverse );

        // Write bone hierarchy
        BoneNode rootBoneNode = BuildBoneHierarchy( scene->mRootNode );
        SerializeBoneNode( serializer, rootBoneNode );

        // Write animations
        std::vector<AnimationClip> clips;
        ExtractAnimations( scene, clips );
        i32 animCount = static_cast<i32>( clips.size() );
        serializer( "AnimCount", animCount );
        for ( i32 i = 0; i < animCount; i++ ) {
            SerializeAnimationClip( serializer, clips[i] );
        }

        LOG_INFO( "Loaded animated model data '%s' (%d meshes, %d bones, %d animations)",
            filePath, meshCount, boneCount, animCount );

        return true;
    }

    // =========================================================================
    // Font raw loading (stb_truetype)
    // =========================================================================

    bool AssetManager::LoadFontDataRaw( const char * filePath, f32 inFontSize, Serializer & serializer ) {
        // Read TTF file
        FILE * f = fopen( filePath, "rb" );
        if ( !f ) {
            LOG_ERROR( "LoadFontData — could not open %s", filePath );
            return false;
        }

        fseek( f, 0, SEEK_END );
        const long fileSize = ftell( f );
        fseek( f, 0, SEEK_SET );

        std::vector<u8> ttfBuffer( fileSize );
        fread( ttfBuffer.data(), 1, fileSize, f );
        fclose( f );

        // Bake the font atlas
        std::vector<u8> atlasBitmap( FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT );
        stbtt_bakedchar bakedCharData[FONT_CHAR_COUNT];

        const i32 result = stbtt_BakeFontBitmap(
            ttfBuffer.data(), 0,
            inFontSize,
            atlasBitmap.data(), FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT,
            FONT_FIRST_CHAR, FONT_CHAR_COUNT,
            bakedCharData
        );

        if ( result <= 0 ) {
            LOG_ERROR( "LoadFontData — stbtt_BakeFontBitmap failed for %s (returned %d)", filePath, result );
        }

        // Write to serializer (must match Font::Serialize order)
        LargeString pathStr = LargeString::FromLiteral( filePath );
        serializer( "Path", pathStr );
        serializer( "FontSize", inFontSize );

        std::vector<u8> charBytes( sizeof( bakedCharData ) );
        std::memcpy( charBytes.data(), bakedCharData, sizeof( bakedCharData ) );
        serializer( "CharData", charBytes );

        serializer( "AtlasBitmap", atlasBitmap );

        return true;
    }

    // =========================================================================
    // WAV raw loading (AudioFile library)
    // =========================================================================

    bool AssetManager::LoadWAV( const char * path, bool mono, Serializer & serializer ) {
        AudioFile<float> audioFile;
        if ( !audioFile.load( path ) ) {
            LOG_ERROR( "Failed to load WAV file: %s", path );
            return false;
        }

        i32 channels = audioFile.getNumChannels();
        i32 sampleRate = static_cast<i32>( audioFile.getSampleRate() );
        i32 numSamples = audioFile.getNumSamplesPerChannel();

        std::vector<i16> samples;

        if ( mono && channels == 2 ) {
            // Downmix stereo to mono
            samples.resize( numSamples );
            for ( i32 i = 0; i < numSamples; ++i ) {
                f32 mixed = ( audioFile.samples[0][i] + audioFile.samples[1][i] ) * 0.5f;
                mixed = mixed < -1.0f ? -1.0f : ( mixed > 1.0f ? 1.0f : mixed );
                samples[i] = static_cast<i16>( mixed * 32767.0f );
            }
            channels = 1;
        }
        else {
            // Interleave channels and convert to i16
            i32 totalSamples = numSamples * channels;
            samples.resize( totalSamples );
            for ( i32 i = 0; i < numSamples; ++i ) {
                for ( i32 ch = 0; ch < channels; ++ch ) {
                    f32 sample = audioFile.samples[ch][i];
                    sample = sample < -1.0f ? -1.0f : ( sample > 1.0f ? 1.0f : sample );
                    samples[i * channels + ch] = static_cast<i16>( sample * 32767.0f );
                }
            }
        }

        // Write to serializer (must match the read order in GetOrLoadSoundHandle)
        std::string name = mono ? std::string( path ) + ":mono" : std::string( path );
        serializer( "Name", name );
        serializer( "Channels", channels );
        serializer( "SampleRate", sampleRate );

        i32 sampleCount = static_cast<i32>( samples.size() );
        serializer( "SampleCount", sampleCount );

        std::vector<u8> sampleBytes( sampleCount * sizeof( i16 ) );
        std::memcpy( sampleBytes.data(), samples.data(), sampleBytes.size() );
        serializer( "Samples", sampleBytes );

        return true;
    }

    // =========================================================================
    // OGG raw loading (stb_vorbis)
    // =========================================================================

    bool AssetManager::LoadOGG( const char * path, bool mono, Serializer & serializer ) {
        int channels = 0;
        int sampleRate = 0;
        short * rawSamples = nullptr;

        int numSamples = stb_vorbis_decode_filename( path, &channels, &sampleRate, &rawSamples );
        if ( numSamples <= 0 ) {
            LOG_ERROR( "Failed to load OGG file: %s", path );
            return false;
        }

        std::vector<i16> samples;
        i32 finalChannels = channels;

        if ( mono && channels == 2 ) {
            // Downmix stereo to mono
            samples.resize( numSamples );
            for ( i32 i = 0; i < numSamples; ++i ) {
                i32 mixed = ( static_cast<i32>( rawSamples[i * 2] ) + static_cast<i32>( rawSamples[i * 2 + 1] ) ) / 2;
                samples[i] = static_cast<i16>( mixed );
            }
            finalChannels = 1;
        }
        else {
            i32 totalSamples = numSamples * channels;
            samples.assign( rawSamples, rawSamples + totalSamples );
        }

        free( rawSamples );

        // Write to serializer (must match the read order in GetOrLoadSoundHandle)
        std::string name = mono ? std::string( path ) + ":mono" : std::string( path );
        serializer( "Name", name );
        serializer( "Channels", finalChannels );
        serializer( "SampleRate", sampleRate );

        i32 sampleCount = static_cast<i32>( samples.size() );
        serializer( "SampleCount", sampleCount );

        std::vector<u8> sampleBytes( sampleCount * sizeof( i16 ) );
        std::memcpy( sampleBytes.data(), samples.data(), sampleBytes.size() );
        serializer( "Samples", sampleBytes );

        return true;
    }


    // =========================================================================
    // Sound loading (auto-detect format)
    // =========================================================================

    // Case-insensitive string comparison (portable)
    static bool StrEqualsIgnoreCase( const char * a, const char * b ) {
        while ( *a && *b ) {
            char ca = *a;
            char cb = *b;
            if ( ca >= 'A' && ca <= 'Z' ) ca += 'a' - 'A';
            if ( cb >= 'A' && cb <= 'Z' ) cb += 'a' - 'A';
            if ( ca != cb ) return false;
            ++a;
            ++b;
        }
        return *a == *b;
    }

    bool AssetManager::LoadSoundRaw( const char * path, bool mono, Serializer & serializer ) {
        const char * ext = strrchr( path, '.' );
        if ( ext ) {
            if ( StrEqualsIgnoreCase( ext, ".ogg" ) ) {
                return LoadOGG( path, mono, serializer );
            }
            else if ( StrEqualsIgnoreCase( ext, ".wav" ) ) {
                return LoadWAV( path, mono, serializer );
            }
        }
        return LoadWAV( path, mono, serializer );
    }
} // namespace atto
