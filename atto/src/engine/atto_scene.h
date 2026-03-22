#pragma once
#include "atto_core.h"

namespace atto {
    class Renderer;
    class SceneInterface;

    class SceneRegistry {
    public:
        template<typename T>
        static void Register() {
            Entry entry;
            entry.name = T::GetSceneNameStatic();
            entry.create = []() -> std::unique_ptr<SceneInterface> { return std::make_unique<T>(); };
            GetEntries().push_back( entry );
        }

        static std::unique_ptr<SceneInterface> CreateNew( const char * name ) {
            for ( const auto & e : GetEntries() ) {
                if ( strcmp( e.name, name ) == 0 ) {
                    return e.create();
                }
            }
            return nullptr;
        }

    private:
        struct Entry {
            const char * name;
            std::unique_ptr<SceneInterface>( *create )();
        };

        static std::vector<Entry> & GetEntries() {
            static std::vector<Entry> entries;
            return entries;
        }
    };

    class SceneInterface {
    public:
        virtual ~SceneInterface() = default;

        virtual void OnStart( const char * args ) {}
        virtual void OnUpdate( f32 deltaTime ) {}
        virtual void OnRender( Renderer & renderer ) {}
        virtual void OnShutdown() {}

        // Optional callbacks
        virtual void OnResize( i32 width, i32 height ) {}

        virtual const char * GetSceneName() = 0;
    };

    void RegisterAllScenes();
}
