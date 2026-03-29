
#include "engine/atto_engine.h"

namespace atto {



    class EditorAssetPacker {
    public:
        static std::vector<std::string> ScrapeAssets();

        // Call from menu to kick off packing
        void BeginPacking();

        // Call each frame. Returns true while packing is in progress.
        bool UpdatePacking();

        // Draw the progress bar popup. Call each frame during ImGui rendering.
        void DrawProgressPopup();

        bool IsPacking() const { return packing; }

    private:
        void FinalizePacking();

        bool                        packing = false;
        i32                         currentIndex = 0;
        std::vector<std::string>    assetPaths;
        std::vector<PackedAssetData> packedAssets;
        u64                         dataOffset = 0;
    };
}
