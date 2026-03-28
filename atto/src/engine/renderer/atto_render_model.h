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
        Material &          GetMaterial() { return material; }
        const Material &    GetMaterial() const { return material; }
        void                SetMaterial( const Material & mat ) { material = mat; }

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
        void Draw( Shader * shader ) const;

        i32                 GetIndexCount() const { return indexCount; }
        const Material & GetMaterial() const { return material; }
        void                SetMaterial( const Material & mat ) { material = mat; }

    private:
        u32 vao = 0;
        u32 vbo = 0;
        u32 ebo = 0;
        i32 indexCount = 0;
        Material material = {};
    };

    class StaticModel {
    public:
        void LoadFromFile( const char * filePath, f32 scale = 1.0f );
        void Serialize( Serializer & serializer );
        void CreateFromMesh( const std::vector<Vertex> & vertices, const std::vector<u32> & indices );
        void Destroy();
        void Draw( Shader * shader ) const;

        bool IsLoaded() const { return !meshes.empty(); }
        i32 GetMeshCount() const { return static_cast<i32>(meshes.size()); }
        Mesh & GetMesh( i32 index ) { return meshes[index]; }
        const Mesh & GetMesh( i32 index ) const { return meshes[index]; }
        const AlignedBox & GetBounds() const { return bounds; }
        const AlignedBox & GetMeshBounds( i32 index ) const { return meshes[index].GetBounds(); }
        const LargeString & GetPath() const { return path; }

    private:
        void ComputeBounds();

        std::vector<Mesh> meshes;
        AlignedBox bounds = {};
        LargeString path;
    };

    class AnimatedModel {
    public:
        void LoadFromFile( const char * filePath, f32 scale = 1.0f );
        void Serialize( Serializer & serializer );
        void Destroy();
        void Draw( Shader * shader ) const;

        void DebugPrint() const;

        bool IsLoaded() const { return !meshes.empty(); }
        i32  GetMeshCount() const { return static_cast<i32>(meshes.size()); }
        i32  GetBoneCount() const { return boneCounter; }
        i32  GetAnimationCount() const { return static_cast<i32>(animations.size()); }
        const char * GetPath() const { return path.GetCStr(); }

        const std::unordered_map<std::string, BoneInfo> & GetBoneInfoMap() const { return boneInfoMap; }
        const BoneNode & GetRootNode() const { return rootNode; }
        const Mat4 & GetGlobalInverseTransform() const { return globalInverseTransform; }
        const std::vector<AnimationClip> & GetAnimations() const { return animations; }
        const i32                                           GetAnimationIndex( const char * name ) const;

    private:
        LargeString                                 path = {};
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

        void PlayAnimation( const AnimatedModel & model, const char * animationName, bool loop, f32 blendTime = 0.1f );
        void PlayAnimation( const AnimatedModel & model, i32 animationIndex, bool loop, f32 blendTime = 0.1f );
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

        static const BoneAnimationChannel * FindChannel( const AnimationClip * clip, const std::string & nodeName );

        Mat4                    finalBoneMatrices[MAX_BONES];
        const AnimatedModel *   model = nullptr;
        const AnimationClip *   currentClip = nullptr;
        f32                     currentTime = 0.0f;
        bool                    looping = true;

        // Crossfade blending state
        const AnimationClip *   prevClip = nullptr;
        f32                     prevTime = 0.0f;
        bool                    prevLooping = true;
        f32                     blendDuration = 0.0f;
        f32                     blendTimer = 0.0f;
    };

    enum class BrushType : i32 {
        Solid = 0,      // Drawn and has collision
        Trigger = 1,    // Not drawn and no collision
        Collision = 2,  // Not drawn but has collision
    };

    inline const char * BrushTypeNames[] = { "Solid", "Trigger", "Collision" };
    constexpr i32 BrushTypeCount = 3;

    class Brush {
    public:
        void ToStaticModel( StaticModel & model ) const;
        void Serialize( Serializer & serializer );

        bool IsPointInside( Vec3 point ) const;
        bool IsPointInside( Vec3 point, i32 hAxis, i32 vAxis ) const;

        bool IsDrawn() const { return type == BrushType::Solid; }
        bool HasCollision() const { return type != BrushType::Trigger; }

        BrushType type = BrushType::Solid;
        Vec3 center = Vec3( 0.0f );
        Vec3 halfExtents = Vec3( 0.5f );
        std::string texturePath;
        f32 textureScale = 1.0f;
    };

} // namespace atto
