#pragma once

#include "atto_core.h"
#include "atto_math.h"

#include <type_traits>

namespace atto {

    class StaticModel;

    // Type trait to detect if a type has a Serialize(Serializer&) method
    template<typename T, typename = void>
    struct HasSerializeMethod : std::false_type {};

    template<typename T>
    struct HasSerializeMethod<T, std::void_t<decltype(std::declval<T>().Serialize( std::declval<class Serializer &>() ))>> : std::true_type {};

    template<typename T>
    struct IsVector : std::false_type {};
    
    template<typename T, typename Alloc>
    struct IsVector<std::vector<T, Alloc>> : std::true_type {};

    class Serializer {
    public:
        Serializer( bool isSaving ) : isSaving( isSaving ) {}
        virtual ~Serializer() {}

        Serializer( const Serializer & ) = delete;
        Serializer & operator=( const Serializer & ) = delete;
        Serializer( Serializer && ) = delete;
        Serializer & operator=( Serializer && ) = delete;

        inline bool IsSaving() const { return isSaving; }
        inline bool IsLoading() const { return !isSaving; }

        // Primitive types
        inline void operator() ( const char * key, u8 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, i8 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, i32 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, i64 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, u32 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, u64 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, f32 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, f64 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, bool & value ) { Op( key, value ); }
        inline void operator() ( const char * key, std::string & value ) { Op( key, value ); }
        inline void operator() ( const char * key, Vec2 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, Vec3 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, Vec4 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, Mat2 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, Mat3 & value ) { Op( key, value ); }
        inline void operator() ( const char * key, Mat4 & value ) { Op( key, value ); }

        // Special case: const StaticModel * (saves path string, loads via Renderer)
        void operator() ( const char * key, const StaticModel *& value );

        // Primitive vector types
        inline void operator() ( const char * key, std::vector<i32> & value ) { OpArrayPrimitive( key, value ); }
        inline void operator() ( const char * key, std::vector<u64> & value ) { OpArrayPrimitive( key, value ); }
        inline void operator() ( const char * key, std::vector<f32> & value ) { OpArrayPrimitive( key, value ); }
        inline void operator() ( const char * key, std::vector<bool> & value ) { OpArrayPrimitive( key, value ); }
        inline void operator() ( const char * key, std::vector<std::string> & value ) { OpArrayPrimitive( key, value ); }
        inline void operator() ( const char * key, std::vector<Vec2> & value ) { OpArrayPrimitive( key, value ); }
        inline void operator() ( const char * key, std::vector<Vec3> & value ) { OpArrayPrimitive( key, value ); }
        inline void operator() ( const char * key, std::vector<Vec4> & value ) { OpArrayPrimitive( key, value ); }

        // Complex types with Serialize() method
        template<typename T>
        inline typename std::enable_if<HasSerializeMethod<T>::value, void>::type
            operator() ( const char * key, T & value ) {
            if ( IsSaving() ) {
                std::unique_ptr<Serializer> sub = CreateSubSerializer();
                value.Serialize( *sub );
                SetObject( key, sub.get() );
            }
            else {
                std::unique_ptr<Serializer> sub = GetObject( key );
                if ( sub ) {
                    value.Serialize( *sub );
                }
            }
        }

        // Vector of primitives (types without Serialize method)
        template<typename T>
        inline typename std::enable_if<IsVector<T>::value, void>::type
            operator()( const char * key, T & value ) {
            using Elem = typename T::value_type;

            if ( IsSaving() ) {
                i32 count = static_cast<i32>(value.size());
                BeginArray( key, count );

                for ( i32 i = 0; i < count; ++i ) {
                    std::unique_ptr<Serializer> sub = CreateSubSerializer();
                    (*sub)("item", value[i]);
                    AppendArrayElement( key, sub.get() );
                }
            }
            else {
                i32 count = 0;
                BeginArray( key, count );
                value.resize( count );

                for ( i32 i = 0; i < count; ++i ) {
                    std::unique_ptr<Serializer> sub = GetArrayElement( key, i );
                    if ( sub ) {
                        (*sub)("item", value[i]);
                    }
                }
            }
        }

        // Polymorphic vector (std::vector<std::unique_ptr<T>>) with factory for loading.
        // Saving: calls T::Serialize on each element directly (no slicing).
        // Loading: factory receives a Serializer& and must return a fully deserialized std::unique_ptr<T>.
        template<typename T, typename Factory>
        void operator()( const char * key, std::vector<std::unique_ptr<T>> & list, Factory && factory ) {
            if ( IsSaving() ) {
                i32 count = static_cast<i32>(list.size());
                BeginArray( key, count );
                for ( i32 i = 0; i < count; ++i ) {
                    std::unique_ptr<Serializer> sub = CreateSubSerializer();
                    list[i]->Serialize( *sub );
                    AppendArrayElement( key, sub.get() );
                }
            }
            else {
                i32 count = 0;
                BeginArray( key, count );
                list.clear();
                for ( i32 i = 0; i < count; ++i ) {
                    std::unique_ptr<Serializer> sub = GetArrayElement( key, i );
                    if ( sub ) {
                        std::unique_ptr<T> obj = factory( *sub );
                        if ( obj ) {
                            list.push_back( std::move( obj ) );
                        }
                    }
                }
            }
        }

    protected:
        // Object serialization
        virtual std::unique_ptr<Serializer> CreateSubSerializer() = 0;
        virtual void SetObject( const char * key, Serializer * serializer ) = 0;
        virtual std::unique_ptr<Serializer> GetObject( const char * key ) = 0;

        // Array serialization
        virtual void BeginArray( const char * key, i32 & count ) = 0;
        virtual void AppendArrayElement( const char * key, Serializer * serializer ) = 0;
        virtual std::unique_ptr<Serializer> GetArrayElement( const char * key, i32 index ) = 0;

        // Primitive array operations
        virtual void OpArrayPrimitive( const char * key, std::vector<i32> & value ) = 0;
        virtual void OpArrayPrimitive( const char * key, std::vector<u64> & value ) = 0;
        virtual void OpArrayPrimitive( const char * key, std::vector<f32> & value ) = 0;
        virtual void OpArrayPrimitive( const char * key, std::vector<bool> & value ) = 0;
        virtual void OpArrayPrimitive( const char * key, std::vector<std::string> & value ) = 0;
        virtual void OpArrayPrimitive( const char * key, std::vector<Vec2> & value ) = 0;
        virtual void OpArrayPrimitive( const char * key, std::vector<Vec3> & value ) = 0;
        virtual void OpArrayPrimitive( const char * key, std::vector<Vec4> & value ) = 0;

        // Primitive operations
        virtual void Op( const char * key, i8 & value ) = 0;
        virtual void Op( const char * key, u8 & value ) = 0;
        virtual void Op( const char * key, i32 & value ) = 0;
        virtual void Op( const char * key, i64 & value ) = 0;
        virtual void Op( const char * key, u32 & value ) = 0;
        virtual void Op( const char * key, u64 & value ) = 0;
        virtual void Op( const char * key, f64 & value ) = 0;
        virtual void Op( const char * key, f32 & value ) = 0;
        virtual void Op( const char * key, bool & value ) = 0;
        virtual void Op( const char * key, std::string & value ) = 0;
        virtual void Op( const char * key, Vec2 & value ) = 0;
        virtual void Op( const char * key, Vec3 & value ) = 0;
        virtual void Op( const char * key, Vec4 & value ) = 0;
        virtual void Op( const char * key, Mat2 & value ) = 0;
        virtual void Op( const char * key, Mat3 & value ) = 0;
        virtual void Op( const char * key, Mat4 & value ) = 0;

    protected:
        bool isSaving;
    };

    class JsonSerializer : public Serializer {
    public:
        JsonSerializer( bool isSaving );
        virtual ~JsonSerializer();

        JsonSerializer( const JsonSerializer & ) = delete;
        JsonSerializer & operator=( const JsonSerializer & ) = delete;
        JsonSerializer( JsonSerializer && ) = delete;
        JsonSerializer & operator=( JsonSerializer && ) = delete;

        std::string ToString() const;
        void FromString( const std::string & json );

    protected:
        // Object serialization
        virtual std::unique_ptr<Serializer> CreateSubSerializer() override;
        virtual void SetObject( const char * key, Serializer * serializer ) override;
        virtual std::unique_ptr<Serializer> GetObject( const char * key ) override;

        // Array serialization
        virtual void BeginArray( const char * key, i32 & count ) override;
        virtual void AppendArrayElement( const char * key, Serializer * serializer ) override;
        virtual std::unique_ptr<Serializer> GetArrayElement( const char * key, i32 index ) override;

        // Primitive array operations
        virtual void OpArrayPrimitive( const char * key, std::vector<i32> & value ) override;
        virtual void OpArrayPrimitive( const char * key, std::vector<u64> & value ) override;
        virtual void OpArrayPrimitive( const char * key, std::vector<f32> & value ) override;
        virtual void OpArrayPrimitive( const char * key, std::vector<bool> & value ) override;
        virtual void OpArrayPrimitive( const char * key, std::vector<std::string> & value ) override;
        virtual void OpArrayPrimitive( const char * key, std::vector<Vec2> & value ) override;
        virtual void OpArrayPrimitive( const char * key, std::vector<Vec3> & value ) override;
        virtual void OpArrayPrimitive( const char * key, std::vector<Vec4> & value ) override;

        // Primitive operations
        virtual void Op( const char * key, i8 & value ) override;
        virtual void Op( const char * key, u8 & value ) override;
        virtual void Op( const char * key, i32 & value ) override;
        virtual void Op( const char * key, i64 & value ) override;
        virtual void Op( const char * key, u32 & value ) override;
        virtual void Op( const char * key, u64 & value ) override;
        virtual void Op( const char * key, f64 & value ) override;
        virtual void Op( const char * key, f32 & value ) override;
        virtual void Op( const char * key, bool & value ) override;
        virtual void Op( const char * key, std::string & value ) override;
        virtual void Op( const char * key, Vec2 & value ) override;
        virtual void Op( const char * key, Vec3 & value ) override;
        virtual void Op( const char * key, Vec4 & value ) override;
        virtual void Op( const char * key, Mat2 & value ) override;
        virtual void Op( const char * key, Mat3 & value ) override;
        virtual void Op( const char * key, Mat4 & value ) override;

    protected:
        struct JsonContainer;
        std::unique_ptr<JsonContainer> jsonContainer;
    };

    class AssetManager {
    public:
        AssetManager();
        ~AssetManager();

        bool            Initialize();
        void            Shutdown();

        void            WriteTextFile( const std::string & path, const std::string & content );
        std::string     ReadTextFile( const std::string & path );

        std::string     OpenFilePicker( const std::string & basePath );
        std::string     SaveFilePicker( const std::string & basePath, const std::string & extensions );  // "png;jpg;fbx"
    };

}
