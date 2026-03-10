workspace "atto"
    location "build"
    configurations { "Debug", "Release" }
    platforms { "x64", "Emscripten" }

    filter "platforms:x64"
        system "windows"
        architecture "x64"

    filter "platforms:Emscripten"
        system "emscripten"
        architecture "wasm32"

    filter "configurations:Debug"
        defines "_DEBUG"
        symbols "On"

    filter "configurations:Release"
        defines "_RELEASE"
        optimize "Full"

project "atto"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    exceptionhandling "Off"
    rtti "Off"
    warnings "Default"
    flags { "MultiProcessorCompile" }

    files {
        "atto/src/**.h",
        "atto/src/**.hpp",
        "atto/src/**.c",
        "atto/src/**.cpp",
        "vendor/stb/stb_vorbis/stb_vorbis.c",
        "vendor/imgui/imgui.cpp",
        "vendor/imgui/imgui_demo.cpp",
        "vendor/imgui/imgui_draw.cpp",
        "vendor/imgui/imgui_impl_glfw.cpp",
        "vendor/imgui/imgui_impl_opengl3.cpp",
        "vendor/imgui/imgui_tables.cpp",
        "vendor/imgui/imgui_widgets.cpp",
    }

    includedirs {
        "vendor/stb",
        "vendor/glm",
        "vendor/json",
        "vendor/audio",
        "vendor/imgui",
        "vendor/clipper2/CPP/Clipper2Lib/include",
        "vendor/fpm",
        "vendor/enet/include",
        "atto/src",
        "vendor/assimp/include"
    }

    links {
        "clipper"
    }

    ---------------------------------------------------------
    -- WINDOWS
    ---------------------------------------------------------
    filter "system:windows"
        includedirs {
            "vendor/glfw/include",
            "vendor/glad/include",
            "vendor/openal/include"
        }

        libdirs {
            "vendor/openal/lib",
            "vendor/assimp/lib"
        }

        links {
            "opengl32",
            "glfw",
            "glad",
            "openal32",
            "kernel32",
            "user32",
            "Dbghelp",
            "ws2_32",
            "winmm",
            "assimp-vc143-mt"
        }


local GLFW_DIR = "vendor/glfw"
local GLAD_DIR = "vendor/glad"
local CLIPPER_DIR = "vendor/clipper2/CPP/Clipper2Lib"

project "clipper"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    files {
        path.join(CLIPPER_DIR, "include/clipper.h"),
        path.join(CLIPPER_DIR, "include/clipper.core.h"),
        path.join(CLIPPER_DIR, "include/clipper.engine.h"),
        path.join(CLIPPER_DIR, "include/clipper.export.h"),
        path.join(CLIPPER_DIR, "include/clipper.minkowski.h"),
        path.join(CLIPPER_DIR, "include/clipper.offset.h"),
        path.join(CLIPPER_DIR, "include/clipper.rectclip.h"),
        path.join(CLIPPER_DIR, "include/clipper.triangulation.h"),
        path.join(CLIPPER_DIR, "src/clipper.engine.cpp"),
        path.join(CLIPPER_DIR, "src/clipper.offset.cpp"),
        path.join(CLIPPER_DIR, "src/clipper.rectclip.cpp"),
        path.join(CLIPPER_DIR, "src/clipper.triangulation.cpp"),
    }
    includedirs { 
        path.join(CLIPPER_DIR, "include") 
    }

    filter "action:vs*"
        defines "_CRT_SECURE_NO_WARNINGS"


project "glad"
    kind "StaticLib"
    language "C"
    files {
        path.join(GLAD_DIR, "inlcude/glad/glad.h"),
        path.join(GLAD_DIR, "inlcude/KHR/khrplatform.h"),
        path.join(GLAD_DIR, "src/glad.c"),
    }
    includedirs { 
        path.join(GLAD_DIR, "include") 
    }

    filter "action:vs*"
        defines "_CRT_SECURE_NO_WARNINGS"

project "glfw"
    kind "StaticLib"
    language "C"
    files
    {
        path.join(GLFW_DIR, "include/GLFW/*.h"),
        path.join(GLFW_DIR, "src/context.c"),
        path.join(GLFW_DIR, "src/egl_context.*"),
        path.join(GLFW_DIR, "src/init.c"),
        path.join(GLFW_DIR, "src/input.c"),
        path.join(GLFW_DIR, "src/internal.h"),
        path.join(GLFW_DIR, "src/monitor.c"),
        path.join(GLFW_DIR, "src/null*.*"),
        path.join(GLFW_DIR, "src/osmesa_context.*"),
        path.join(GLFW_DIR, "src/platform.c"),
        path.join(GLFW_DIR, "src/vulkan.c"),
        path.join(GLFW_DIR, "src/window.c"),
    }
    includedirs { path.join(GLFW_DIR, "include") }
    filter "system:windows"
        defines "_GLFW_WIN32"
        files
        {
            path.join(GLFW_DIR, "src/win32_*.*"),
            path.join(GLFW_DIR, "src/wgl_context.*")
        }

    filter "action:vs*"
        defines "_CRT_SECURE_NO_WARNINGS"
        