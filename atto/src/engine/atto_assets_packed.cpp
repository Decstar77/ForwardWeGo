#include "atto_assets.h"
#include "atto_log.h"

#define POCKETLZMA_LZMA_C_DEFINE
#include "pocketlzma.hpp"

namespace atto {

    static u64 FNV1a( const char * str, usize len ) {
        u64 hash = 14695981039346656037ULL;
        for ( usize i = 0; i < len; i++ ) {
            hash ^= static_cast<u64>( str[ i ] );
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    // Finds the asset entry for `filePath` in the loaded pack blob, decompresses it,
    // and returns the raw (pre-serialized) bytes. Returns false if not found or corrupt.
    static bool ExtractPackedAsset( const std::vector<u8> & packData, const char * filePath,
                                    std::vector<u8> & outData ) {
        if ( packData.size() < sizeof( PackHeader ) ) {
            LOG_ERROR( "ExtractPackedAsset: pack data too small" );
            return false;
        }

        const PackHeader * header = reinterpret_cast<const PackHeader *>( packData.data() );
        if ( header->magic != PACK_MAGIC ) {
            LOG_ERROR( "ExtractPackedAsset: invalid magic" );
            return false;
        }
        if ( header->version != PACK_VERSION ) {
            LOG_ERROR( "ExtractPackedAsset: version mismatch (%u)", header->version );
            return false;
        }

        const u64 hash       = FNV1a( filePath, strlen( filePath ) );
        usize     tablePos   = (usize)header->tableOffset;
        const u32 assetCount = header->assetCount;

        for ( u32 i = 0; i < assetCount; i++ ) {
            if ( tablePos + sizeof( AssetEntry ) > packData.size() ) {
                LOG_ERROR( "ExtractPackedAsset: table entry %u out of bounds", i );
                return false;
            }

            const AssetEntry * entry = reinterpret_cast<const AssetEntry *>( packData.data() + tablePos );
            tablePos += sizeof( AssetEntry ) + entry->pathLength;

            if ( entry->pathHash != hash ) { continue; }

            // Found matching entry — extract blob
            const usize blobStart = (usize)entry->offset;
            const usize blobSize  = (usize)entry->compressedSize;
            if ( blobStart + blobSize > packData.size() ) {
                LOG_ERROR( "ExtractPackedAsset: blob for '%s' out of bounds", filePath );
                return false;
            }

            if ( entry->compressionType == 1 ) {
                // LZMA decompress
                std::vector<u8> compressed( packData.begin() + blobStart,
                                            packData.begin() + blobStart + blobSize );
                plz::PocketLzma  lzma;
                plz::StatusCode  status = lzma.decompress( compressed, outData );
                if ( status != plz::StatusCode::Ok ) {
                    LOG_ERROR( "ExtractPackedAsset: LZMA decompress failed for '%s' (status %d)",
                               filePath, (i32)status );
                    return false;
                }
            } else {
                outData.assign( packData.begin() + blobStart,
                                packData.begin() + blobStart + blobSize );
            }

            return true;
        }

        LOG_ERROR( "ExtractPackedAsset: '%s' not found in pack", filePath );
        return false;
    }

    // Shared helper: extract asset bytes and feed them into the BinarySerializer buffer.
    // The caller (renderer / audio) will then call serializer.Reset(false) and read back.
    static bool LoadFromPack( const std::vector<u8> & packData, const char * filePath,
                              Serializer & serializer ) {
        std::vector<u8> data;
        if ( !ExtractPackedAsset( packData, filePath, data ) ) { return false; }
        static_cast<BinarySerializer &>( serializer ).SetBuffer( data );
        return true;
    }

    bool AssetManager::LoadTextureDataPacked( const char * filePath, Serializer & serializer ) {
        return LoadFromPack( packedAssetData, filePath, serializer );
    }

    bool AssetManager::LoadStaticModelDataPacked( const char * filePath, Serializer & serializer ) {
        return LoadFromPack( packedAssetData, filePath, serializer );
    }

    bool AssetManager::LoadAnimatedModelDataPacked( const char * filePath, Serializer & serializer ) {
        return LoadFromPack( packedAssetData, filePath, serializer );
    }

    bool AssetManager::LoadFontDataPacked( const char * filePath, f32 /*fontSize*/,
                                           Serializer & serializer ) {
        return LoadFromPack( packedAssetData, filePath, serializer );
    }

    bool AssetManager::LoadSoundPacked( const char * path, bool /*mono*/,
                                        Serializer & serializer ) {
        return LoadFromPack( packedAssetData, path, serializer );
    }

    std::string AssetManager::LoadShaderTextPacked( const char * filePath ) {
        std::vector<u8> data;
        if ( !ExtractPackedAsset( packedAssetData, filePath, data ) ) {
            return {};
        }
        return std::string( reinterpret_cast<const char *>( data.data() ), data.size() );
    }
}
