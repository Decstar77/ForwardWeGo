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
# Shipping configuration
# Inherits Release flags; defines ATTO_SHIPPING; excludes
# raw-asset and editor libraries from compilation & linking.
# ---------------------------------------------------------
if(NOT EMSCRIPTEN)
    get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(IS_MULTI_CONFIG)
        list(APPEND CMAKE_CONFIGURATION_TYPES Shipping)
        list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "" FORCE)
    endif()

    # Shipping borrows all compiler/linker flags from Release
    set(CMAKE_CXX_FLAGS_SHIPPING          "${CMAKE_CXX_FLAGS_RELEASE}"          CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_SHIPPING            "${CMAKE_C_FLAGS_RELEASE}"            CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_SHIPPING   "${CMAKE_EXE_LINKER_FLAGS_RELEASE}"   CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_SHIPPING "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)
    mark_as_advanced(
        CMAKE_CXX_FLAGS_SHIPPING
        CMAKE_C_FLAGS_SHIPPING
        CMAKE_EXE_LINKER_FLAGS_SHIPPING
        CMAKE_SHARED_LINKER_FLAGS_SHIPPING
    )
endif()

# ---------------------------------------------------------
# Vendor paths
# ---------------------------------------------------------
set(CLIPPER_DIR ${CMAKE_SOURCE_DIR}/vendor/clipper2/CPP/Clipper2Lib)

if(NOT EMSCRIPTEN)
    set(GLFW_DIR    ${CMAKE_SOURCE_DIR}/vendor/glfw)
    set(GLAD_DIR    ${CMAKE_SOURCE_DIR}/vendor/glad)
endif()

# ---------------------------------------------------------
# assimp (built from source — not linked in Shipping/Emscripten,
# but always configured for multi-config generator compatibility)
# ---------------------------------------------------------
if(NOT EMSCRIPTEN)
    set(BUILD_SHARED_LIBS             OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TESTS            OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS     OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SAMPLES          OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL                OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_DOCS             OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ZLIB             ON  CACHE BOOL "" FORCE)
    set(ASSIMP_NO_EXPORT              ON  CACHE BOOL "" FORCE)
    set(ASSIMP_WARNINGS_AS_ERRORS     OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INJECT_DEBUG_POSTFIX   OFF CACHE BOOL "" FORCE)
    add_subdirectory(vendor/assimp)
endif()

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
# glad / glfw (skipped for Emscripten — uses its own GL + GLFW)
# ---------------------------------------------------------
if(NOT EMSCRIPTEN)
    # glad (static lib, C)
    add_library(glad STATIC
        ${GLAD_DIR}/src/glad.c
    )
    target_include_directories(glad PUBLIC ${GLAD_DIR}/include)
    if(MSVC)
        target_compile_definitions(glad PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    # glfw (static lib, C)
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
endif()

# ---------------------------------------------------------
# atto (executable, C++20)
# ---------------------------------------------------------

# Core sources — always compiled in every configuration.
# Editor and raw-asset sources are handled separately below.
file(GLOB_RECURSE ATTO_ALL_SOURCES
    atto/src/*.h
    atto/src/*.hpp
    atto/src/*.c
    atto/src/*.cpp
)

file(GLOB_RECURSE ATTO_EDITOR_SOURCES
    atto/src/editor/*.h
    atto/src/editor/*.hpp
    atto/src/editor/*.c
    atto/src/editor/*.cpp
)

# Raw-asset loader (pulls in Assimp / stb_image / stb_vorbis / AudioFile)
set(ATTO_RAW_SOURCES
    ${CMAKE_SOURCE_DIR}/atto/src/engine/atto_assets_raw.cpp
)

# Win32 platform sources
file(GLOB_RECURSE ATTO_WIN32_SOURCES
    atto/src/win32/*.h
    atto/src/win32/*.hpp
    atto/src/win32/*.c
    atto/src/win32/*.cpp
)

# Remove editor and raw sources from the always-compiled set
list(REMOVE_ITEM ATTO_ALL_SOURCES ${ATTO_EDITOR_SOURCES})
list(REMOVE_ITEM ATTO_ALL_SOURCES ${ATTO_RAW_SOURCES})

# For Emscripten, also remove win32 platform sources
if(EMSCRIPTEN)
    list(REMOVE_ITEM ATTO_ALL_SOURCES ${ATTO_WIN32_SOURCES})
endif()

# Vendor sources excluded from Shipping (ImGui / ImGuizmo / stb_vorbis)
set(VENDOR_NONSHIPPING_SOURCES
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

add_executable(atto ${ATTO_ALL_SOURCES})

if(EMSCRIPTEN)
    # Emscripten build is treated like Shipping — no editor, raw-asset, or ImGui sources
else()
    # Add editor, raw-asset, and editor-vendor sources only for non-Shipping configs.
    # Each file gets its own generator expression (required for CMake to handle lists correctly).
    foreach(src IN LISTS ATTO_EDITOR_SOURCES ATTO_RAW_SOURCES VENDOR_NONSHIPPING_SOURCES)
        target_sources(atto PRIVATE "$<$<NOT:$<CONFIG:Shipping>>:${src}>")
    endforeach()
endif()

# pocketlzma uses try/catch internally — enable exceptions for those two files only
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
    ${CLIPPER_DIR}/include
    vendor/fpm
    vendor/enet/include
    atto/src
)

if(EMSCRIPTEN)
    # Use the stub glad header that redirects to GLES3
    target_include_directories(atto PRIVATE
        ${CMAKE_SOURCE_DIR}/vendor/emscripten_compat
    )
else()
    # ImGui / ImGuizmo / Assimp headers — not needed in Shipping
    target_include_directories(atto PRIVATE
        $<$<NOT:$<CONFIG:Shipping>>:${CMAKE_SOURCE_DIR}/vendor/imgui>
        $<$<NOT:$<CONFIG:Shipping>>:${CMAKE_SOURCE_DIR}/vendor/imgui-gizmo>
        $<$<NOT:$<CONFIG:Shipping>>:${CMAKE_SOURCE_DIR}/vendor/assimp/include>
        $<$<NOT:$<CONFIG:Shipping>>:${CMAKE_BINARY_DIR}/vendor/assimp/include>
    )
endif()

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
if(EMSCRIPTEN)
    target_compile_definitions(atto PRIVATE
        _RELEASE
        ATTO_SHIPPING
        EMSCRIPTEN
    )
else()
    target_compile_definitions(atto PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:_RELEASE>
        $<$<CONFIG:Shipping>:_RELEASE>
        $<$<CONFIG:Shipping>:ATTO_SHIPPING>
    )
endif()

# Linking
if(EMSCRIPTEN)
    target_link_libraries(atto PRIVATE clipper)

    # Emscripten linker flags: GLFW3, OpenAL, WebGL2 (OpenGL ES 3.0)
    target_link_options(atto PRIVATE
        -sUSE_GLFW=3
        -sFULL_ES3=1
        -sALLOW_MEMORY_GROWTH=1
        -sWASM=1
        -lopenal
    )

    # If game.bin exists at configure time, preload it into the virtual filesystem.
    # The editor packs game.bin before invoking the Emscripten build.
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets/packed/game.bin")
        target_link_options(atto PRIVATE
            "--preload-file" "${CMAKE_SOURCE_DIR}/assets/packed/game.bin@game.bin"
        )
    endif()

    # Also pass USE_GLFW=3 at compile time so Emscripten provides the GLFW headers
    target_compile_options(atto PRIVATE -sUSE_GLFW=3)

    # Output HTML
    set_target_properties(atto PROPERTIES SUFFIX ".html")
else()
    # Core libs always linked; assimp only for non-Shipping
    target_link_libraries(atto PRIVATE
        clipper
        glad
        glfw
        $<$<NOT:$<CONFIG:Shipping>>:assimp>
    )
endif()

# ---------------------------------------------------------
# Windows platform specifics
# ---------------------------------------------------------
if(WIN32 AND NOT EMSCRIPTEN)
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
