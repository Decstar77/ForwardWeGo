#include "engine/atto_engine.h"
#include "engine/atto_assets.h"

namespace atto {

    struct PlayerStart {
        Vec3 spawnPos;
        Mat3 spawnOri;

        void Serialize( Serializer & serializer );
    };

    class GameMap {
    public:
        GameMap();
        ~GameMap();

        void Initialize();

        void StartMap();

        void Update( f32 dt );
        void Render( Renderer & renderer, f32 dt, bool lit, i32 selectedBrush = -1 );

        void Serialize( Serializer & serializer );


        // =========== Brushes ===========
        i32             AddBrush();
        void            RemoveBrush( i32 index );
        void            RebuildBrushModel( i32 index );
        void            RebuildBrushCollision( i32 index );
        void            RebuildAllBrushModels();
        void            RebuildAllBrushCollision();
        Brush &         GetBrush( i32 index ) { return brushes[index]; }
        const Brush &   GetBrush( i32 index ) const { return brushes[index]; }
        i32             GetBrushCount() const { return static_cast<i32>( brushes.size() ); }

    private:
        std::vector<StaticModel> staticModels;

        PlayerStart playerStart;

        StaticModel model;
        Texture texture;

        std::vector<Brush> brushes;
        std::vector<StaticModel> brushModels;

        std::vector<AlignedBox>     brushCollsion;
    };
}
