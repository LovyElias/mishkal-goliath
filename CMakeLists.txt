cmake_minimum_required(VERSION 3.22)

# Mishkal Tahor: the standalone EQ - RESO (resonance suppressor) > TAHOR (clean EQ) > CHARACTER (eleven vintage units).
# Part of the Mishkal collection with Goliath, Moses, Aaron, Mikdash, Samuel, Messiah, John the Baptist and David.
# Universal binary (Apple Silicon + Intel), macOS 12+
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Mac architectures")

project(Tahor VERSION 1.13.0)   # raise with every version so hosts rescan
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
juce_add_binary_data(TahorAssets SOURCES ${TAHOR_FONTS})

if(APPLE)
    set(TAHOR_COPY_AFTER_BUILD TRUE)
else()
    set(TAHOR_COPY_AFTER_BUILD FALSE)
endif()

# Versions (TAHOR_VERSION switches each version's changes on):
#   V1 = the first Tahor, exactly the Claude Design canvas: three tabs RESO > TAHOR > CHARACTER (each with its power),
#        RESO (soothe-style: Soft / Hard, Depth, Detail, Attack, Release, Max cut, Mix, Trim, Low latency, Stereo / Mid /
#        Side, Delta, Sidechain, up to six shaping bands in eight shapes, the focus edges, an Advanced panel with Tilt,
#        Stereo link and Minimum / Linear phase), TAHOR (Goliath's clean EQ: eight bands, Natural / Linear / Linear HQ,
#        ST / M / S / L / R, Dynamic, Spectral, Sidechain, Solo, 6 to 48 dB/oct cuts), CHARACTER (eleven vintage units -
#        LE-EP1, LE-ME5, LE-E4K, LE-N73, LE-N81, LE-550B, LE-K9, LE-H69, LE-SP1, LE-M4, LE-MP2 - up to four in a chain,
#        Clean / Subtle / Warm), oversampling Off to 32x plus its own Render setting, undo / redo, presets, A / B,
#        MATCH, BYPASS, IN / AUTO / OUT, external sidechain, a resizable window
#   V2 = the input meter on the left (IN and AUTO under it), the output meter on the right (SC and OUT under it); a
#        Pro-Q 4-style analyzer (smooth arches: Pre a deep layer behind, Post bright in front); every vintage unit starts
#        flat; drag nodes on the Character curves (locked to each unit's steps and Q); the hardware 60% of its size and
#        see-through until the mouse is over it, Clean / Subtle / Warm under it; no page dots; a full-screen button;
#        a livelier, slowly moving background with a dB grid
#   V3 = full screen fits the screen (nothing cut off at the top); Tahor's band chips smaller and always fit all eight;
#        SAVE / SAVE AS in the preset menu; the tabs one small control (each with its power); the screen edge to edge
#        with the meters inside it (Pro-Q style); Tahor starts flat (no bands); sizes Mini / Small / Medium / Large /
#        Extra Large (remembered), one compact RESO / TAHOR / CHARACTER selector (arrows, its power), thin meters at the
#        window's edges, the graph nearly edge to edge, each tab's band panel fixed at the bottom (no chip row);
#        only the dots show on every tab - the controls fade in when the mouse touches a dot (or their place) and out again
#   V4 = each band's panel is solid and opens right under the band you touch (half a second after the mouse leaves, it
#        goes); Character's hardware 40% and solid, hidden until a band is touched; the chain chips down the left, faint
#        until the mouse is over them; a smaller legend and Reso's line near the top; a slim bottom bar; the screen's
#        corners rounded, fading softly into the header and the bottom bar
#   V5 = the band panels small (Pro-Q 4 style) along the bottom, sliding under the band you touch; Reso's strip thinner
#        and solid, open from the start, moved only by its handle; Depth starts at 9 o'clock; the hardware always shown;
#        Clean / Subtle / Warm in the top row (Character only); the screen arches up into the header and down into the
#        bottom bar (the bar's controls split either side); sharper drawing (a point per pixel, an 8192-point analyzer)
#   V6 = Reso's controls slide out from the left (in the space below 50 Hz), ADV from the right, each with a tab on the
#        edge; Reso's band panel: Amount and Width on top, a shape dropdown below; Tahor's four switches in one dropdown;
#        band panels 20% bigger; each Character unit casts its own coloured shadow; Reso's shapes in shades of red;
#        both arches centred and the same width; the analyzer fills the bottom arch; fine grain against banding
#   V7 = the arches back (fixed width, centred); smaller header controls; logo "Mishkal / by Lovy Elias" over a faded
#        scale; Advanced matches the Depth panel and both side panels are centred; a THD slider on Character (charThd);
#        + Add EQ under the chain chips; arched band panels with a glow in the band's colour
#   V8 = a 44 px header with preset < > arrows; the top arch blends in like the bottom (no outline anywhere); Reso's
#        line opens under the arch and the reduction rises into it; Reso's hint stays in the lit area; worn vintage band
#        panels; Solo on Tahor and Character band panels; + Add EQ shows the gear; L and R curves drawn apart
#   V9 = L / R split bands on all three tabs (independent partners); EQ / Dynamic / Spectral band views (true spectral
#        detection, per-band attack / release); Pro-Q-style solo column; Reso solo + dims when off; analyzer edge fades;
#        selector by the arch; scale after the logo; history list; auto gain; tagged preset browser; paired boost / cut
#        dots; one-word unit hints; five new units: LE-Esh, LE-Shemen, LE-Kav, LE-Even, LE-Kavod
#   V10 = Spectral removed from Tahor; plain-text unit hint; the analyzer only shows while audio plays; Pro-Q-style
#        dynamic curves (set gain solid, live reduction shaded); bigger breathing tab arrows; peak / average readouts
#        under the meters; two-unit gear (EP1, MP2, Esh, Even, Kavod); hardware arrows only on hover, centred
#   V11 = the dynamic range drawn on screen; Tahor's (and Reso's) bubble shows frequency and gain; Character starts empty
#        (LE-EP1 previewed); two-unit gear laid out like the originals (EP1 gains its high atten); no tube glow; the
#        hint sits higher and quieter; 69 factory presets in ten folders; keyboard shortcuts
#   V12 = modern 80% band panels (arch bump, softened corners, colour line), bigger main knobs, placement dropdowns,
#        band power buttons, solo on the bubbles, right-click menus, Dynamic stays on in the EQ view, drag the range on
#        screen, 20 Hz - 20 kHz scale, preset submenus, see-through Reso side panels, Spectrum Grab, multi-select,
#        piano roll, EQ Match, opens at Small, double-click a Character band to switch it off
#   V13 = the piano strip has its own on / off button in the bottom bar (off to start); the note stays in the bubble
#   V14 = the dynamic range can be grabbed anywhere along its dashed outline (it brightens under the mouse);
#        the band bubble stays open while you hover it and lingers 4 s (like Pro-Q 4), so Solo is easy to reach;
#        click the shape for a menu, or click the frequency / gain / Q to type a value ("2.5k", "A4", "-3", "0.7");
#        over the empty screen a hairline and readout show the frequency, note (and gain on Tahor) under the mouse
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
        VST3_CATEGORIES         Fx EQ
        AU_MAIN_TYPE            kAudioUnitType_Effect
        AAX_CATEGORY            AAX_ePlugInCategory_EQ
        COPY_PLUGIN_AFTER_BUILD ${TAHOR_COPY_AFTER_BUILD}
        AAX_COPY_DIR            "${CMAKE_BINARY_DIR}/AAX-ready-to-install")
    target_sources(${target} PRIVATE Source/TahorProcessor.cpp Source/TahorEditor.cpp Source/TahorPaint.cpp)
    if(APPLE)
        target_sources(${target} PRIVATE Source/TahorMac.mm)
    endif()
    target_include_directories(${target} PRIVATE Source Source/Dsp)
    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0
        TAHOR_VERSION=${version})
    target_link_libraries(${target}
        PRIVATE juce::juce_audio_utils juce::juce_dsp TahorAssets
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
endfunction()

set(TAHOR_LATEST_VERSION 14)   # raise this by one for every new version

# One plug-in, updated in place (the version shows in its window's About)
tahor_plugin(Tahor "Mishkal Tahor" Thor com.mishkalaudio.tahor ${TAHOR_LATEST_VERSION} "${TAHOR_FORMATS}")
