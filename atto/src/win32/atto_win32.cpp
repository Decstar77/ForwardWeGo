#include "../engine/atto_assets.h"

#ifdef _WIN32

#include <windows.h>
#include <shobjidl.h>
#include <string>

namespace atto {
    inline std::string WStringToUtf8( const std::wstring & w ) {
        if ( w.empty() ) return {};

        int size = WideCharToMultiByte(
            CP_UTF8, 0,
            w.data(), (int)w.size(),
            nullptr, 0,
            nullptr, nullptr
        );

        std::string result( size, 0 );
        WideCharToMultiByte(
            CP_UTF8, 0,
            w.data(), (int)w.size(),
            result.data(), size,
            nullptr, nullptr
        );

        return result;
    }

    std::string AssetManager::OpenFilePicker( const std::string & basePath ) {
        HRESULT hr = CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED );
        if ( FAILED( hr ) )
            return "";

        IFileOpenDialog * dialog = nullptr;
        hr = CoCreateInstance( CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
            IID_PPV_ARGS( &dialog ) );
        if ( FAILED( hr ) ) {
            CoUninitialize();
            return "";
        }

        if ( !basePath.empty() ) {
            IShellItem * folder = nullptr;
            std::wstring wpath( basePath.begin(), basePath.end() );
            if ( SUCCEEDED( SHCreateItemFromParsingName( wpath.c_str(), nullptr, IID_PPV_ARGS( &folder ) ) ) ) {
                dialog->SetFolder( folder );
                folder->Release();
            }
        }

        std::string result;

        if ( SUCCEEDED( dialog->Show( nullptr ) ) ) {
            IShellItem * item = nullptr;
            if ( SUCCEEDED( dialog->GetResult( &item ) ) ) {
                PWSTR path = nullptr;
                if ( SUCCEEDED( item->GetDisplayName( SIGDN_FILESYSPATH, &path ) ) ) {
                    std::wstring wpath( path );
                    result = WStringToUtf8( wpath );
                    CoTaskMemFree( path );
                }
                item->Release();
            }
        }

        dialog->Release();
        CoUninitialize();
        return result;
    }

    static std::vector<std::wstring> SplitExtensions( const std::string & ext ) {
        std::vector<std::wstring> result;
        size_t start = 0;

        while ( true ) {
            size_t end = ext.find( ';', start );
            std::string token = ext.substr( start, end - start );

            if ( !token.empty() )
                result.emplace_back( token.begin(), token.end() );

            if ( end == std::string::npos )
                break;

            start = end + 1;
        }
        return result;
    }

    std::string AssetManager::SaveFilePicker( const std::string & basePath,
        const std::string & extensions ) {
        HRESULT hr = CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED );
        if ( FAILED( hr ) )
            return "";

        IFileSaveDialog * dialog = nullptr;
        hr = CoCreateInstance( CLSID_FileSaveDialog, nullptr, CLSCTX_ALL,
            IID_PPV_ARGS( &dialog ) );
        if ( FAILED( hr ) ) {
            CoUninitialize();
            return "";
        }

        // ---------- File type filters ----------
        std::vector<std::wstring> extList = SplitExtensions( extensions );

        std::vector<COMDLG_FILTERSPEC> filters;
        std::vector<std::wstring> patterns;

        for ( const auto & e : extList ) {
            patterns.push_back( L"*." + e );
            filters.push_back( { e.c_str(), patterns.back().c_str() } );
        }

        if ( !filters.empty() )
            dialog->SetFileTypes( (UINT)filters.size(), filters.data() );

        // ---------- Initial folder ----------
        if ( !basePath.empty() ) {
            std::wstring wpath( basePath.begin(), basePath.end() );
            IShellItem * folder = nullptr;
            if ( SUCCEEDED( SHCreateItemFromParsingName( wpath.c_str(), nullptr,
                IID_PPV_ARGS( &folder ) ) ) ) {
                dialog->SetFolder( folder );
                folder->Release();
            }
        }

        std::string result;

        if ( SUCCEEDED( dialog->Show( nullptr ) ) ) {
            IShellItem * item = nullptr;
            if ( SUCCEEDED( dialog->GetResult( &item ) ) ) {
                PWSTR path = nullptr;
                if ( SUCCEEDED( item->GetDisplayName( SIGDN_FILESYSPATH, &path ) ) ) {
                    std::wstring wpath( path );
                    result = WStringToUtf8( wpath );
                    // result.assign( wpath.begin(), wpath.end() );
                    CoTaskMemFree( path );
                }
                item->Release();
            }
        }

        dialog->Release();
        CoUninitialize();
        return result; // empty if cancelled
    }
}

#endif