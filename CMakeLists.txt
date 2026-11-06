cmake_minimum_required(VERSION 3.22)

# Build a universal binary (Apple Silicon + Intel) that runs on macOS 12+
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Mac architectures")

project(Mishkal VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE: downloaded automatically, or point JUCE_LOCAL_PATH at a copy you have
# ---------------------------------------------------------------------------
set(JUCE_LOCAL_PATH "" CACHE PATH "Optional path to a local JUCE checkout")

if(JUCE_LOCAL_PATH)
    add_subdirectory(${JUCE_LOCAL_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Plugin formats
#   Mac:  AU + VST3 (Ableton), AAX (Pro Tools), Standalone app for quick tests
# ---------------------------------------------------------------------------
set(VOX_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND VOX_FORMATS AU AAX)
endif()

# Fonts built into the plugin (the signature script for "Lovy Elias", Barlow and Fira Sans; SIL Open Font Licence)
juce_add_binary_data(MishkalAssets SOURCES Assets/Parisienne-Regular.ttf
    Assets/Barlow-Regular.ttf Assets/Barlow-Medium.ttf Assets/Barlow-SemiBold.ttf
    Assets/FiraSans-Medium.ttf Assets/FiraSans-SemiBold.ttf)

# One function builds each version of the plugin from the same source; MISHKAL_VERSION switches the try-out changes on.
#   Mishkal   = "Mishkal Goliath Strip"     (the main version, 1)
#   V2 = screen-style Character EQ
#   V3 = V2 + minimal Reso and Tahor band controls
#   V4 = V3 + red bar gain-reduction meters (De-ess, Zerem)
#   V5 = V4 + red bar gain reduction in the Dynamics and Limiter meter blocks
# Every change gets the next number; each version installs side by side for comparing.
# Mac: copy the plug-ins into ~/Library after every build. Windows (the GitHub build): no copying, the installer does it.
if(APPLE)
    set(MISHKAL_COPY_AFTER_BUILD TRUE)
else()
    set(MISHKAL_COPY_AFTER_BUILD FALSE)
endif()
function(mishkal_plugin target product code bundle version formats)
    juce_add_plugin(${target}
        COMPANY_NAME            "Mishkal Audio"      # change to your own brand
        BUNDLE_ID               ${bundle}
        PLUGIN_MANUFACTURER_CODE Mshk                # 4 chars, at least one uppercase
        PLUGIN_CODE             ${code}              # 4 chars, unique per plugin
        PRODUCT_NAME            "${product}"
        FORMATS                 ${formats}
        IS_SYNTH                FALSE
        NEEDS_MIDI_INPUT        FALSE
        NEEDS_MIDI_OUTPUT       FALSE
        IS_MIDI_EFFECT          FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES         Fx Dynamics EQ
        AU_MAIN_TYPE            kAudioUnitType_Effect
        AAX_CATEGORY            AAX_ePlugInCategory_Dynamics
        # Copy VST3/AU into ~/Library/Audio/Plug-Ins after every build.
        COPY_PLUGIN_AFTER_BUILD ${MISHKAL_COPY_AFTER_BUILD}
        # The system AAX folder needs admin rights, so the AAX lands here instead
        # and you copy it with one sudo command (see README).
        AAX_COPY_DIR            "${CMAKE_BINARY_DIR}/AAX-ready-to-install")

    target_sources(${target} PRIVATE
        Source/PluginProcessor.cpp
        Source/FactoryPresets.cpp
        Source/PluginEditor.cpp
        Source/CleanupEqView.cpp
        Source/ToolbarView.cpp
        Source/HardwareLook.cpp)

    target_include_directories(${target} PRIVATE Source)

    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        MISHKAL_VERSION=${version})

    target_link_libraries(${target}
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            MishkalAssets
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endfunction()

# V78: renamed "Mishkal Goliath" (was "Mishkal Goliath Strip"); same plugin codes and bundle IDs, so sessions still find it
mishkal_plugin(Mishkal "Mishkal Goliath" Mkvs com.mishkalaudio.mishkal 1 "${VOX_FORMATS}")

# Try-out versions: VST3 + AU only (for comparing in Ableton). Switch off with -DMISHKAL_BUILD_VERSIONS=OFF
set(MISHKAL_LATEST_VERSION 84)   # raise this by one for every new try-out
option(MISHKAL_BUILD_VERSIONS "Also build the numbered try-out versions" ON)
if(MISHKAL_BUILD_VERSIONS)
    set(TRY_FORMATS VST3)
    if(APPLE)
        list(APPEND TRY_FORMATS AU)
    endif()
    foreach(v RANGE 2 ${MISHKAL_LATEST_VERSION})
        # plugin codes must be exactly 4 characters: Mkv2..Mkv9, then Mk10, Mk11 ...
        if(v LESS 10)
            set(code "Mkv${v}")
        else()
            set(code "Mk${v}")
        endif()
        if(v LESS 78)   # the try-outs made before the rename keep their names (renaming them would leave duplicates installed)
            set(tryname "Mishkal Goliath Strip V${v}")
        else()
            set(tryname "Mishkal Goliath V${v}")
        endif()
        mishkal_plugin(MishkalV${v} "${tryname}" "${code}" "com.mishkalaudio.mishkalv${v}" ${v} "${TRY_FORMATS}")
    endforeach()
endif()

# Tester beta: a clean name ("Mishkal Goliath Beta") built from the latest try-out version,
# so it can be packaged and sent to testers (see "Make Tester Installer.command" on the Desktop)
option(MISHKAL_BUILD_BETA "Build the tester beta" ON)
if(MISHKAL_BUILD_BETA)
    set(BETA_FORMATS VST3)
    if(APPLE)
        list(APPEND BETA_FORMATS AU)
    endif()
    mishkal_plugin(MishkalBeta "Mishkal Goliath Beta" MkBt com.mishkalaudio.mishkalbeta ${MISHKAL_LATEST_VERSION} "${BETA_FORMATS}")
endif()

# ---------------------------------------------------------------------------
# Optional: offline audio tests for the DSP (cmake -DVOX_BUILD_TESTS=ON)
# ---------------------------------------------------------------------------
option(VOX_BUILD_TESTS "Build the offline DSP test program" OFF)
if(VOX_BUILD_TESTS)
    juce_add_console_app(MishkalDspTest PRODUCT_NAME "MishkalDspTest")
    target_sources(MishkalDspTest PRIVATE Tests/DspTest.cpp)
    target_include_directories(MishkalDspTest PRIVATE Source)
    target_compile_definitions(MishkalDspTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(MishkalDspTest PRIVATE juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
