#include <regex>
#include "editor_asset_packer.h"

#define POCKETLZMA_LZMA_C_DEFINE
#include "pocketlzma.hpp"

namespace atto {
    std::vector<std::string> EditorAssetPacker::ScrapeAssets() {
        AssetManager & assetManager = Engine::Get().GetAssetManager();
        std::vector< std::string > files = assetManager.GetFilesInFolderRecursive( "atto/src", ".h" );
        std::vector< std::string > headerFiles = assetManager.GetFilesInFolderRecursive( "atto/src", ".cpp" );
        std::vector< std::string > mapFiles = assetManager.GetFilesInFolderRecursive( "assets/maps", ".map" );

        files.insert(files.end(), headerFiles.begin(), headerFiles.end());
        files.insert(files.end(), mapFiles.begin(), mapFiles.end());

        std::vector< std::string > assetPaths;
        for ( const std::string &file: files ) {
            const std::string data = assetManager.ReadTextFile( file );

            std::regex assetRegex( R"(assets/[^\s"'<>]+)" );
            auto begin = std::sregex_iterator( data.begin(), data.end(), assetRegex );
            auto end = std::sregex_iterator();
            for ( auto it = begin; it != end; ++it ) {
                assetPaths.push_back( it->str() );
            }
        }

        LOG_INFO( "Assets found:" );
        for ( auto assetPath: assetPaths ) {
            LOG_INFO( assetPath.c_str() );
        }

        return assetPaths;
    }

    void EditorAssetPacker::PackAssets() {
        std::vector<std::string> assetPaths = ScrapeAssets();

    }
}
