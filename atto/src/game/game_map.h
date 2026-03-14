#include "engine/atto_engine.h"
#include "engine/atto_assets.h"

namespace atto {

    class GameMap {
    public:
        GameMap();
        ~GameMap();

        void Initialize();
        void Update( f32 dt );
        void Render( Renderer & renderer, f32 dt, bool lit );

        void Serialize( Serializer & serializer );

    private:
        std::vector<StaticModel> staticModels;

        // Test stuff
        StaticModel model;
        Texture texture;

        // This is editor stuff
        std::vector<Brush> brushes;
    };
}
