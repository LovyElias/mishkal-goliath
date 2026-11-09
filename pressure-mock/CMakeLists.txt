cmake_minimum_required(VERSION 3.22)

# Mishkal Pressure: dynamics - HELPERS > CLEAN > VINTAGE (built on Tahor's frame: header, meters, presets, sizes).
# Part of the Mishkal collection with Goliath, Moses, Aaron, Mikdash, Samuel, Messiah, John the Baptist, David and Tahor.
# Universal binary (Apple Silicon + Intel), macOS 12+
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Mac architectures")

project(Pressure VERSION 1.10.0)   # raise with every version so hosts rescan
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

set(TAHOR_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND TAHOR_FORMATS AU AAX)
endif()

file(GLOB TAHOR_FONTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.ttf")
juce_add_binary_data(TahorAssets SOURCES ${TAHOR_FONTS})   # (the shared Mishkal fonts)

if(APPLE)
    set(TAHOR_COPY_AFTER_BUILD TRUE)
else()
    set(TAHOR_COPY_AFTER_BUILD FALSE)
endif()

# Versions (PRESSURE_VERSION):
#   V1 = the first Pressure: the approved design (HELPERS > CLEAN > VINTAGE), CLEAN working - LE-11/25E, LE-755C, LE-DS1,
#        LE-MLA, LE-580E on one feed-forward core (knee, attack / release, auto attack / release, peak / RMS, range, hold,
#        mix, make-up, auto gain, delta, sidechain: internal / external, listen, gain, link, M / S, look-ahead, detector EQ);
#        every CLEAN knob and switch live; the knee, envelope, top and sidechain drawers open; real gain reduction on screen.
#        HELPERS and VINTAGE are on screen and pass the sound through (wired up next).
#   V2 = fix: a crash when the window was closed and opened again (or with two copies open) - the screen read a closed
#        window; the detector EQ's filters set up properly (they were running as first-order); LE-755C no longer adds
#        CLEAN's Make-up on top of its own Gain
#   V3 = fix: the crash on opening (HELPERS drew its Rider meter and Breath strip before the screen's history existed)
#   V4 = HELPERS gets its name at the top centre, like the other tabs: LE-4, with PEAK · RIDE · LIFT · BREATH under it;
#        the HELPERS tab selector wears its bronze (was teal)
#   V5 = HELPERS works: Lift (two bands, Upward / Dyn EQ), Rider (Range, Target, Speed), Breath (Amount, Sense, Fade),
#        the 1964 (Input, Output, Slope, Release, Meter, Pad - gain only, no colour); every HELPERS knob, switch and
#        power button live; the Rider meter, loudness, Breath light / count, 1964 lights and the Tone drawer show the real
#        thing. Each tab's on / off is its own labelled button beside the selector. Everything that goes away (drawers,
#        the knee display, controls that sit back) fades out over 3 seconds.
#   V6 = VINTAGE works: all fifteen units (LE-76, LE-660, LE-STA, LE-2500, LE-G, LE-IRON, LE-336, LE-8X, LE-160, LE-7,
#        LE-760, LE-SHM, LE-2A, LE-1B, LE-VSC), each with its own controls live (knobs, stepped switches, push buttons,
#        bat toggles, the 1176 revision, the 2500's Thrust, the top-strip switches, Delta, THD) and its own detector,
#        curve, timing and colour; the unit arrows choose the unit you hear. Stereo link fixed when one side is quiet
#        (CLEAN too). The sidechain handle is gone from VINTAGE (its switches are on the top strip).
#   V7 = a tab switched off dims its whole screen (like a HELPERS module that's off - still adjustable); a full-screen
#        button in the bottom bar beside Size (like Tahor's); the undo arrow no longer touches "Pressure"
#   V8 = Solo on Lift's Low and High bands (an S beside each band's name): hear just that band, as Lift shapes it
#   V9 = the Character drawer's light distorts with THD (runs hot gold > red, splits into red / green / blue ghosts,
#        flickers); a new Pressure starts with CLEAN and VINTAGE off (the CLEAN presets switch CLEAN on) and
#        oversampling off
#   V10 = a gain-reduction strip along the bottom of CLEAN and VINTAGE: the last ten seconds of reduction (with the peak
#         as a dashed line), and NOW / AVERAGE / PEAK readouts
function(tahor_plugin target product code bundle version formats)
    juce_add_plugin(${target}
        COMPANY_NAME            "Mishkal Audio"
        BUNDLE_ID               ${bundle}
        PLUGIN_MANUFACTURER_CODE Mshk
        PLUGIN_CODE             ${code}
        PRODUCT_NAME            "${product}"
        FORMATS                 ${formats}
        IS_SYNTH                FALSE
        NEEDS_MIDI_INPUT        FALSE
        NEEDS_MIDI_OUTPUT       FALSE
        IS_MIDI_EFFECT          FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES         Fx Dynamics
        AU_MAIN_TYPE            kAudioUnitType_Effect
        AAX_CATEGORY            AAX_ePlugInCategory_Dynamics
        COPY_PLUGIN_AFTER_BUILD ${TAHOR_COPY_AFTER_BUILD}
        AAX_COPY_DIR            "${CMAKE_BINARY_DIR}/AAX-ready-to-install")
    target_sources(${target} PRIVATE Source/TahorProcessor.cpp Source/TahorEditor.cpp Source/TahorPaint.cpp)
    if(APPLE)
        target_sources(${target} PRIVATE Source/TahorMac.mm)
    endif()
    target_include_directories(${target} PRIVATE Source Source/Dsp)
    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0
        TAHOR_VERSION=15 PRESSURE_VERSION=${version})
    target_link_libraries(${target}
        PRIVATE juce::juce_audio_utils juce::juce_dsp TahorAssets
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
endfunction()

set(PRESSURE_LATEST_VERSION 10)   # raise this by one for every new version

# One plug-in, updated in place
tahor_plugin(Pressure "Mishkal Pressure" Prss com.mishkalaudio.pressure ${PRESSURE_LATEST_VERSION} "${TAHOR_FORMATS}")
