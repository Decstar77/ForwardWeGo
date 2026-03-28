
#include "engine/atto_engine.h"

namespace atto {

    constexpr u32 PACK_MAGIC   = 0x4F545441; // 'ATTO'
    constexpr u32 PACK_VERSION = 1;

    struct PackHeader {
        u32 magic;
        u32 version;
        u32 assetCount;
        u32 _pad0;
        u64 tableOffset;    // byte offset to the asset entry table
    };

    struct AssetEntry {
        u64 pathHash;           // FNV-1a hash of the asset path
        u64 offset;             // absolute byte offset into the pack file
        u32 compressedSize;
        u32 uncompressedSize;
        u8  compressionType;    // 0 = none, 1 = lzma
        u8  _pad0[3];
        u32 pathLength;         // length of the path string that follows this entry
    };

    struct PackedAssetData {
        std::string     path;
        AssetEntry      entry;
        std::vector<u8> data;
    };

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
