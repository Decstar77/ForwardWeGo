#include "editor_asset_browser.h"

#include "editor_asset_thumbnail.h"

#include <imgui.h>
#include <filesystem>

namespace atto {

    namespace fs = std::filesystem;

    void EditorAssetBrowser::OnStart() {
        texCurrentDir   = TextureRootDir;
        modelCurrentDir = ModelRootDir;

        thumbnailBaker = std::make_unique<ThumbnailBaker>();

        Renderer & renderer = Engine::Get().GetRenderer();
        folderIcon = renderer.GetOrLoadTexture( "assets/textures/editor/folder.png", true );
        modelIcon  = renderer.GetOrLoadTexture( "assets/textures/editor/model_icon.png", true );

        RefreshTextures();
        RefreshModels();
    }

    void EditorAssetBrowser::RefreshTextures() {
        texFolders.clear();
        textures.clear();

        const fs::path dir = texCurrentDir;
        if ( !fs::exists( dir ) || !fs::is_directory( dir ) ) {
            return;
        }

        Renderer & renderer = Engine::Get().GetRenderer();

        for ( const fs::directory_entry & entry : fs::directory_iterator( dir ) ) {
            if ( entry.is_directory() ) {
                FolderEntry & folder = texFolders.emplace_back();
                folder.path = entry.path().string();
                folder.name = entry.path().filename().string();
                continue;
            }

            if ( !entry.is_regular_file() ) {
                continue;
            }

            const std::string ext = entry.path().extension().string();
            if ( ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".tga" && ext != ".bmp" ) {
                continue;
            }

            TextureEntry & tex = textures.emplace_back();
            tex.path           = entry.path().string();
            tex.filename       = entry.path().filename().string();
            tex.texture        = renderer.GetOrLoadTexture( tex.path.c_str() );
        }
    }

    void EditorAssetBrowser::RefreshModels() {
        modelFolders.clear();
        models.clear();

        const fs::path dir = modelCurrentDir;
        if ( !fs::exists( dir ) || !fs::is_directory( dir ) ) {
            return;
        }

        Renderer & renderer = Engine::Get().GetRenderer();

        for ( const fs::directory_entry & entry : fs::directory_iterator( dir ) ) {
            if ( entry.is_directory() ) {
                FolderEntry & folder = modelFolders.emplace_back();
                folder.path = entry.path().string();
                folder.name = entry.path().filename().string();
                continue;
            }

            if ( !entry.is_regular_file() ) {
                continue;
            }

            const std::string ext = entry.path().extension().string();
            if ( ext != ".obj" && ext != ".fbx" && ext != ".glb" && ext != ".gltf" ) {
                continue;
            }

            ModelEntry & model = models.emplace_back();
            model.path     = entry.path().string();
            model.filename = entry.path().filename().string();

            const std::string stem      = entry.path().stem().string();
            const std::string thumbPath = "assets/textures/editor/thumbnails/" + stem + ".png";
            if ( fs::exists( thumbPath ) ) {
                model.thumbnail = renderer.GetOrLoadTexture( thumbPath.c_str(), true );
            }
        }
    }

    void EditorAssetBrowser::DrawTexturesTab() {
        if ( selectingForBrush >= 0 ) {
            ImGui::TextColored( ImVec4( 1.0f, 0.8f, 0.2f, 1.0f ), "Select texture for Brush %d", selectingForBrush );
            ImGui::SameLine();
            if ( ImGui::SmallButton( "Cancel" ) ) {
                selectingForBrush = -1;
            }
            ImGui::Separator();
        }

        // Navigation bar
        const bool atRoot = ( texCurrentDir == TextureRootDir );
        if ( !atRoot ) {
            if ( ImGui::Button( "<- Up" ) ) {
                texCurrentDir = fs::path( texCurrentDir ).parent_path().string();
                RefreshTextures();
            }
            ImGui::SameLine();
        }

        const std::string relPath = fs::relative( fs::path( texCurrentDir ), "assets" ).string();
        ImGui::TextDisabled( "%s", relPath.c_str() );

        ImGui::SameLine( ImGui::GetContentRegionAvail().x - 160.0f );
        if ( ImGui::Button( "Refresh##tex" ) ) {
            RefreshTextures();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth( 100.0f );
        ImGui::SliderFloat( "##size_tex", &thumbnailSize, 40.0f, 200.0f );

        ImGui::Separator();

        // Scrollable grid region
        ImGui::BeginChild( "##tex_scroll", ImVec2( 0, 0 ), false );

        // Grid layout
        const f32 padding     = 8.0f;
        const f32 cellSize    = thumbnailSize + padding;
        const f32 panelWidth  = ImGui::GetContentRegionAvail().x;
        const i32 columnCount = Max( 1, (i32)( panelWidth / cellSize ) );

        ImGui::Columns( columnCount, nullptr, false );

        const ImVec2 thumbSize( thumbnailSize, thumbnailSize );
        const ImVec2 uv0( 0, 1 );
        const ImVec2 uv1( 1, 0 );

        // Folders first
        ImTextureID folderTexID = (ImTextureID)(intptr_t)( folderIcon ? folderIcon->GetHandle() : 0 );

        for ( i32 i = 0; i < (i32)texFolders.size(); i++ ) {
            ImGui::PushID( i );

            if ( ImGui::ImageButton( "##folder", folderTexID, thumbSize, uv0, uv1 ) ) {
                texCurrentDir = texFolders[i].path;
                RefreshTextures();
                ImGui::PopID();
                break;
            }

            if ( ImGui::IsItemHovered() ) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted( texFolders[i].path.c_str() );
                ImGui::EndTooltip();
            }

            ImGui::TextUnformatted( texFolders[i].name.c_str() );
            ImGui::NextColumn();
            ImGui::PopID();
        }

        // Textures
        const bool selecting = selectingForBrush >= 0;

        for ( i32 i = 0; i < (i32)textures.size(); i++ ) {
            ImGui::PushID( (i32)texFolders.size() + i );

            ImTextureID texID = (ImTextureID)(intptr_t)( textures[i].texture ? textures[i].texture->GetHandle() : 0 );

            if ( selecting ) {
                if ( ImGui::ImageButton( "##tex", texID, thumbSize, uv0, uv1 ) ) {
                    selectedTexturePath = textures[i].path;
                    selectionMade = true;
                }
            }
            else {
                ImGui::Image( texID, thumbSize, uv0, uv1 );
            }

            if ( ImGui::IsItemHovered() ) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted( textures[i].path.c_str() );
                if ( textures[i].texture ) {
                    ImGui::Text( "%dx%d", textures[i].texture->GetWidth(), textures[i].texture->GetHeight() );
                }
                if ( selecting ) {
                    ImGui::Text( "Click to apply" );
                }
                ImGui::EndTooltip();
            }

            ImGui::TextUnformatted( textures[i].filename.c_str() );
            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns( 1 );
        ImGui::EndChild();
    }

    void EditorAssetBrowser::DrawModelsTab() {
        if ( selectingForModel ) {
            ImGui::TextColored( ImVec4( 1.0f, 0.8f, 0.2f, 1.0f ), "Select a model" );
            ImGui::SameLine();
            if ( ImGui::SmallButton( "Cancel##mdl" ) ) {
                selectingForModel = false;
            }
            ImGui::Separator();
        }

        // Navigation bar
        const bool atRoot = ( modelCurrentDir == ModelRootDir );
        if ( !atRoot ) {
            if ( ImGui::Button( "<- Up##mdl" ) ) {
                modelCurrentDir = fs::path( modelCurrentDir ).parent_path().string();
                RefreshModels();
            }
            ImGui::SameLine();
        }

        const std::string relPath = fs::relative( fs::path( modelCurrentDir ), "assets" ).string();
        ImGui::TextDisabled( "%s", relPath.c_str() );

        ImGui::SameLine( ImGui::GetContentRegionAvail().x - 280.0f );
        if ( ImGui::Button( "Regen Thumbs##mdl" ) ) {
            LargeString fixedPath = LargeString::FromLiteral( modelCurrentDir.c_str() );
            fixedPath.BackSlashesToSlashes();
            thumbnailBaker->GenerateThumbnailsForFolder( fixedPath.GetCStr() );
            RefreshModels();
        }
        ImGui::SameLine();
        if ( ImGui::Button( "Refresh##mdl" ) ) {
            RefreshModels();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth( 100.0f );
        ImGui::SliderFloat( "##size_mdl", &thumbnailSize, 40.0f, 200.0f );

        ImGui::Separator();

        // Scrollable grid region
        ImGui::BeginChild( "##mdl_scroll", ImVec2( 0, 0 ), false );

        // Grid layout
        const f32 padding     = 8.0f;
        const f32 cellSize    = thumbnailSize + padding;
        const f32 panelWidth  = ImGui::GetContentRegionAvail().x;
        const i32 columnCount = Max( 1, (i32)( panelWidth / cellSize ) );

        ImGui::Columns( columnCount, nullptr, false );

        const ImVec2 thumbSize( thumbnailSize, thumbnailSize );
        const ImVec2 uv0( 0, 1 );
        const ImVec2 uv1( 1, 0 );

        // Folders first
        ImTextureID folderTexID = (ImTextureID)(intptr_t)( folderIcon ? folderIcon->GetHandle() : 0 );

        for ( i32 i = 0; i < (i32)modelFolders.size(); i++ ) {
            ImGui::PushID( i );

            if ( ImGui::ImageButton( "##mfolder", folderTexID, thumbSize, uv0, uv1 ) ) {
                modelCurrentDir = modelFolders[i].path;
                RefreshModels();
                ImGui::PopID();
                break;
            }

            if ( ImGui::IsItemHovered() ) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted( modelFolders[i].path.c_str() );
                ImGui::EndTooltip();
            }

            ImGui::TextUnformatted( modelFolders[i].name.c_str() );
            ImGui::NextColumn();
            ImGui::PopID();
        }

        // Models
        ImTextureID fallbackTexID = (ImTextureID)(intptr_t)( modelIcon ? modelIcon->GetHandle() : 0 );

        for ( i32 i = 0; i < (i32)models.size(); i++ ) {
            ImGui::PushID( (i32)modelFolders.size() + i );

            const ModelEntry & m = models[i];
            const bool hasTex = ( m.thumbnail != nullptr ) || ( modelIcon != nullptr );

            if ( hasTex ) {
                ImTextureID texID = m.thumbnail
                    ? (ImTextureID)(intptr_t)m.thumbnail->GetHandle()
                    : fallbackTexID;
                if ( ImGui::ImageButton( "##mdl", texID, thumbSize, uv0, uv1 ) ) {
                    selectedModelPath  = m.path;
                    modelSelectionMade = true;
                }
            }
            else {
                // Fallback: colored button
                ImGui::PushStyleColor( ImGuiCol_Button,        ImVec4( 0.2f, 0.3f, 0.5f, 1.0f ) );
                ImGui::PushStyleColor( ImGuiCol_ButtonHovered,  ImVec4( 0.3f, 0.4f, 0.6f, 1.0f ) );
                ImGui::PushStyleColor( ImGuiCol_ButtonActive,   ImVec4( 0.4f, 0.5f, 0.7f, 1.0f ) );
                if ( ImGui::Button( "3D", thumbSize ) ) {
                    selectedModelPath  = m.path;
                    modelSelectionMade = true;
                }
                ImGui::PopStyleColor( 3 );
            }

            if ( ImGui::IsItemHovered() ) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted( m.path.c_str() );
                ImGui::EndTooltip();
            }

            ImGui::TextUnformatted( m.filename.c_str() );
            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns( 1 );
        ImGui::EndChild();
    }

    void EditorAssetBrowser::Draw() {
        if ( !isOpen ) {
            selectingForBrush = -1;
            selectingForModel = false;
            return;
        }

        ImGui::Begin( "Asset Browser", &isOpen );

        // Auto-select Models tab when selecting for model
        ImGuiTabItemFlags modelTabFlags = 0;
        if ( selectingForModel ) {
            modelTabFlags = ImGuiTabItemFlags_SetSelected;
        }

        if ( ImGui::BeginTabBar( "AssetBrowserTabs" ) ) {
            if ( ImGui::BeginTabItem( "Textures" ) ) {
                activeTab = 0;
                DrawTexturesTab();
                ImGui::EndTabItem();
            }
            if ( ImGui::BeginTabItem( "Models", nullptr, modelTabFlags ) ) {
                activeTab = 1;
                DrawModelsTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

} // namespace atto
