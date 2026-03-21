#pragma once

#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_material.h"

#include <vector>
#include <string>

namespace atto {

    class EditorAssetBrowser {
        friend class EditorScene;
    public:
        void OnStart();
        void Draw();

    private:
        void Refresh();

        struct TextureEntry {
            std::string     path;
            std::string     filename;
            const Texture * texture = nullptr;
        };

        struct FolderEntry {
            std::string path;
            std::string name;
        };

        static constexpr const char * RootDir = "assets/textures";

        std::vector<FolderEntry>  folders;
        std::vector<TextureEntry> textures;
        std::string               currentDir;
        const Texture *           folderIcon     = nullptr;
        f32                       thumbnailSize  = 80.0f;
        bool                      isOpen         = false;
    };

} // namespace atto
