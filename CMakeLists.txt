cmake_minimum_required(VERSION 3.22)
project(atto LANGUAGES C CXX)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT atto)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------------------------------------------------------
# Vendor paths
# ---------------------------------------------------------
set(GLFW_DIR   ${CMAKE_SOURCE_DIR}/vendor/glfw)
set(GLAD_DIR   ${CMAKE_SOURCE_DIR}/vendor/glad)
set(CLIPPER_DIR ${CMAKE_SOURCE_DIR}/vendor/clipper2/CPP/Clipper2Lib)

# ---------------------------------------------------------
# assimp (built from source)
# ---------------------------------------------------------
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/assimp)

# ---------------------------------------------------------
# clipper (static lib, C++17)
# ---------------------------------------------------------
add_library(clipper STATIC
    ${CLIPPER_DIR}/src/clipper.engine.cpp
    ${CLIPPER_DIR}/src/clipper.offset.cpp
    ${CLIPPER_DIR}/src/clipper.rectclip.cpp
    ${CLIPPER_DIR}/src/clipper.triangulation.cpp
)
target_include_directories(clipper PUBLIC ${CLIPPER_DIR}/include)
set_target_properties(clipper PROPERTIES CXX_STANDARD 17)
if(MSVC)
    target_compile_definitions(clipper PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ---------------------------------------------------------
# glad (static lib, C)
# ---------------------------------------------------------
add_library(glad STATIC
    ${GLAD_DIR}/src/glad.c
)
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)
if(MSVC)
    target_compile_definitions(glad PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ---------------------------------------------------------
# glfw (static lib, C)
# ---------------------------------------------------------
set(GLFW_SOURCES
    ${GLFW_DIR}/src/context.c
    ${GLFW_DIR}/src/egl_context.c
    ${GLFW_DIR}/src/init.c
    ${GLFW_DIR}/src/input.c
    ${GLFW_DIR}/src/monitor.c
    ${GLFW_DIR}/src/null_init.c
    ${GLFW_DIR}/src/null_joystick.c
    ${GLFW_DIR}/src/null_monitor.c
    ${GLFW_DIR}/src/null_window.c
    ${GLFW_DIR}/src/osmesa_context.c
    ${GLFW_DIR}/src/platform.c
    ${GLFW_DIR}/src/vulkan.c
    ${GLFW_DIR}/src/window.c
)

if(WIN32)
    list(APPEND GLFW_SOURCES
        ${GLFW_DIR}/src/win32_init.c
        ${GLFW_DIR}/src/win32_joystick.c
        ${GLFW_DIR}/src/win32_module.c
        ${GLFW_DIR}/src/win32_monitor.c
        ${GLFW_DIR}/src/win32_thread.c
        ${GLFW_DIR}/src/win32_time.c
        ${GLFW_DIR}/src/win32_window.c
        ${GLFW_DIR}/src/wgl_context.c
    )
endif()

add_library(glfw STATIC ${GLFW_SOURCES})
target_include_directories(glfw PUBLIC ${GLFW_DIR}/include)
if(WIN32)
    target_compile_definitions(glfw PRIVATE _GLFW_WIN32)
endif()
if(MSVC)
    target_compile_definitions(glfw PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ---------------------------------------------------------
# atto (executable, C++20)
# ---------------------------------------------------------
file(GLOB_RECURSE ATTO_SOURCES
    atto/src/*.h
    atto/src/*.hpp
    atto/src/*.c
    atto/src/*.cpp
)

set(VENDOR_SOURCES
    vendor/stb/stb_vorbis/stb_vorbis.c
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_impl_glfw.cpp
    vendor/imgui/imgui_impl_opengl3.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui-gizmo/ImGuizmo.cpp
)

add_executable(atto ${ATTO_SOURCES} ${VENDOR_SOURCES})

# pocketlzma uses try/catch internally, enable exceptions and suppress vendor warnings for this file
if(MSVC)
    set_source_files_properties(atto/src/engine/atto_assets_packed.cpp PROPERTIES
        COMPILE_OPTIONS "/EHsc"
    )
    set_source_files_properties(atto/src/editor/editor_asset_packer.cpp PROPERTIES
        COMPILE_OPTIONS "/EHsc"
    )
else()
    set_source_files_properties(atto/src/engine/atto_assets_packed.cpp PROPERTIES
        COMPILE_OPTIONS "-fexceptions;-Wno-dangling-pointer;-Wno-sign-compare"
    )
    set_source_files_properties(atto/src/editor/editor_asset_packer.cpp PROPERTIES
            COMPILE_OPTIONS "-fexceptions;-Wno-dangling-pointer;-Wno-sign-compare"
    )
endif()

target_include_directories(atto PRIVATE
    vendor/stb
    vendor/pocketlzma
    vendor/glm
    vendor/json
    vendor/audio
    vendor/imgui
    vendor/imgui-gizmo
    ${CLIPPER_DIR}/include
    vendor/fpm
    vendor/enet/include
    atto/src
    vendor/assimp/include
    ${CMAKE_BINARY_DIR}/vendor/assimp/include   # for generated assimp/config.h
)

# C++20, no exceptions, no RTTI
set_target_properties(atto PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
)

if(MSVC)
    target_compile_options(atto PRIVATE
        /MP             # Multi-processor compilation
        /EHsc-          # Exceptions off
        /GR-            # RTTI off
        /W3             # Warning level 3
        /wd4100         # Unreferenced formal parameter
        /wd4201         # Nameless struct/union
        /wd4702         # Unreachable code
    )
else()
    target_compile_options(atto PRIVATE
        -fno-exceptions
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
        -Wall
        -Wno-unused-parameter
        -Wno-unused-local-typedefs
    )
endif()

# Configuration-specific defines
target_compile_definitions(atto PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:_RELEASE>
)

# Link vendor static libs (assimp built from source as static lib)
target_link_libraries(atto PRIVATE clipper glad glfw assimp)

# ---------------------------------------------------------
# Windows platform specifics
# ---------------------------------------------------------
if(WIN32)
    target_include_directories(atto PRIVATE
        ${GLFW_DIR}/include
        ${GLAD_DIR}/include
        vendor/openal/include
    )

    target_link_directories(atto PRIVATE
        vendor/openal/lib
    )

    target_link_libraries(atto PRIVATE
        opengl32
        openal32
        kernel32
        user32
        Dbghelp
        ws2_32
        winmm
    )

    # Copy OpenAL DLL to output directory after build
    add_custom_command(TARGET atto POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/vendor/openal/lib/OpenAL32.dll"
            "$<TARGET_FILE_DIR:atto>"
    )
endif()
