
#include "engine/atto_engine.h"

namespace atto {

    struct PackHeader {
        u32 magic;
        u32 version;
        u32 assetCount;
        u32 tableOffset;
    };

    struct AssetEntry {
        u64 pathHash;     // hashed asset path for fast lookup
        u64 offset;       // byte offset into file
        u32 compressedSize;
        u32 uncompressedSize;
        u8  compressionType; // 0 = none, 1 = zstd, 2 = lz4, etc.
    };

    class EditorAssetPacker {
    public:
        static std::vector<std::string>    ScrapeAssets();
        static void                        PackAssets();
    };
}
