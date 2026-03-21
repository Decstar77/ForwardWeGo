#include "editor_asset_browser.h"

#include <imgui.h>
#include <filesystem>

namespace atto {

    namespace fs = std::filesystem;

    void EditorAssetBrowser::OnStart() {
        currentDir = RootDir;
        folderIcon = Engine::Get().GetRenderer().GetOrLoadTexture( "assets/textures/folder.png" );
        Refresh();
    }

    void EditorAssetBrowser::Refresh() {
        folders.clear();
        textures.clear();

        const fs::path dir = currentDir;
        if ( !fs::exists( dir ) || !fs::is_directory( dir ) ) {
            return;
        }

        Renderer & renderer = Engine::Get().GetRenderer();

        for ( const fs::directory_entry & entry : fs::directory_iterator( dir ) ) {
            if ( entry.is_directory() ) {
                FolderEntry & folder = folders.emplace_back();
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

    void EditorAssetBrowser::Draw() {
        if ( !isOpen ) {
            return;
        }

        ImGui::Begin( "Asset Browser", &isOpen );

        // Navigation bar
        const bool atRoot = ( currentDir == RootDir );
        if ( !atRoot ) {
            if ( ImGui::Button( "<- Up" ) ) {
                currentDir = fs::path( currentDir ).parent_path().string();
                Refresh();
            }
            ImGui::SameLine();
        }

        // Show path relative to "assets/" for brevity
        const std::string relPath = fs::relative( fs::path( currentDir ), "assets" ).string();
        ImGui::TextDisabled( "%s", relPath.c_str() );

        ImGui::SameLine( ImGui::GetContentRegionAvail().x - 160.0f );
        if ( ImGui::Button( "Refresh" ) ) {
            Refresh();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth( 100.0f );
        ImGui::SliderFloat( "##size", &thumbnailSize, 40.0f, 200.0f );

        ImGui::Separator();

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

        for ( i32 i = 0; i < (i32)folders.size(); i++ ) {
            ImGui::PushID( i );

            if ( ImGui::ImageButton( "##folder", folderTexID, thumbSize, uv0, uv1 ) ) {
                currentDir = folders[i].path;
                Refresh();
                ImGui::PopID();
                break; // folders vector was cleared — stop iterating
            }

            if ( ImGui::IsItemHovered() ) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted( folders[i].path.c_str() );
                ImGui::EndTooltip();
            }

            ImGui::TextUnformatted( folders[i].name.c_str() );
            ImGui::NextColumn();
            ImGui::PopID();
        }

        // Textures
        for ( i32 i = 0; i < (i32)textures.size(); i++ ) {
            ImGui::PushID( (i32)folders.size() + i );

            ImTextureID texID = (ImTextureID)(intptr_t)( textures[i].texture ? textures[i].texture->GetHandle() : 0 );
            ImGui::Image( texID, thumbSize, uv0, uv1 );

            if ( ImGui::IsItemHovered() ) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted( textures[i].path.c_str() );
                if ( textures[i].texture ) {
                    ImGui::Text( "%dx%d", textures[i].texture->GetWidth(), textures[i].texture->GetHeight() );
                }
                ImGui::EndTooltip();
            }

            ImGui::TextUnformatted( textures[i].filename.c_str() );
            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns( 1 );

        ImGui::End();
    }

} // namespace atto
