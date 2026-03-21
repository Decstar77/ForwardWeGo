#include "atto_assets.h"
#include "atto_engine.h"
#include "atto_log.h"

#include <json/json.hpp>
#include <fstream>
#include <filesystem>

namespace atto {

    void Serializer::OpStaticModel( const char * key, const StaticModel *& value ) {
        if ( IsSaving() ) {
            std::string path;
            if ( value ) {
                path = value->GetPath().GetCStr();
            }
            Op( key, path );
        }
        else {
            std::string path;
            Op( key, path );
            if ( !path.empty() ) {
                value = Engine::Get().GetRenderer().GetOrLoadStaticModel( path.c_str() );
            }
            else {
                value = nullptr;
            }
        }
    }

    struct JsonSerializer::JsonContainer {
        nlohmann::json json;
    };

    JsonSerializer::JsonSerializer( bool isSaving ) : Serializer( isSaving ) {
        jsonContainer = std::make_unique<JsonContainer>();
    }

    JsonSerializer::~JsonSerializer() = default;

    void JsonSerializer::Op( const char * key, i8 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value = jsonContainer->json[key].get<i8>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, u8 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value = jsonContainer->json[key].get<u8>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }
    void JsonSerializer::Op( const char * key, i32 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value = jsonContainer->json[key].get<i32>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, i64 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value = jsonContainer->json[key].get<i64>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, u32 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value = jsonContainer->json[key].get<u32>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, u64 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value = jsonContainer->json[key].get<u64>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, f32 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value = jsonContainer->json[key].get<f32>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, f64 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value = jsonContainer->json[key].get<f64>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, bool & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value = jsonContainer->json[key].get<bool>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, std::string & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value = jsonContainer->json[key].get<std::string>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, Vec2 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = { value.x, value.y };
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value.x = jsonContainer->json[key][0].get<f32>();
                value.y = jsonContainer->json[key][1].get<f32>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, Vec3 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = { value.x, value.y, value.z };
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value.x = jsonContainer->json[key][0].get<f32>();
                value.y = jsonContainer->json[key][1].get<f32>();
                value.z = jsonContainer->json[key][2].get<f32>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, Vec4 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = { value.x, value.y, value.z, value.w };
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value.x = jsonContainer->json[key][0].get<f32>();
                value.y = jsonContainer->json[key][1].get<f32>();
                value.z = jsonContainer->json[key][2].get<f32>();
                value.w = jsonContainer->json[key][3].get<f32>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, Mat2 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = { value[0][0], value[0][1], value[1][0], value[1][1] };
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value[0][0] = jsonContainer->json[key][0].get<f32>();
                value[0][1] = jsonContainer->json[key][1].get<f32>();
                value[1][0] = jsonContainer->json[key][2].get<f32>();
                value[1][1] = jsonContainer->json[key][3].get<f32>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, Mat3 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = {
                value[0][0], value[0][1], value[0][2],
                value[1][0], value[1][1], value[1][2],
                value[2][0], value[2][1], value[2][2]
            };
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value[0][0] = jsonContainer->json[key][0].get<f32>();
                value[0][1] = jsonContainer->json[key][1].get<f32>();
                value[0][2] = jsonContainer->json[key][2].get<f32>();
                value[1][0] = jsonContainer->json[key][3].get<f32>();
                value[1][1] = jsonContainer->json[key][4].get<f32>();
                value[1][2] = jsonContainer->json[key][5].get<f32>();
                value[2][0] = jsonContainer->json[key][6].get<f32>();
                value[2][1] = jsonContainer->json[key][7].get<f32>();
                value[2][2] = jsonContainer->json[key][8].get<f32>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    void JsonSerializer::Op( const char * key, Mat4 & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = {
                value[0][0], value[0][1], value[0][2], value[0][3],
                value[1][0], value[1][1], value[1][2], value[1][3],
                value[2][0], value[2][1], value[2][2], value[2][3],
                value[3][0], value[3][1], value[3][2], value[3][3]
            };
        }
        else {
            if ( jsonContainer->json.contains( key ) ) {
                value[0][0] = jsonContainer->json[key][0].get<f32>();
                value[0][1] = jsonContainer->json[key][1].get<f32>();
                value[0][2] = jsonContainer->json[key][2].get<f32>();
                value[0][3] = jsonContainer->json[key][3].get<f32>();
                value[1][0] = jsonContainer->json[key][4].get<f32>();
                value[1][1] = jsonContainer->json[key][5].get<f32>();
                value[1][2] = jsonContainer->json[key][6].get<f32>();
                value[1][3] = jsonContainer->json[key][7].get<f32>();
                value[2][0] = jsonContainer->json[key][8].get<f32>();
                value[2][1] = jsonContainer->json[key][9].get<f32>();
                value[2][2] = jsonContainer->json[key][10].get<f32>();
                value[2][3] = jsonContainer->json[key][11].get<f32>();
                value[3][0] = jsonContainer->json[key][12].get<f32>();
                value[3][1] = jsonContainer->json[key][13].get<f32>();
                value[3][2] = jsonContainer->json[key][14].get<f32>();
                value[3][3] = jsonContainer->json[key][15].get<f32>();
            }
            else {
                LOG_WARN( "Key %s not found in JSON", key );
            }
        }
    }

    std::unique_ptr<Serializer> JsonSerializer::CreateSubSerializer() {
        return std::make_unique<JsonSerializer>( isSaving );
    }

    void JsonSerializer::SetObject( const char * key, Serializer * serializer ) {
        JsonSerializer * jsonSerializer = static_cast<JsonSerializer *>(serializer);
        jsonContainer->json[key] = jsonSerializer->jsonContainer->json;
    }

    std::unique_ptr<Serializer> JsonSerializer::GetObject( const char * key ) {
        if ( !jsonContainer->json.contains( key ) ) {
            return nullptr;
        }
        auto sub = std::make_unique<JsonSerializer>( isSaving );
        sub->jsonContainer->json = jsonContainer->json[key];
        return sub;
    }

    void JsonSerializer::BeginArray( const char * key, i32 & count ) {
        if ( isSaving ) {
            // When saving, count is already set by the caller
            // Initialize the array as empty
            jsonContainer->json[key] = nlohmann::json::array();
        }
        else {
            // When loading, get the count from the JSON array
            if ( jsonContainer->json.contains( key ) && jsonContainer->json[key].is_array() ) {
                count = static_cast<i32>(jsonContainer->json[key].size());
            }
            else {
                count = 0;
            }
        }
    }

    void JsonSerializer::AppendArrayElement( const char * key, Serializer * serializer ) {
        JsonSerializer * jsonSerializer = static_cast<JsonSerializer *>(serializer);
        jsonContainer->json[key].push_back( jsonSerializer->jsonContainer->json );
    }

    std::unique_ptr<Serializer> JsonSerializer::GetArrayElement( const char * key, i32 index ) {
        if ( !jsonContainer->json.contains( key ) || !jsonContainer->json[key].is_array() ) {
            return nullptr;
        }
        if ( index < 0 || index >= static_cast<i32>( jsonContainer->json[key].size() ) ) {
            return nullptr;
        }
        auto sub = std::make_unique<JsonSerializer>( isSaving );
        sub->jsonContainer->json = jsonContainer->json[key][index];
        return sub;
    }

    void JsonSerializer::OpArrayPrimitive( const char * key, std::vector<i32> & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) && jsonContainer->json[key].is_array() ) {
                value.clear();
                for ( const auto & elem : jsonContainer->json[key] ) {
                    value.push_back( elem.get<i32>() );
                }
            }
        }
    }

    void JsonSerializer::OpArrayPrimitive( const char * key, std::vector<u64> & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) && jsonContainer->json[key].is_array() ) {
                value.clear();
                for ( const auto & elem : jsonContainer->json[key] ) {
                    value.push_back( elem.get<u64>() );
                }
            }
        }
    }

    void JsonSerializer::OpArrayPrimitive( const char * key, std::vector<f32> & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) && jsonContainer->json[key].is_array() ) {
                value.clear();
                for ( const auto & elem : jsonContainer->json[key] ) {
                    value.push_back( elem.get<f32>() );
                }
            }
        }
    }

    void JsonSerializer::OpArrayPrimitive( const char * key, std::vector<bool> & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = nlohmann::json::array();
            for ( bool b : value ) {
                jsonContainer->json[key].push_back( b );
            }
        }
        else {
            if ( jsonContainer->json.contains( key ) && jsonContainer->json[key].is_array() ) {
                value.clear();
                for ( const auto & elem : jsonContainer->json[key] ) {
                    value.push_back( elem.get<bool>() );
                }
            }
        }
    }

    void JsonSerializer::OpArrayPrimitive( const char * key, std::vector<std::string> & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = value;
        }
        else {
            if ( jsonContainer->json.contains( key ) && jsonContainer->json[key].is_array() ) {
                value.clear();
                for ( const auto & elem : jsonContainer->json[key] ) {
                    value.push_back( elem.get<std::string>() );
                }
            }
        }
    }

    void JsonSerializer::OpArrayPrimitive( const char * key, std::vector<Vec2> & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = nlohmann::json::array();
            for ( const Vec2 & v : value ) {
                jsonContainer->json[key].push_back( { v.x, v.y } );
            }
        }
        else {
            if ( jsonContainer->json.contains( key ) && jsonContainer->json[key].is_array() ) {
                value.clear();
                for ( const auto & elem : jsonContainer->json[key] ) {
                    Vec2 v;
                    v.x = elem[0].get<f32>();
                    v.y = elem[1].get<f32>();
                    value.push_back( v );
                }
            }
        }
    }

    void JsonSerializer::OpArrayPrimitive( const char * key, std::vector<Vec3> & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = nlohmann::json::array();
            for ( const Vec3 & v : value ) {
                jsonContainer->json[key].push_back( { v.x, v.y, v.z } );
            }
        }
        else {
            if ( jsonContainer->json.contains( key ) && jsonContainer->json[key].is_array() ) {
                value.clear();
                for ( const auto & elem : jsonContainer->json[key] ) {
                    Vec3 v;
                    v.x = elem[0].get<f32>();
                    v.y = elem[1].get<f32>();
                    v.z = elem[2].get<f32>();
                    value.push_back( v );
                }
            }
        }
    }

    void JsonSerializer::OpArrayPrimitive( const char * key, std::vector<Vec4> & value ) {
        if ( isSaving ) {
            jsonContainer->json[key] = nlohmann::json::array();
            for ( const Vec4 & v : value ) {
                jsonContainer->json[key].push_back( { v.x, v.y, v.z, v.w } );
            }
        }
        else {
            if ( jsonContainer->json.contains( key ) && jsonContainer->json[key].is_array() ) {
                value.clear();
                for ( const auto & elem : jsonContainer->json[key] ) {
                    Vec4 v;
                    v.x = elem[0].get<f32>();
                    v.y = elem[1].get<f32>();
                    v.z = elem[2].get<f32>();
                    v.w = elem[3].get<f32>();
                    value.push_back( v );
                }
            }
        }
    }

    std::string JsonSerializer::ToString() const {
        return jsonContainer->json.dump( 4, ' ', true, nlohmann::json::error_handler_t::strict );
    }

    void JsonSerializer::FromString( const std::string & json ) {
        jsonContainer->json = nlohmann::json::parse( json );
    }

    AssetManager::AssetManager() {
    }

    AssetManager::~AssetManager() {
    }

    bool AssetManager::Initialize() {
        return true;
    }

    void AssetManager::Shutdown() {
    }

    void AssetManager::WriteTextFile( const std::string & path, const std::string & content ) {
        std::ofstream file( path.c_str() );
        if ( file ) {
            LOG_INFO( "Writing file %s", path.c_str() );
            file << content;
            file.close();
        }
        else {
            LOG_ERROR( "Failed to read text file: %s", path.c_str() );
        }
    }

    std::string AssetManager::ReadTextFile( const std::string & path ) {
        std::ifstream file( path.c_str() );
        std::string content = "";
        if ( file.is_open() ) {
            std::string line;
            while ( std::getline( file, line ) ) {
                content += line + "\n";
            }
            file.close();
            return content;
        }
        LOG_ERROR( "Failed to read text file: %s", path.c_str() );
        return "";
    }

    std::vector< std::string > AssetManager::GetFilesInFolderRecursive( const char * path, const char * ext ) {
        std::vector<std::string> files;
        const std::filesystem::path currentPath = std::filesystem::path( path );

        for ( const auto & entry : std::filesystem::recursive_directory_iterator( currentPath ) ) {
            if ( entry.is_regular_file() && entry.path().extension() == ext ) {
                files.push_back( entry.path().string() );
            }
        }

        return files;
    }
}