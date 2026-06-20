cmake_minimum_required(VERSION 3.22)

project(MoodMixer VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

# Use a JUCE checked out next to this project when present (local dev),
# otherwise fetch the matching version (CI / fresh clones).
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.13
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

#==============================================================================
# AU is macOS-only; everywhere else build VST3 + Standalone.
if (APPLE)
    set(UT_FORMATS AU VST3 Standalone)
else()
    set(UT_FORMATS VST3 Standalone)
endif()

juce_add_plugin(MoodMixer
    PRODUCT_NAME            "Ultimo Tocco"
    COMPANY_NAME            "StudioGift"
    BUNDLE_ID               "com.studiogift.moodmixer"
    PLUGIN_MANUFACTURER_CODE Sgft
    PLUGIN_CODE             Mood
    FORMATS                 ${UT_FORMATS}
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(MoodMixer)

#==============================================================================
# Embed the portrait frames as binary data.
#   rise_*  : normal -> happy            (knob 0.0 .. 0.5)
#   doubt_* : happy  -> "are you sure?"  (knob 0.5 .. 1.0)
#   mood_*  : legacy single-gradient fallback
file(GLOB MOOD_FRAMES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.png")

juce_add_binary_data(MoodMixerAssets SOURCES ${MOOD_FRAMES})

# Position-independent code so the static asset lib links cleanly into the
# shared plugin bundles (VST3/AU).
set_target_properties(MoodMixerAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

#==============================================================================
target_sources(MoodMixer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(MoodMixer PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    # Decode embedded PNGs with JUCE's bundled libpng instead of Apple ImageIO.
    # ImageIO's PNG plugin hits an alignment fault on some macOS versions.
    JUCE_USE_COREIMAGE_LOADER=0)

target_link_libraries(MoodMixer
    PRIVATE
        MoodMixerAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

#==============================================================================
# Optional headless tool to render the editor UI to a PNG (build verification).
option(MOODMIXER_BUILD_SNAPSHOT "Build the headless UI snapshot tool" OFF)

if (MOODMIXER_BUILD_SNAPSHOT)
    juce_add_console_app(MoodSnapshot PRODUCT_NAME "MoodSnapshot")
    juce_generate_juce_header(MoodSnapshot)
    target_sources(MoodSnapshot PRIVATE
        tools/snapshot.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
    target_include_directories(MoodSnapshot PRIVATE Source)
    target_compile_definitions(MoodSnapshot PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_USE_COREIMAGE_LOADER=0
        JUCE_STANDALONE_APPLICATION=1
        JucePlugin_Name="Ultimo Tocco")
    target_link_libraries(MoodSnapshot PRIVATE
        MoodMixerAssets
        juce::juce_audio_utils
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endif()
