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

        // Call each frame. Drives the active build phase.
        bool UpdatePacking();

        // Draw the progress/status popup. Call each frame during ImGui rendering.
        void DrawProgressPopup();

        bool IsPacking() const { return buildPhase != BuildPhase::Idle; }

    private:
        void FinalizePacking();
        void StartBuildThread();
        void DoCopyPhase();
        static std::string MakeTimestampDir();

        enum class BuildPhase {
            Idle,
            Packing,
            Building,   // cmake --build Shipping running in background thread
            Copying,    // copying binaries + game.bin to ship/TIMESTAMP/
            Done,
            Error
        };

        BuildPhase                  buildPhase   = BuildPhase::Idle;
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
