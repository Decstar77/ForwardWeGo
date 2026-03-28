#pragma once

#include "engine/atto_assets.h"

namespace atto {

    class EditorAssetBrowser;

    class ImguiPropertySerializer : public Serializer {
    public:
        ImguiPropertySerializer();
        ~ImguiPropertySerializer();

        void SetAssetBrowser( EditorAssetBrowser * browser ) { assetBrowser = browser; }
        bool HasChanges() const { return changed; }

    protected:
        std::unique_ptr<Serializer> CreateSubSerializer() override;
        void SetObject( const char * key, Serializer * serializer ) override;
        std::unique_ptr<Serializer> GetObject( const char * key ) override;

        void BeginArray( const char * key, i32 & count ) override;
        void AppendArrayElement( const char * key, Serializer * serializer ) override;
        std::unique_ptr<Serializer> GetArrayElement( const char * key, i32 index ) override;

        void OpArrayPrimitive( const char * key, std::vector<u8> & value ) override;
        void OpArrayPrimitive( const char * key, std::vector<i32> & value ) override;
        void OpArrayPrimitive( const char * key, std::vector<u64> & value ) override;
        void OpArrayPrimitive( const char * key, std::vector<f32> & value ) override;
        void OpArrayPrimitive( const char * key, std::vector<bool> & value ) override;
        void OpArrayPrimitive( const char * key, std::vector<std::string> & value ) override;
        void OpArrayPrimitive( const char * key, std::vector<Vec2> & value ) override;
        void OpArrayPrimitive( const char * key, std::vector<Vec3> & value ) override;
        void OpArrayPrimitive( const char * key, std::vector<Vec4> & value ) override;

        void Op( const char * key, i8 & value ) override;
        void Op( const char * key, u8 & value ) override;
        void Op( const char * key, i32 & value ) override;
        void Op( const char * key, i64 & value ) override;
        void Op( const char * key, u32 & value ) override;
        void Op( const char * key, u64 & value ) override;
        void Op( const char * key, f64 & value ) override;
        void Op( const char * key, f32 & value ) override;
        void Op( const char * key, bool & value ) override;
        void Op( const char * key, std::string & value ) override;
        void Op( const char * key, Vec2 & value ) override;
        void Op( const char * key, Vec3 & value ) override;
        void Op( const char * key, Vec4 & value ) override;
        void Op( const char * key, Mat2 & value ) override;
        void Op( const char * key, Mat3 & value ) override;
        void Op( const char * key, Mat4 & value ) override;
        void Op( const char * key, SmallString & value ) override;
        void Op( const char * key, LargeString & value ) override;

        void OpStaticModel( const char * key, const StaticModel *& value ) override;
        void OpAnimatedModel( const char * key, const AnimatedModel *& value ) override;
        void OpFont( const char * key, const Font *& value ) override;

    private:
        EditorAssetBrowser * assetBrowser = nullptr;
        bool changed = false;
        i32 nextSubId = 0;
        bool pushedImguiId = false;
    };

}
