#pragma once

#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_material.h"
#include "editor_asset_thumbnail.h"

#include <memory>
#include <vector>
#include <string>

namespace atto {

    class EditorAssetBrowser {
        friend class EditorScene;
        friend class ImguiPropertySerializer;
    public:
        void OnStart();
        void Draw();

    private:
        void RefreshTextures();
        void RefreshModels();

        void DrawTexturesTab();
        void DrawModelsTab();

        struct TextureEntry {
            std::string     path;
            std::string     filename;
            const Texture * texture = nullptr;
        };

        struct FolderEntry {
            std::string path;
            std::string name;
        };

        struct ModelEntry {
            std::string     path;
            std::string     filename;
            const Texture * thumbnail = nullptr;
        };

        static constexpr const char * TextureRootDir = "assets/textures";
        static constexpr const char * ModelRootDir   = "assets/models";

        // Texture tab state
        std::vector<FolderEntry>  texFolders;
        std::vector<TextureEntry> textures;
        std::string               texCurrentDir;

        // Model tab state
        std::vector<FolderEntry>  modelFolders;
        std::vector<ModelEntry>   models;
        std::string               modelCurrentDir;

        std::unique_ptr<ThumbnailBaker> thumbnailBaker;

        const Texture *           folderIcon     = nullptr;
        const Texture *           modelIcon      = nullptr;
        f32                       thumbnailSize  = 80.0f;
        bool                      isOpen         = false;
        i32                       activeTab      = 0; // 0 = Textures, 1 = Models

        // Texture selection for brushes
        i32                       selectingForBrush = -1;
        std::string               selectedTexturePath;
        bool                      selectionMade     = false;

        // Model selection
        bool                      selectingForModel  = false;
        std::string               selectedModelPath;
        bool                      modelSelectionMade = false;
    };

} // namespace atto
