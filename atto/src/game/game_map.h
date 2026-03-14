#include "engine/atto_engine.h"
#include "engine/atto_assets.h"

namespace atto {

    class GameMap {
    public:
        GameMap();
        ~GameMap();

        void Initialize();
        void Update( f32 dt );
        void Render( Renderer & renderer, f32 dt, bool lit, i32 selectedBrush = -1 );

        void Serialize( Serializer & serializer );

        i32 AddBrush();
        void RemoveBrush( i32 index );
        void RebuildBrushModel( i32 index );
        void RebuildAllBrushModels();

        Brush &       GetBrush( i32 index ) { return brushes[index]; }
        const Brush & GetBrush( i32 index ) const { return brushes[index]; }
        i32           GetBrushCount() const { return static_cast<i32>( brushes.size() ); }

    private:
        std::vector<StaticModel> staticModels;

        StaticModel model;
        Texture texture;

        std::vector<Brush> brushes;
        std::vector<StaticModel> brushModels;
    };
}
