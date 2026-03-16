#pragma once

#include "../atto_core.h"
#include "../atto_math.h"
#include "../atto_assets.h"
#include "../atto_shapes_3D.h"

#include "atto_render_material.h"

#include <unordered_map>

namespace atto {

    static constexpr i32 MAX_BONES_PER_VERTEX = 4;
    static constexpr i32 MAX_BONES = 128;

    struct Vertex {
        Vec3 position;
        Vec3 normal;
        Vec2 texCoords;
    };

    struct AnimationVertex {
        AnimationVertex();
        void AddBoneData( i32 boneID, f32 weight );

        Vec3 position;
        Vec3 normal;
        Vec2 texCoords;
        i32  boneIDs[MAX_BONES_PER_VERTEX];
        f32  boneWeights[MAX_BONES_PER_VERTEX];
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
        void Draw( Shader * shader ) const;

        i32                 GetIndexCount() const { return indexCount; }
        const AlignedBox &  GetBounds() const { return bounds; }

    private:
        u32 vao = 0;
        u32 vbo = 0;
        u32 ebo = 0;
        i32 indexCount = 0;
        AlignedBox bounds = {};
        Material material = {};
    };

    class AnimatedMesh {
    public:
        void Create( const std::vector<AnimationVertex> & vertices, const std::vector<u32> & indices );
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
        void Draw( Shader * shader ) const;

        bool IsLoaded() const { return !meshes.empty(); }
        i32 GetMeshCount() const { return static_cast<i32>(meshes.size()); }
        const AlignedBox & GetBounds() const { return bounds; }
        const AlignedBox & GetMeshBounds( i32 index ) const { return meshes[index].GetBounds(); }

    private:
        void ComputeBounds();

        std::vector<Mesh> meshes;
        AlignedBox bounds = {};
    };

    class AnimatedModel {
    public:
        void LoadFromFile( const char * filePath, f32 scale = 1.0f );
        void Destroy();
        void Draw() const;

        void DebugPrint() const;

        bool IsLoaded() const { return !meshes.empty(); }
        i32  GetMeshCount() const { return static_cast<i32>(meshes.size()); }
        i32  GetBoneCount() const { return boneCounter; }
        i32  GetAnimationCount() const { return static_cast<i32>(animations.size()); }

        const std::unordered_map<std::string, BoneInfo> & GetBoneInfoMap() const { return boneInfoMap; }
        const BoneNode & GetRootNode() const { return rootNode; }
        const Mat4 & GetGlobalInverseTransform() const { return globalInverseTransform; }
        const std::vector<AnimationClip> & GetAnimations() const { return animations; }
        const i32                                           GetAnimationIndex( const char * name ) const;

    private:
        std::vector<AnimatedMesh>                   meshes;
        std::unordered_map<std::string, BoneInfo>   boneInfoMap;
        i32                                         boneCounter = 0;
        BoneNode                                    rootNode;
        Mat4                                        globalInverseTransform = Mat4( 1.0f );
        std::vector<AnimationClip>                  animations;
    };

    class Animator {
    public:
        Animator();

        void PlayAnimation( const AnimatedModel & model, const char * animationName, bool loop );
        void PlayAnimation( const AnimatedModel & model, i32 animationIndex, bool loop );
        void Update( f32 dt );

        const Mat4 * GetFinalBoneMatrices() const { return finalBoneMatrices; }
        bool IsPlaying() const { return currentClip != nullptr; }
        bool IsFinished() const { return currentClip ? currentTime >= currentClip->duration : true; }
        f32  GetCurrentTime() const { return currentTime; }
        f32  GetDuration() const { return currentClip ? currentClip->duration / currentClip->ticksPerSecond : 0.0f; }
        f32  GetPercentComplete() const { return currentClip ? currentTime / currentClip->duration : 0.0f; }
        const AnimationClip * GetCurrentAnimation() const { return currentClip; }

    private:
        void CalculateBoneTransform( const BoneNode & node, const Mat4 & parentTransform );

        static Vec3 InterpolatePosition( f32 animTime, const BoneAnimationChannel & channel );
        static Quat InterpolateRotation( f32 animTime, const BoneAnimationChannel & channel );
        static Vec3 InterpolateScale( f32 animTime, const BoneAnimationChannel & channel );

        const BoneAnimationChannel * FindChannel( const std::string & nodeName ) const;

        Mat4                    finalBoneMatrices[MAX_BONES];
        const AnimatedModel * model = nullptr;
        const AnimationClip * currentClip = nullptr;
        f32                     currentTime = 0.0f;
        bool                    looping = true;
    };

    class Brush {
    public:
        void ToStaticModel( StaticModel & model ) const;
        void Serialize( Serializer & serializer );

        bool IsPointInside( Vec3 point ) const;
        bool IsPointInside( Vec3 point, i32 hAxis, i32 vAxis ) const;

        Vec3 center = Vec3( 0.0f );
        Vec3 halfExtents = Vec3( 0.5f );
    };

} // namespace atto
