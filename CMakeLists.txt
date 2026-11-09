cmake_minimum_required(VERSION 3.22)

# Mishkal Wall: the mastering limiter (replaces Goliath's Chomah as its own plug-in).
# PUNCH > CHARACTER > BANDS > LEVELER > GOLD > CLIPPER (any order) > LIMITER > CEILING, oversampled, true peak.
# Part of the Mishkal collection with Goliath, Moses, Aaron, Mikdash, Samuel, Messiah, John the Baptist, David, Tahor and Pressure.
# Universal binary (Apple Silicon + Intel), macOS 12+
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Mac architectures")

project(Wall VERSION 1.5.0)   # raise with every version so hosts rescan
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: Goliath's copy when it's next door (no download), otherwise download it
set(JUCE_LOCAL_PATH "" CACHE PATH "Optional path to a local JUCE checkout")
if(NOT JUCE_LOCAL_PATH AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Mishkal/build/_deps/juce-src/CMakeLists.txt")
    set(JUCE_LOCAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Mishkal/build/_deps/juce-src")
endif()
if(JUCE_LOCAL_PATH)
    add_subdirectory(${JUCE_LOCAL_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(WALL_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND WALL_FORMATS AU AAX)
endif()

file(GLOB WALL_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.ttf" "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.png")
juce_add_binary_data(WallAssets SOURCES ${WALL_ASSETS})   # (the Mishkal fonts and logo)

if(APPLE)
    set(WALL_COPY_AFTER_BUILD TRUE)
else()
    set(WALL_COPY_AFTER_BUILD FALSE)
endif()

# Versions (WALL_VERSION):
#   V1 = the first Wall: the approved mock. Six movable stages (Punch with transients + upward, Character with four
#        flavours, MLA-style Bands 2-4 with upward / limit / compress-expand, Leveler, Gold converter clip, Clipper with
#        four curves) then the look-ahead Limiter (nine styles, 1-3 stages, ARC release, separate stereo links) and
#        the true-peak Ceiling that nothing passes. Oversampling Off / 2x / 4x / 8x, dither, DC, mono bass, mix,
#        delta, unity gain, streaming preview, LEARN to a target. Wave / Bands / Loudness views, GR and Crest meters,
#        LUFS M / S / I / LRA, true peak, correlation, ISP counter. Presets with our own names, A / B, undo / redo.
#   V2 = external sidechain (the limiter listens to the louder of the music and the key - stem mastering) with its own
#        gain; oversampling to 16x and 32x; dither to 24 / 22 / 20 / 18 / 16 bit with None / Basic / Optimized noise
#        shaping; the display: wave speed Fast / Slow / Infinite (the whole song) / Off, wave range 12 / 24 / 48 dB,
#        edge meters Peak 60 / Peak 24 / K-20 / K-14 / K-12, peak labels on the moments of most gain reduction;
#        loudness standards (Streaming, Apple Music, EBU R128, ATSC A/85, TR-B32, Club) that set target and ceiling,
#        an 18 / 36 LU loudness view and readings in LU from the target; MIDI Learn on every control
#   V3 = the Loudness view makes room for the song when it runs louder than the target; its Reset button moved off the graph
#   V4 = every panel more glass (stage panels, loudness strip, GR / Crest meters, the Loudness view) - the Spotify preview unchanged
#   V5 = the streaming preview banner sits in the middle of the screen; the Punch and Limiter panels 20% smaller
function(wall_plugin target product code bundle version formats)
    juce_add_plugin(${target}
        COMPANY_NAME            "Mishkal Audio"
        BUNDLE_ID               ${bundle}
        PLUGIN_MANUFACTURER_CODE Mshk
        PLUGIN_CODE             ${code}
        PRODUCT_NAME            "${product}"
        FORMATS                 ${formats}
        IS_SYNTH                FALSE
        NEEDS_MIDI_INPUT        TRUE    # V2: MIDI Learn
        NEEDS_MIDI_OUTPUT       FALSE
        IS_MIDI_EFFECT          FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES         Fx Dynamics Mastering
        AU_MAIN_TYPE            kAudioUnitType_MusicEffect   # V2: so hosts send it MIDI (MIDI Learn)
        AAX_CATEGORY            AAX_ePlugInCategory_Dynamics
        COPY_PLUGIN_AFTER_BUILD ${WALL_COPY_AFTER_BUILD}
        AAX_COPY_DIR            "${CMAKE_BINARY_DIR}/AAX-ready-to-install")
    target_sources(${target} PRIVATE Source/WallProcessor.cpp Source/WallEditor.cpp Source/WallPaint.cpp)
    if(APPLE)
        target_sources(${target} PRIVATE Source/WallMac.mm)
    endif()
    target_include_directories(${target} PRIVATE Source)
    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0
        WALL_VERSION=${version})
    target_link_libraries(${target}
        PRIVATE juce::juce_audio_utils juce::juce_dsp WallAssets
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
endfunction()

set(WALL_LATEST_VERSION 5)   # raise this by one for every new version

wall_plugin(Wall "Mishkal Wall" Wall com.mishkalaudio.wall ${WALL_LATEST_VERSION} "${WALL_FORMATS}")
