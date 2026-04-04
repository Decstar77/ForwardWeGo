#include "editor_asset_packer.h"

#if ATTO_EDITOR

#include <imgui.h>
#include <regex>
#include <fstream>
#include <set>
#include <filesystem>
#include <ctime>
#include <windows.h>
#include <shellapi.h>

#include "pocketlzma.hpp"
#include "game/game_map.h"

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

        // Third pass: explicitly add every .map file under assets/maps/game/ —
        // these are passed as runtime args so they won't appear in source scraping.
        for ( const std::string & rawPath : mapFiles ) {
            std::string normalized = rawPath;
            for ( char & c : normalized ) { if ( c == '\\' ) c = '/'; }
            if ( normalized.rfind( "assets/maps/game/", 0 ) == 0 ) {
                uniquePaths.insert( normalized );
            }
        }

        std::vector<std::string> result( uniquePaths.begin(), uniquePaths.end() );

        LOG_INFO( "Assets found: %d", (i32)result.size() );
        for ( const auto & assetPath : result ) {
            LOG_INFO( "  %s", assetPath.c_str() );
        }

        return result;
    }

    void EditorAssetPacker::BeginPacking() {
        if ( buildPhase != BuildPhase::Idle ) {
            return;
        }

        buildTarget = BuildTarget::Windows;

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
        buildPhase = BuildPhase::Packing;

        LOG_INFO( "Packing %d assets...", (i32)assetPaths.size() );
    }

    void EditorAssetPacker::BeginPackingWeb() {
        if ( buildPhase != BuildPhase::Idle ) {
            return;
        }

        buildTarget = BuildTarget::Web;

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
        buildPhase = BuildPhase::Packing;

        LOG_INFO( "Packing %d assets for web...", (i32)assetPaths.size() );
    }

    enum class PackAssetType {
        Raw,
        Texture,
        StaticModel,
        AnimatedModel,
        Font,
        Audio,
        Map,
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
        if ( extLower == ".map" ) {
            return PackAssetType::Map;
        }

        return PackAssetType::Raw;
    }

    bool EditorAssetPacker::UpdatePacking() {
        if ( buildPhase == BuildPhase::Idle ) {
            return false;
        }

        if ( buildPhase == BuildPhase::Building ) {
            if ( buildFinished.load() ) {
                if ( buildExitCode.load() != 0 ) {
                    errorMessage = buildTarget == BuildTarget::Web
                        ? "Emscripten web build failed.\n\n"
                        : "cmake --build build --config Shipping failed.\n\n";
                    // Show the last 40 lines of output to keep the popup readable
                    const std::string & out = buildOutput;
                    usize start = 0;
                    i32 newlines = 0;
                    for ( usize i = out.size(); i > 0; i-- ) {
                        if ( out[i - 1] == '\n' ) {
                            newlines++;
                            if ( newlines == 40 ) { start = i; break; }
                        }
                    }
                    errorMessage += out.substr( start );
                    buildPhase = BuildPhase::Error;
                } else {
                    if ( buildTarget == BuildTarget::Web ) {
                        DoWebCopyPhase();
                    } else {
                        DoCopyPhase();
                    }
                }
            }
            return true;
        }

        if ( buildPhase == BuildPhase::Done || buildPhase == BuildPhase::Error ||
             buildPhase == BuildPhase::Copying ) {
            return true;
        }

        // BuildPhase::Packing — process one asset per frame
        if ( currentIndex >= (i32)assetPaths.size() ) {
            FinalizePacking();
            if ( buildTarget == BuildTarget::Web ) {
                StartWebBuildThread();
            } else {
                StartBuildThread();
            }
            buildPhase = BuildPhase::Building;
            return true;
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

            case PackAssetType::Map: {
                const std::string jsonText = assetManager.ReadTextFile( path );
                if ( !jsonText.empty() ) {
                    JsonSerializer jsonSer( false );
                    jsonSer.FromString( jsonText );
                    GameMap tempMap;
                    tempMap.Serialize( jsonSer );

                    BinarySerializer binSer( true );
                    tempMap.Serialize( binSer );

                    const auto & buf = binSer.GetBuffer();
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

    std::string EditorAssetPacker::MakeTimestampDir() {
        std::time_t t = std::time( nullptr );
        std::tm tm    = {};
        localtime_s( &tm, &t );
        char buf[ 32 ];
        std::strftime( buf, sizeof( buf ), "%Y%m%d_%H%M%S", &tm );
        return std::string( "ship/" ) + buf;
    }

    static std::string GetCMakeCommand() {
        std::ifstream cache( "build/CMakeCache.txt" );
        if ( !cache.is_open() ) { return "cmake"; }
        std::string line;
        const std::string prefix = "CMAKE_COMMAND:INTERNAL=";
        while ( std::getline( cache, line ) ) {
            if ( line.rfind( prefix, 0 ) == 0 ) {
                return line.substr( prefix.size() );
            }
        }
        return "cmake";
    }

    void EditorAssetPacker::StartBuildThread() {
        buildFinished.store( false );
        buildExitCode.store( 0 );
        buildOutput.clear();
        LOG_INFO( "Starting Shipping build..." );

        buildThread = std::thread( [this]() {
            const std::string cmakePath = GetCMakeCommand();
            const std::string cmd = "\"" + cmakePath + "\" --build build --config Shipping 2>&1";
            FILE * pipe = _popen( cmd.c_str(), "r" );
            if ( !pipe ) {
                buildExitCode.store( -1 );
                buildFinished.store( true );
                return;
            }
            std::string output;
            char buffer[ 256 ];
            while ( fgets( buffer, sizeof( buffer ), pipe ) ) {
                output += buffer;
            }
            int code = _pclose( pipe );
            buildOutput   = std::move( output );
            buildExitCode.store( code );
            buildFinished.store( true );
        } );
        buildThread.detach();
    }

    void EditorAssetPacker::DoCopyPhase() {
        buildPhase = BuildPhase::Copying;

        shipDir = MakeTimestampDir();
        std::error_code ec;
        std::filesystem::create_directories( shipDir, ec );
        if ( ec ) {
            errorMessage = "Failed to create output directory '" + shipDir + "': " + ec.message();
            buildPhase = BuildPhase::Error;
            return;
        }

        auto tryCopy = [&]( const std::string & src, const std::string & dst ) -> bool {
            std::filesystem::copy_file( src, dst,
                std::filesystem::copy_options::overwrite_existing, ec );
            if ( ec ) {
                errorMessage = "Failed to copy '" + src + "' -> '" + dst + "': " + ec.message();
                buildPhase = BuildPhase::Error;
                return false;
            }
            return true;
        };

        if ( !tryCopy( "assets/packed/game.bin",           shipDir + "/game.bin"       ) ) { return; }
        if ( !tryCopy( "build/bin/Shipping/atto.exe",      shipDir + "/atto.exe"       ) ) { return; }
        if ( !tryCopy( "build/bin/Shipping/OpenAL32.dll",  shipDir + "/OpenAL32.dll"   ) ) { return; }

        LOG_INFO( "=== Ship build ready: %s ===", shipDir.c_str() );
        buildPhase = BuildPhase::Done;
    }

    void EditorAssetPacker::StartWebBuildThread() {
        buildFinished.store( false );
        buildExitCode.store( 0 );
        buildOutput.clear();
        LOG_INFO( "Starting Emscripten web build..." );

        buildThread = std::thread( [this]() {
            // Activate emsdk, configure (must re-configure so --preload-file picks up the freshly packed game.bin), and build.
            const std::string cmd =
                "call vendor\\emsdk\\emsdk_env.bat >nul 2>&1 && "
                "call emcmake cmake -B build_web -DCMAKE_BUILD_TYPE=Release >nul 2>&1 && "
                "cmake --build build_web 2>&1";

            FILE * pipe = _popen( cmd.c_str(), "r" );
            if ( !pipe ) {
                buildExitCode.store( -1 );
                buildFinished.store( true );
                return;
            }
            std::string output;
            char buffer[ 256 ];
            while ( fgets( buffer, sizeof( buffer ), pipe ) ) {
                output += buffer;
            }
            int code = _pclose( pipe );
            buildOutput   = std::move( output );
            buildExitCode.store( code );
            buildFinished.store( true );
        } );
        buildThread.detach();
    }

    void EditorAssetPacker::DoWebCopyPhase() {
        buildPhase = BuildPhase::Copying;

        shipDir = MakeTimestampDir() + "_web";
        std::error_code ec;
        std::filesystem::create_directories( shipDir, ec );
        if ( ec ) {
            errorMessage = "Failed to create output directory '" + shipDir + "': " + ec.message();
            buildPhase = BuildPhase::Error;
            return;
        }

        auto tryCopy = [&]( const std::string & src, const std::string & dst ) -> bool {
            std::filesystem::copy_file( src, dst,
                std::filesystem::copy_options::overwrite_existing, ec );
            if ( ec ) {
                errorMessage = "Failed to copy '" + src + "' -> '" + dst + "': " + ec.message();
                buildPhase = BuildPhase::Error;
                return false;
            }
            return true;
        };

        // Copy the Emscripten build outputs (game.bin is embedded in the .data file via --preload-file)
        if ( !tryCopy( "build_web/bin/atto.html",  shipDir + "/index.html" ) ) { return; }
        if ( !tryCopy( "build_web/bin/atto.js",    shipDir + "/atto.js"    ) ) { return; }
        if ( !tryCopy( "build_web/bin/atto.wasm",  shipDir + "/atto.wasm"  ) ) { return; }
        if ( !tryCopy( "build_web/bin/atto.data",  shipDir + "/atto.data"  ) ) { return; }

        LOG_INFO( "=== Web build ready: %s ===", shipDir.c_str() );
        buildPhase = BuildPhase::Done;
    }

    void EditorAssetPacker::DrawProgressPopup() {
        if ( buildPhase == BuildPhase::Idle ) {
            return;
        }

        ImGui::OpenPopup( "Build Game" );
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos( center, ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
        ImGui::SetNextWindowSize( ImVec2( 520, 0 ) );

        if ( ImGui::BeginPopupModal( "Build Game", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) ) {

            if ( buildPhase == BuildPhase::Packing ) {
                i32 total    = (i32)assetPaths.size();
                f32 progress = total > 0 ? (f32)currentIndex / (f32)total : 0.0f;

                ImGui::Text( "Packing assets..." );
                char overlay[ 64 ];
                snprintf( overlay, sizeof( overlay ), "%d / %d", currentIndex, total );
                ImGui::ProgressBar( progress, ImVec2( -1, 0 ), overlay );

                if ( currentIndex < total ) {
                    ImGui::TextWrapped( "%s", assetPaths[currentIndex].c_str() );
                } else {
                    ImGui::Text( "Writing pack file..." );
                }
            }
            else if ( buildPhase == BuildPhase::Building ) {
                if ( buildTarget == BuildTarget::Web ) {
                    ImGui::Text( "Building Emscripten web version..." );
                    const char * frames = "|/-\\";
                    ImGui::Text( "%c  emcmake cmake --build build_web",
                                 frames[ (i32)( ImGui::GetTime() * 8.0 ) & 3 ] );
                } else {
                    ImGui::Text( "Building Shipping configuration..." );
                    const char * frames = "|/-\\";
                    ImGui::Text( "%c  cmake --build build --config Shipping",
                                 frames[ (i32)( ImGui::GetTime() * 8.0 ) & 3 ] );
                }
            }
            else if ( buildPhase == BuildPhase::Copying ) {
                ImGui::Text( "Copying files to %s ...", shipDir.c_str() );
            }
            else if ( buildPhase == BuildPhase::Done ) {
                ImGui::TextColored( ImVec4( 0.2f, 1.0f, 0.2f, 1.0f ), "Build complete!" );
                ImGui::Text( "Output: %s", shipDir.c_str() );
                ImGui::Spacing();
                if ( ImGui::Button( "Open Folder", ImVec2( 120, 0 ) ) ) {
                    std::string absPath = std::filesystem::absolute( shipDir ).string();
                    ShellExecuteA( nullptr, "explore", absPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL );
                }
                ImGui::SameLine();
                if ( ImGui::Button( "Close", ImVec2( 120, 0 ) ) ) {
                    ImGui::CloseCurrentPopup();
                    buildPhase = BuildPhase::Idle;
                }
            }
            else if ( buildPhase == BuildPhase::Error ) {
                ImGui::TextColored( ImVec4( 1.0f, 0.3f, 0.3f, 1.0f ), "Build failed!" );
                ImGui::Spacing();
                ImGui::BeginChild( "##errlog", ImVec2( -1, 280 ), true );
                ImGui::TextWrapped( "%s", errorMessage.c_str() );
                ImGui::EndChild();
                ImGui::Spacing();
                if ( ImGui::Button( "Copy Error", ImVec2( 120, 0 ) ) ) {
                    ImGui::SetClipboardText( errorMessage.c_str() );
                }
                ImGui::SameLine();
                if ( ImGui::Button( "Close", ImVec2( 120, 0 ) ) ) {
                    ImGui::CloseCurrentPopup();
                    buildPhase = BuildPhase::Idle;
                }
            }

            ImGui::EndPopup();
        }
    }
}

#endif