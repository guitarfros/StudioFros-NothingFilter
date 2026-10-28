cmake_minimum_required(VERSION 3.22)

# macOS: universal binary (Apple Silicon + Intel). Must be set before project().
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binaries")
    endif()
endif()

project(StudioFrosNothingFilter VERSION 0.1.0 LANGUAGES C CXX)

# Windows: static runtime, so users do not need the Visual C++ redistributable.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Copy the built VST3 into the user's plug-in folder after building.
# Off by default on Windows (the system folder needs admin rights) and on CI.
if(WIN32 OR DEFINED ENV{CI})
    set(nf_copy_default OFF)
else()
    set(nf_copy_default ON)
endif()
option(NF_COPY_AFTER_BUILD "Copy the VST3 to the plug-in folder after building" ${nf_copy_default})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- JUCE: -DJUCE_PATH, env JUCE_PATH, nearby folders, installed package, or download ----
set(JUCE_PATH "" CACHE PATH "Path to a JUCE source checkout (optional)")
if(NOT JUCE_PATH AND DEFINED ENV{JUCE_PATH})
    set(JUCE_PATH "$ENV{JUCE_PATH}")
endif()
if(NOT JUCE_PATH)
    file(GLOB nf_glob
        "${CMAKE_CURRENT_SOURCE_DIR}/../*/JUCE"
        "$ENV{HOME}/*/JUCE" "$ENV{HOME}/Desktop/*/JUCE" "$ENV{HOME}/Documents/*/JUCE")
    foreach(cand
            "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE"
            "$ENV{HOME}/JUCE" "$ENV{HOME}/juce" "/opt/JUCE" "/usr/local/JUCE" "/usr/share/JUCE"
            ${nf_glob})
        if(EXISTS "${cand}/CMakeLists.txt" AND EXISTS "${cand}/modules")
            set(JUCE_PATH "${cand}")
            break()
        endif()
    endforeach()
endif()

if(JUCE_PATH)
    message(STATUS "Using JUCE source at: ${JUCE_PATH}")
    add_subdirectory(${JUCE_PATH} JUCE EXCLUDE_FROM_ALL)
else()
    find_package(JUCE CONFIG QUIET)
    if(NOT JUCE_FOUND)
        message(STATUS "JUCE not found locally: downloading JUCE 8.0.4 (needs git + internet)")
        include(FetchContent)
        FetchContent_Declare(juce
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 8.0.4
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(juce)
    endif()
endif()

# ---- Plugin ----
juce_add_plugin(NothingFilter
    COMPANY_NAME "GuitarFros"
    PRODUCT_NAME "StudioFros - Nothing Filter"
    BUNDLE_ID com.guitarfros.nothingfilter
    PLUGIN_MANUFACTURER_CODE Gfro
    PLUGIN_CODE Nfil
    FORMATS VST3
    VST3_CATEGORIES Fx Filter
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${NF_COPY_AFTER_BUILD}
)

target_sources(NothingFilter PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/FactoryPresets.h
    Source/wah_core.h
)

# ---- Embedded images (Assets/*.png, Assets/*.jpg -> BinaryData) ----
file(GLOB NF_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.png" "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.jpg")
juce_add_binary_data(NFAssets SOURCES ${NF_ASSETS})
set_target_properties(NFAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(NothingFilter PRIVATE NFAssets)

target_compile_definitions(NothingFilter PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(NothingFilter
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
