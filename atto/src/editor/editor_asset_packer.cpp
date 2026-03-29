#include <regex>
#include <fstream>
#include <set>
#include <filesystem>

#include "editor_asset_packer.h"

#include <imgui.h>

#include "pocketlzma.hpp"

namespace atto {

    static u64 FNV1a( const char * str, usize len ) {
        u64 hash = 14695981039346656037ULL;
        for ( usize i = 0; i < len; i++ ) {
            hash ^= static_cast<u64>( str[i] );
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    static bool PathHasExtension( const std::string & path ) {
        const auto lastSlash = path.rfind( '/' );
        const auto lastDot   = path.rfind( '.' );
        return lastDot != std::string::npos &&
               ( lastSlash == std::string::npos || lastDot > lastSlash );
    }

    static void ScrapeMtlTextures( AssetManager & assetManager, const std::string & objPath,
                                   std::set<std::string> & out ) {
        // Derive MTL path: same directory, same base name, .mtl extension
        const std::string mtlPath = objPath.substr( 0, objPath.size() - 4 ) + ".mtl";
        const std::string mtlData = assetManager.ReadTextFile( mtlPath );
        if ( mtlData.empty() ) { return; }

        // Directory of the OBJ (to resolve relative texture paths)
        std::string dir;
        const auto lastSlash = objPath.rfind( '/' );
        if ( lastSlash != std::string::npos ) {
            dir = objPath.substr( 0, lastSlash + 1 );
        }

        // Match texture directives: map_Kd, map_Ka, map_Ks, map_Bump, bump, map_d, map_Ns, map_refl
        std::regex mtlMapRegex( R"((?:map_Kd|map_Ka|map_Ks|map_Bump|map_d|map_Ns|map_refl|bump)\s+(\S+))",
                                std::regex::icase );
        auto begin = std::sregex_iterator( mtlData.begin(), mtlData.end(), mtlMapRegex );
        auto end   = std::sregex_iterator();
        for ( auto it = begin; it != end; ++it ) {
            std::string texName = (*it)[1].str();

            // Normalize backslashes
            for ( char & c : texName ) { if ( c == '\\' ) c = '/'; }

            // If not already a full relative path from project root, prepend obj directory
            std::string texPath = ( texName.rfind( "assets/", 0 ) == 0 ) ? texName : dir + texName;

            if ( PathHasExtension( texPath ) ) {
                out.insert( texPath );
            }
        }
    }

    std::vector<std::string> EditorAssetPacker::ScrapeAssets() {
        AssetManager & assetManager = Engine::Get().GetAssetManager();
        std::vector<std::string> files = assetManager.GetFilesInFolderRecursive( "atto/src", ".h" );
        std::vector<std::string> cppFiles = assetManager.GetFilesInFolderRecursive( "atto/src", ".cpp" );
        std::vector<std::string> mapFiles = assetManager.GetFilesInFolderRecursive( "assets/maps", ".map" );

        files.insert( files.end(), cppFiles.begin(), cppFiles.end() );
        files.insert( files.end(), mapFiles.begin(), mapFiles.end() );

        // First pass: regex-scrape all assets/... paths from source and map files.
        // Only keep paths that have a file extension (filters bare directory names).
        std::set<std::string> uniquePaths;
        for ( const std::string & file : files ) {
            const std::string data = assetManager.ReadTextFile( file );
            std::regex assetRegex( R"(assets/[^\s"'<>]+)" );
            auto begin = std::sregex_iterator( data.begin(), data.end(), assetRegex );
            auto end   = std::sregex_iterator();
            for ( auto it = begin; it != end; ++it ) {
                const std::string path = it->str();
                if ( PathHasExtension( path ) ) {
                    uniquePaths.insert( path );
                }
            }
        }

        // Second pass: for any .obj paths found, parse their .mtl for texture references.
        std::set<std::string> mtlTextures;
        for ( const std::string & path : uniquePaths ) {
            const char * ext = strrchr( path.c_str(), '.' );
            if ( ext && strcmp( ext, ".obj" ) == 0 ) {
                ScrapeMtlTextures( assetManager, path, mtlTextures );
            }
        }
        for ( const std::string & p : mtlTextures ) {
            uniquePaths.insert( p );
        }

        std::vector<std::string> result( uniquePaths.begin(), uniquePaths.end() );

        LOG_INFO( "Assets found: %d", (i32)result.size() );
        for ( const auto & assetPath : result ) {
            LOG_INFO( "  %s", assetPath.c_str() );
        }

        return result;
    }

    void EditorAssetPacker::BeginPacking() {
        if ( packing ) {
            return;
        }

        std::filesystem::remove( "assets/packed/game.bin" );
        std::filesystem::remove( "assets/packed/scraped.txt" );

        assetPaths = ScrapeAssets();
        if ( assetPaths.empty() ) {
            LOG_WARN( "No assets found to pack" );
            return;
        }

        std::stringstream ss;
        for ( auto path : assetPaths ) {
            ss << path << std::endl;
        }

        Engine::Get().GetAssetManager().WriteTextFile( "assets/packed/scraped.txt", ss.str().c_str() );

        packedAssets.clear();
        currentIndex = 0;
        dataOffset = sizeof( PackHeader );
        packing = true;

        LOG_INFO( "Packing %d assets...", (i32)assetPaths.size() );
    }

    enum class PackAssetType {
        Raw,
        Texture,
        StaticModel,
        AnimatedModel,
        Font,
        Audio,
    };

    static PackAssetType DetermineAssetType( const std::string & path ) {
        const char * ext = strrchr( path.c_str(), '.' );
        if ( !ext ) {
            return PackAssetType::Raw;
        }

        std::string extLower = ext;
        for ( char & c : extLower ) {
            if ( c >= 'A' && c <= 'Z' ) c += 'a' - 'A';
        }

        if ( extLower == ".png" || extLower == ".jpg" || extLower == ".jpeg" ||
             extLower == ".tga" || extLower == ".bmp" || extLower == ".hdr" ) {
            return PackAssetType::Texture;
        }
        if ( extLower == ".glb" ) {
            return PackAssetType::AnimatedModel;
        }
        if ( extLower == ".fbx" || extLower == ".obj" || extLower == ".gltf" ) {
            return PackAssetType::StaticModel;
        }
        if ( extLower == ".ttf" || extLower == ".otf" ) {
            return PackAssetType::Font;
        }
        if ( extLower == ".wav" || extLower == ".ogg" ) {
            return PackAssetType::Audio;
        }

        return PackAssetType::Raw;
    }

    bool EditorAssetPacker::UpdatePacking() {
        if ( !packing ) {
            return false;
        }

        if ( currentIndex >= (i32)assetPaths.size() ) {
            FinalizePacking();
            packing = false;
            return false;
        }

        const std::string & path = assetPaths[currentIndex];
        AssetManager & assetManager = Engine::Get().GetAssetManager();
        PackAssetType assetType = DetermineAssetType( path );

        std::vector<u8> rawData;
        bool loaded = false;

        switch ( assetType ) {
            case PackAssetType::Texture: {
                BinarySerializer serializer( true );
                if ( assetManager.LoadTextureDataRaw( path.c_str(), serializer ) ) {
                    const auto & buf = serializer.GetBuffer();
                    rawData.assign( buf.begin(), buf.end() );
                    loaded = true;
                }
            } break;

            case PackAssetType::StaticModel: {
                BinarySerializer serializer( true );
                if ( assetManager.LoadStaticModelDataRaw( path.c_str(), serializer ) ) {
                    const auto & buf = serializer.GetBuffer();
                    rawData.assign( buf.begin(), buf.end() );
                    loaded = true;
                }
            } break;

            case PackAssetType::AnimatedModel: {
                BinarySerializer serializer( true );
                if ( assetManager.LoadAnimatedModelDataRaw( path.c_str(), serializer ) ) {
                    const auto & buf = serializer.GetBuffer();
                    rawData.assign( buf.begin(), buf.end() );
                    loaded = true;
                }
            } break;

            case PackAssetType::Font: {
                BinarySerializer serializer( true );
                f32 defaultFontSize = 24.0f;
                if ( assetManager.LoadFontDataRaw( path.c_str(), defaultFontSize, serializer ) ) {
                    const auto & buf = serializer.GetBuffer();
                    rawData.assign( buf.begin(), buf.end() );
                    loaded = true;
                }
            } break;

            case PackAssetType::Audio: {
                BinarySerializer serializer( true );
                if ( assetManager.LoadSoundRaw( path.c_str(), false, serializer ) ) {
                    const auto & buf = serializer.GetBuffer();
                    rawData.assign( buf.begin(), buf.end() );
                    loaded = true;
                }
            } break;

            case PackAssetType::Raw:
                break;
        }

        // Fallback: raw file copy for unrecognized types or if loading failed
        if ( !loaded ) {
            std::ifstream file( path, std::ios::binary | std::ios::ate );
            if ( !file.is_open() ) {
                LOG_WARN( "PackAssets: Could not open '%s', skipping", path.c_str() );
                currentIndex++;
                return true;
            }

            std::streamsize fileSize = file.tellg();
            file.seekg( 0 );
            rawData.resize( (usize)fileSize );
            file.read( reinterpret_cast<char *>( rawData.data() ), fileSize );
            file.close();
        }

        plz::PocketLzma lzma( plz::Preset::Default );
        std::vector<u8> compressed;
        plz::StatusCode status = lzma.compress( rawData, compressed );

        PackedAssetData packed = {};
        packed.path = path;
        packed.entry.pathHash = FNV1a( path.c_str(), path.size() );
        packed.entry.offset = dataOffset;
        packed.entry.pathLength = (u32)path.size();
        memset( packed.entry._pad0, 0, sizeof( packed.entry._pad0 ) );

        if ( status == plz::StatusCode::Ok && compressed.size() < rawData.size() ) {
            packed.entry.compressedSize = (u32)compressed.size();
            packed.entry.uncompressedSize = (u32)rawData.size();
            packed.entry.compressionType = 1; // LZMA
            packed.data = std::move( compressed );
        }
        else {
            packed.entry.compressedSize = (u32)rawData.size();
            packed.entry.uncompressedSize = (u32)rawData.size();
            packed.entry.compressionType = 0; // None
            packed.data = std::move( rawData );
        }

        dataOffset += packed.entry.compressedSize;

        LOG_INFO( "Packed: %s (%u -> %u bytes, %s)",
            path.c_str(),
            packed.entry.uncompressedSize,
            packed.entry.compressedSize,
            packed.entry.compressionType == 1 ? "lzma" : "none" );

        packedAssets.push_back( std::move( packed ) );
        currentIndex++;

        return true;
    }

    void EditorAssetPacker::FinalizePacking() {
        std::filesystem::create_directories( "assets/packed" );
        std::ofstream out( "assets/packed/game.bin", std::ios::binary );
        if ( !out.is_open() ) {
            LOG_ERROR( "PackAssets: Failed to create assets/packed/game.bin" );
            return;
        }

        PackHeader header = {};
        header.magic = PACK_MAGIC;
        header.version = PACK_VERSION;
        header.assetCount = (u32)packedAssets.size();
        header._pad0 = 0;
        header.tableOffset = dataOffset;
        out.write( reinterpret_cast<const char *>( &header ), sizeof( PackHeader ) );

        for ( const PackedAssetData & asset : packedAssets ) {
            out.write( reinterpret_cast<const char *>( asset.data.data() ), asset.data.size() );
        }

        for ( const PackedAssetData & asset : packedAssets ) {
            out.write( reinterpret_cast<const char *>( &asset.entry ), sizeof( AssetEntry ) );
            out.write( asset.path.c_str(), asset.path.size() );
        }

        out.close();

        u64 totalUncompressed = 0;
        u64 totalCompressed = 0;
        for ( const PackedAssetData & asset : packedAssets ) {
            totalUncompressed += asset.entry.uncompressedSize;
            totalCompressed += asset.entry.compressedSize;
        }

        LOG_INFO( "=== Pack complete ===" );
        LOG_INFO( "  Assets: %u", header.assetCount );
        LOG_INFO( "  Uncompressed: %llu bytes", totalUncompressed );
        LOG_INFO( "  Compressed:   %llu bytes", totalCompressed );
        LOG_INFO( "  Ratio:        %.1f%%", totalCompressed * 100.0 / ( totalUncompressed > 0 ? totalUncompressed : 1 ) );
        LOG_INFO( "  Output:       assets/packed/game.bin" );

        packedAssets.clear();
        assetPaths.clear();
    }

    void EditorAssetPacker::DrawProgressPopup() {
        if ( !packing ) {
            return;
        }

        ImGui::OpenPopup( "Packing Assets" );
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos( center, ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
        ImGui::SetNextWindowSize( ImVec2( 400, 0 ) );

        if ( ImGui::BeginPopupModal( "Packing Assets", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) ) {
            i32 total = (i32)assetPaths.size();
            f32 progress = total > 0 ? (f32)currentIndex / (f32)total : 0.0f;

            char overlay[64];
            snprintf( overlay, sizeof( overlay ), "%d / %d", currentIndex, total );
            ImGui::ProgressBar( progress, ImVec2( -1, 0 ), overlay );

            if ( currentIndex < total ) {
                // Show current asset name, truncated from the left if too long
                const std::string & name = assetPaths[currentIndex];
                ImGui::TextWrapped( "%s", name.c_str() );
            }
            else {
                ImGui::Text( "Writing pack file..." );
            }

            ImGui::EndPopup();
        }
    }
}
