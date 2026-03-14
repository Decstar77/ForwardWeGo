#pragma once

#include "../atto_core.h"
#include "../atto_math.h"
#include "../atto_assets.h"

#include <unordered_map>

namespace atto {

    static constexpr i32 MAX_BONES_PER_VERTEX = 4;
    static constexpr i32 MAX_BONES = 128;

    struct Vertex {
        Vec3 position;
        Vec3 normal;
        Vec2 texCoords;
    };

    struct SkinnedVertex {
        Vec3 position;
        Vec3 normal;
        Vec2 texCoords;
        i32  boneIDs[MAX_BONES_PER_VERTEX];
        f32  boneWeights[MAX_BONES_PER_VERTEX];

        SkinnedVertex() {
            for ( i32 i = 0; i < MAX_BONES_PER_VERTEX; i++ ) {
                boneIDs[i] = -1;
                boneWeights[i] = 0.0f;
            }
        }

        void AddBoneData( i32 boneID, f32 weight ) {
            for ( i32 i = 0; i < MAX_BONES_PER_VERTEX; i++ ) {
                if ( boneIDs[i] < 0 ) {
                    boneIDs[i] = boneID;
                    boneWeights[i] = weight;
                    return;
                }
            }
        }
    };

    struct BoneInfo {
        i32  id = -1;
        Mat4 offsetMatrix = Mat4( 1.0f );
    };

    struct PositionKeyframe {
        f32  time;
        Vec3 position;
    };

    struct RotationKeyframe {
        f32  time;
        Quat rotation;
    };

    struct ScaleKeyframe {
        f32  time;
        Vec3 scale;
    };

    struct BoneAnimationChannel {
        std::string                     boneName;
        std::vector<PositionKeyframe>   positionKeys;
        std::vector<RotationKeyframe>   rotationKeys;
        std::vector<ScaleKeyframe>      scaleKeys;
    };

    struct AnimationClip {
        std::string                         name;
        f32                                 duration = 0.0f;
        f32                                 ticksPerSecond = 25.0f;
        std::vector<BoneAnimationChannel>   channels;
    };

    struct BoneNode {
        std::string             name;
        Mat4                    transformation = Mat4( 1.0f );
        std::vector<BoneNode>   children;
    };

    class Mesh {
    public:
        void Create( const std::vector<Vertex> & vertices, const std::vector<u32> & indices );
        void Destroy();
        void Draw() const;

        i32 GetIndexCount() const { return indexCount; }

    private:
        u32 vao = 0;
        u32 vbo = 0;
        u32 ebo = 0;
        i32 indexCount = 0;
    };

    class SkinnedMesh {
    public:
        void Create( const std::vector<SkinnedVertex> & vertices, const std::vector<u32> & indices );
        void Destroy();
        void Draw() const;

        i32 GetIndexCount() const { return indexCount; }

    private:
        u32 vao = 0;
        u32 vbo = 0;
        u32 ebo = 0;
        i32 indexCount = 0;
    };

    class StaticModel {
    public:
        void LoadFromFile( const char * filePath, f32 scale = 1.0f );
        void CreateFromMesh( const std::vector<Vertex> & vertices, const std::vector<u32> & indices );
        void Destroy();
        void Draw() const;

        bool IsLoaded() const { return !meshes.empty(); }
        i32 GetMeshCount() const { return static_cast<i32>(meshes.size()); }

    private:
        std::vector<Mesh> meshes;
    };

    class AnimatedModel {
    public:
        void LoadFromFile( const char * filePath, f32 scale = 1.0f );
        void Destroy();
        void Draw() const;

        bool IsLoaded() const { return !meshes.empty(); }
        i32  GetMeshCount() const { return static_cast<i32>(meshes.size()); }
        i32  GetBoneCount() const { return boneCounter; }
        i32  GetAnimationCount() const { return static_cast<i32>(animations.size()); }

        const std::unordered_map<std::string, BoneInfo> & GetBoneInfoMap() const { return boneInfoMap; }
        const BoneNode &                    GetRootNode() const { return rootNode; }
        const Mat4 &                        GetGlobalInverseTransform() const { return globalInverseTransform; }
        const std::vector<AnimationClip> &  GetAnimations() const { return animations; }

    private:
        std::vector<SkinnedMesh>                    meshes;
        std::unordered_map<std::string, BoneInfo>   boneInfoMap;
        i32                                         boneCounter = 0;
        BoneNode                                    rootNode;
        Mat4                                        globalInverseTransform = Mat4( 1.0f );
        std::vector<AnimationClip>                  animations;
    };

    class Brush {
    public:
        void ToStaticModel( StaticModel & model ) const;
        void Serialize( Serializer & serializer );

        Vec3 center = Vec3( 0.0f );
        Vec3 halfExtents = Vec3( 0.5f );
    };

} // namespace atto
