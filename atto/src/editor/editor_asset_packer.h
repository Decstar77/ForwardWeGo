#pragma once

#include "../engine/atto_core.h"
#if ATTO_EDITOR

#include "engine/atto_engine.h"

#include <atomic>
#include <thread>

namespace atto {

    class EditorAssetPacker {
    public:
        static std::vector<std::string> ScrapeAssets();

        // Call from menu to kick off the full build pipeline (pack -> cmake -> copy).
        void BeginPacking();

        // Call from menu to kick off the web build pipeline (pack -> emcmake -> copy).
        void BeginPackingWeb();

        // Call each frame. Drives the active build phase.
        bool UpdatePacking();

        // Draw the progress/status popup. Call each frame during ImGui rendering.
        void DrawProgressPopup();

        bool IsPacking() const { return buildPhase != BuildPhase::Idle; }

    private:
        void FinalizePacking();
        void StartBuildThread();
        void StartWebBuildThread();
        void DoCopyPhase();
        void DoWebCopyPhase();
        static std::string MakeTimestampDir();

        enum class BuildPhase {
            Idle,
            Packing,
            Building,   // cmake --build running in background thread
            Copying,    // copying binaries + game.bin to ship/TIMESTAMP/
            Done,
            Error
        };

        enum class BuildTarget {
            Windows,
            Web
        };

        BuildPhase                  buildPhase   = BuildPhase::Idle;
        BuildTarget                 buildTarget  = BuildTarget::Windows;
        i32                         currentIndex = 0;
        std::vector<std::string>    assetPaths;
        std::vector<PackedAssetData> packedAssets;
        u64                         dataOffset   = 0;

        std::thread                 buildThread;
        std::atomic<bool>           buildFinished { false };
        std::atomic<int>            buildExitCode { 0 };
        std::string                 buildOutput;    // written by thread before buildFinished = true
        std::string                 shipDir;
        std::string                 errorMessage;
    };
}

#endif
