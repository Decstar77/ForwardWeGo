#pragma once
#include "atto_core.h"

namespace atto {
    class Renderer;
    class SceneInterface;

    class SceneRegistryEntry {
    public:
        virtual std::unique_ptr<SceneInterface> CreateNewScene() const = 0;
        virtual const char *                    GetSceneName() const = 0;
    };

    class SceneRegistry {
    public:
        inline static void RegisterScene( const SceneRegistryEntry * scene ) { scenes.push_back( scene ); }
        inline static std::unique_ptr<SceneInterface> CreateNew( const char * name ) {
            for ( const auto & s : scenes ) {
                if ( strcmp( s->GetSceneName(), name ) == 0 ) {
                    return s->CreateNewScene();
                }
            }
            return nullptr;
        }
        
    private:
        inline static std::vector< const SceneRegistryEntry * > scenes;
    };

    template<typename _type_>
    class SceneRegistryTyped : public SceneRegistryEntry {
    public:
        SceneRegistryTyped() { 
            SceneRegistry::RegisterScene( this ); 
        }

        virtual std::unique_ptr<SceneInterface> CreateNewScene() const override { return std::make_unique<_type_>(); }
        virtual const char *                    GetSceneName() const { return _type_::GetSceneNameStatic(); }
    };

    class SceneInterface {
    public:
        virtual ~SceneInterface() = default;

        virtual void OnStart() {}
        virtual void OnUpdate( f32 deltaTime ) {}
        virtual void OnRender( Renderer & renderer ) {}
        virtual void OnShutdown() {}

        // Optional callbacks
        virtual void OnResize( i32 width, i32 height ) {}

        virtual const char * GetSceneName() = 0;
    };

    template<typename _type_>
    class Scene : public SceneInterface {
    private:
        inline static SceneRegistryTyped<_type_> registryInstance;
    };
}