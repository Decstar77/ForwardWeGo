#include "editor_property_serializer.h"
#include "editor_asset_browser.h"

#include <imgui.h>
#include <cstdio>

namespace atto {

    ImguiPropertySerializer::ImguiPropertySerializer()
        : Serializer( true ) {
    }

    ImguiPropertySerializer::~ImguiPropertySerializer() {
        if ( pushedImguiId ) {
            ImGui::PopID();
        }
    }

    // ============================================================
    // Primitive Ops
    // ============================================================

    void ImguiPropertySerializer::Op( const char * key, i8 & value ) {
        i32 v = value;
        if ( ImGui::DragInt( key, &v, 1.0f, -128, 127 ) ) {
            value = static_cast<i8>( v );
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, u8 & value ) {
        i32 v = value;
        if ( ImGui::DragInt( key, &v, 1.0f, 0, 255 ) ) {
            value = static_cast<u8>( v );
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, i32 & value ) {
        if ( ImGui::DragInt( key, &value ) ) {
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, i64 & value ) {
        if ( ImGui::DragScalar( key, ImGuiDataType_S64, &value ) ) {
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, u32 & value ) {
        if ( ImGui::DragScalar( key, ImGuiDataType_U32, &value ) ) {
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, u64 & value ) {
        if ( ImGui::DragScalar( key, ImGuiDataType_U64, &value ) ) {
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, f32 & value ) {
        if ( ImGui::DragFloat( key, &value, 0.1f ) ) {
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, f64 & value ) {
        f32 speed = 0.1f;
        if ( ImGui::DragScalar( key, ImGuiDataType_Double, &value, speed ) ) {
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, bool & value ) {
        if ( ImGui::Checkbox( key, &value ) ) {
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, std::string & value ) {
        char buffer[256];
        snprintf( buffer, sizeof( buffer ), "%s", value.c_str() );
        if ( ImGui::InputText( key, buffer, sizeof( buffer ) ) ) {
            value = buffer;
            changed = true;
        }
    }

    // ============================================================
    // Vector / Matrix Ops
    // ============================================================

    void ImguiPropertySerializer::Op( const char * key, Vec2 & value ) {
        if ( ImGui::DragFloat2( key, &value.x, 0.1f ) ) {
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, Vec3 & value ) {
        if ( ImGui::DragFloat3( key, &value.x, 0.1f ) ) {
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, Vec4 & value ) {
        if ( ImGui::DragFloat4( key, &value.x, 0.1f ) ) {
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, Mat2 & value ) {
        if ( ImGui::TreeNodeEx( key, ImGuiTreeNodeFlags_DefaultOpen ) ) {
            changed |= ImGui::DragFloat2( "Row 0", &value[0].x, 0.1f );
            changed |= ImGui::DragFloat2( "Row 1", &value[1].x, 0.1f );
            ImGui::TreePop();
        }
    }

    void ImguiPropertySerializer::Op( const char * key, Mat3 & value ) {
        if ( ImGui::TreeNodeEx( key, ImGuiTreeNodeFlags_DefaultOpen ) ) {
            changed |= ImGui::DragFloat3( "Row 0", &value[0].x, 0.1f );
            changed |= ImGui::DragFloat3( "Row 1", &value[1].x, 0.1f );
            changed |= ImGui::DragFloat3( "Row 2", &value[2].x, 0.1f );
            ImGui::TreePop();
        }
    }

    void ImguiPropertySerializer::Op( const char * key, Mat4 & value ) {
        if ( ImGui::TreeNodeEx( key, ImGuiTreeNodeFlags_DefaultOpen ) ) {
            changed |= ImGui::DragFloat4( "Row 0", &value[0].x, 0.1f );
            changed |= ImGui::DragFloat4( "Row 1", &value[1].x, 0.1f );
            changed |= ImGui::DragFloat4( "Row 2", &value[2].x, 0.1f );
            changed |= ImGui::DragFloat4( "Row 3", &value[3].x, 0.1f );
            ImGui::TreePop();
        }
    }

    // ============================================================
    // Object serialization
    // ============================================================

    std::unique_ptr<Serializer> ImguiPropertySerializer::CreateSubSerializer() {
        auto sub = std::make_unique<ImguiPropertySerializer>();
        ImGui::PushID( nextSubId++ );
        sub->pushedImguiId = true;
        return sub;
    }

    void ImguiPropertySerializer::SetObject( const char * key, Serializer * serializer ) {
        ImguiPropertySerializer * sub = static_cast<ImguiPropertySerializer *>( serializer );
        changed |= sub->changed;
    }

    std::unique_ptr<Serializer> ImguiPropertySerializer::GetObject( const char * key ) {
        auto sub = std::make_unique<ImguiPropertySerializer>();
        ImGui::PushID( nextSubId++ );
        sub->pushedImguiId = true;
        return sub;
    }

    // ============================================================
    // Array serialization
    // ============================================================

    void ImguiPropertySerializer::BeginArray( const char * key, i32 & count ) {
        // count is already set by the caller in saving mode
    }

    void ImguiPropertySerializer::AppendArrayElement( const char * key, Serializer * serializer ) {
        ImguiPropertySerializer * sub = static_cast<ImguiPropertySerializer *>( serializer );
        changed |= sub->changed;
    }

    std::unique_ptr<Serializer> ImguiPropertySerializer::GetArrayElement( const char * key, i32 index ) {
        auto sub = std::make_unique<ImguiPropertySerializer>();
        ImGui::PushID( nextSubId++ );
        sub->pushedImguiId = true;
        return sub;
    }

    // ============================================================
    // Array primitive operations
    // ============================================================

    void ImguiPropertySerializer::OpArrayPrimitive( const char * key, std::vector<i32> & value ) {
        bool open = ImGui::TreeNode( key );
        ImGui::SameLine();
        if ( ImGui::SmallButton( "+" ) ) {
            value.push_back( 0 );
            changed = true;
        }
        if ( open ) {
            i32 removeIndex = -1;
            for ( i32 i = 0; i < static_cast<i32>( value.size() ); i++ ) {
                ImGui::PushID( i );
                if ( ImGui::SmallButton( "X" ) ) { removeIndex = i; }
                ImGui::SameLine();
                if ( ImGui::DragInt( "##v", &value[i] ) ) { changed = true; }
                ImGui::PopID();
            }
            if ( removeIndex >= 0 ) {
                value.erase( value.begin() + removeIndex );
                changed = true;
            }
            ImGui::TreePop();
        }
    }

    void ImguiPropertySerializer::OpArrayPrimitive( const char * key, std::vector<u64> & value ) {
        bool open = ImGui::TreeNode( key );
        ImGui::SameLine();
        if ( ImGui::SmallButton( "+" ) ) {
            value.push_back( 0 );
            changed = true;
        }
        if ( open ) {
            i32 removeIndex = -1;
            for ( i32 i = 0; i < static_cast<i32>( value.size() ); i++ ) {
                ImGui::PushID( i );
                if ( ImGui::SmallButton( "X" ) ) { removeIndex = i; }
                ImGui::SameLine();
                if ( ImGui::DragScalar( "##v", ImGuiDataType_U64, &value[i] ) ) { changed = true; }
                ImGui::PopID();
            }
            if ( removeIndex >= 0 ) {
                value.erase( value.begin() + removeIndex );
                changed = true;
            }
            ImGui::TreePop();
        }
    }

    void ImguiPropertySerializer::OpArrayPrimitive( const char * key, std::vector<f32> & value ) {
        bool open = ImGui::TreeNode( key );
        ImGui::SameLine();
        if ( ImGui::SmallButton( "+" ) ) {
            value.push_back( 0.0f );
            changed = true;
        }
        if ( open ) {
            i32 removeIndex = -1;
            for ( i32 i = 0; i < static_cast<i32>( value.size() ); i++ ) {
                ImGui::PushID( i );
                if ( ImGui::SmallButton( "X" ) ) { removeIndex = i; }
                ImGui::SameLine();
                if ( ImGui::DragFloat( "##v", &value[i], 0.1f ) ) { changed = true; }
                ImGui::PopID();
            }
            if ( removeIndex >= 0 ) {
                value.erase( value.begin() + removeIndex );
                changed = true;
            }
            ImGui::TreePop();
        }
    }

    void ImguiPropertySerializer::OpArrayPrimitive( const char * key, std::vector<bool> & value ) {
        bool open = ImGui::TreeNode( key );
        ImGui::SameLine();
        if ( ImGui::SmallButton( "+" ) ) {
            value.push_back( false );
            changed = true;
        }
        if ( open ) {
            i32 removeIndex = -1;
            for ( i32 i = 0; i < static_cast<i32>( value.size() ); i++ ) {
                ImGui::PushID( i );
                if ( ImGui::SmallButton( "X" ) ) { removeIndex = i; }
                ImGui::SameLine();
                bool v = value[i];
                if ( ImGui::Checkbox( "##v", &v ) ) {
                    value[i] = v;
                    changed = true;
                }
                ImGui::PopID();
            }
            if ( removeIndex >= 0 ) {
                value.erase( value.begin() + removeIndex );
                changed = true;
            }
            ImGui::TreePop();
        }
    }

    void ImguiPropertySerializer::OpArrayPrimitive( const char * key, std::vector<std::string> & value ) {
        bool open = ImGui::TreeNode( key );
        ImGui::SameLine();
        if ( ImGui::SmallButton( "+" ) ) {
            value.push_back( "" );
            changed = true;
        }
        if ( open ) {
            i32 removeIndex = -1;
            for ( i32 i = 0; i < static_cast<i32>( value.size() ); i++ ) {
                ImGui::PushID( i );
                if ( ImGui::SmallButton( "X" ) ) { removeIndex = i; }
                ImGui::SameLine();
                char buffer[256];
                snprintf( buffer, sizeof( buffer ), "%s", value[i].c_str() );
                if ( ImGui::InputText( "##v", buffer, sizeof( buffer ) ) ) {
                    value[i] = buffer;
                    changed = true;
                }
                ImGui::PopID();
            }
            if ( removeIndex >= 0 ) {
                value.erase( value.begin() + removeIndex );
                changed = true;
            }
            ImGui::TreePop();
        }
    }

    void ImguiPropertySerializer::OpArrayPrimitive( const char * key, std::vector<Vec2> & value ) {
        bool open = ImGui::TreeNode( key );
        ImGui::SameLine();
        if ( ImGui::SmallButton( "+" ) ) {
            value.push_back( Vec2( 0.0f ) );
            changed = true;
        }
        if ( open ) {
            i32 removeIndex = -1;
            for ( i32 i = 0; i < static_cast<i32>( value.size() ); i++ ) {
                ImGui::PushID( i );
                if ( ImGui::SmallButton( "X" ) ) { removeIndex = i; }
                ImGui::SameLine();
                if ( ImGui::DragFloat2( "##v", &value[i].x, 0.1f ) ) { changed = true; }
                ImGui::PopID();
            }
            if ( removeIndex >= 0 ) {
                value.erase( value.begin() + removeIndex );
                changed = true;
            }
            ImGui::TreePop();
        }
    }

    void ImguiPropertySerializer::OpArrayPrimitive( const char * key, std::vector<Vec3> & value ) {
        bool open = ImGui::TreeNode( key );
        ImGui::SameLine();
        if ( ImGui::SmallButton( "+" ) ) {
            value.push_back( Vec3( 0.0f ) );
            changed = true;
        }
        if ( open ) {
            i32 removeIndex = -1;
            for ( i32 i = 0; i < static_cast<i32>( value.size() ); i++ ) {
                ImGui::PushID( i );
                if ( ImGui::SmallButton( "X" ) ) { removeIndex = i; }
                ImGui::SameLine();
                if ( ImGui::DragFloat3( "##v", &value[i].x, 0.1f ) ) { changed = true; }
                ImGui::PopID();
            }
            if ( removeIndex >= 0 ) {
                value.erase( value.begin() + removeIndex );
                changed = true;
            }
            ImGui::TreePop();
        }
    }

    void ImguiPropertySerializer::OpArrayPrimitive( const char * key, std::vector<Vec4> & value ) {
        bool open = ImGui::TreeNode( key );
        ImGui::SameLine();
        if ( ImGui::SmallButton( "+" ) ) {
            value.push_back( Vec4( 0.0f ) );
            changed = true;
        }
        if ( open ) {
            i32 removeIndex = -1;
            for ( i32 i = 0; i < static_cast<i32>( value.size() ); i++ ) {
                ImGui::PushID( i );
                if ( ImGui::SmallButton( "X" ) ) { removeIndex = i; }
                ImGui::SameLine();
                if ( ImGui::DragFloat4( "##v", &value[i].x, 0.1f ) ) { changed = true; }
                ImGui::PopID();
            }
            if ( removeIndex >= 0 ) {
                value.erase( value.begin() + removeIndex );
                changed = true;
            }
            ImGui::TreePop();
        }
    }

    void ImguiPropertySerializer::Op( const char * key, SmallString & value ) {
        char buffer[SmallString::CAPCITY];
        snprintf( buffer, sizeof( buffer ), "%s", value.GetCStr() );
        if ( ImGui::InputText( key, buffer, sizeof( buffer ) ) ) {
            value = buffer;
            changed = true;
        }
    }

    void ImguiPropertySerializer::Op( const char * key, LargeString & value ) {
        char buffer[LargeString::CAPCITY];
        snprintf( buffer, sizeof( buffer ), "%s", value.GetCStr() );
        if ( ImGui::InputText( key, buffer, sizeof( buffer ) ) ) {
            value = buffer;
            changed = true;
        }
    }

    // ============================================================
    // Special-case: StaticModel pointer
    // ============================================================

    void ImguiPropertySerializer::OpStaticModel( const char * key, const StaticModel *& value ) {
        ImGui::PushID( key );

        // Show the current model path
        const char * currentPath = value ? value->GetPath().GetCStr() : "<none>";
        ImGui::Text( "%s: %s", key, currentPath );

        // Check if the asset browser has a pending model selection
        if ( assetBrowser && assetBrowser->modelSelectionMade ) {
            std::string modelPath = assetBrowser->selectedModelPath;
            for ( char & ch : modelPath ) {
                if ( ch == '\\' ) ch = '/';
            }
            value = Engine::Get().GetRenderer().GetOrLoadStaticModel( modelPath.c_str() );
            assetBrowser->modelSelectionMade = false;
            assetBrowser->selectingForModel  = false;
            changed = true;
        }

        // Browse button to open the asset browser on the Models tab
        if ( assetBrowser ) {
            if ( ImGui::Button( "Browse Models..." ) ) {
                assetBrowser->isOpen          = true;
                assetBrowser->selectingForModel = true;
            }
            ImGui::SameLine();
        }

        // Clear button
        if ( value ) {
            if ( ImGui::Button( "Clear" ) ) {
                value = nullptr;
                changed = true;
            }
        }

        ImGui::PopID();
    }

}
