cmake_minimum_required(VERSION 3.22)

project(DrumDoctor VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(DrumDoctor
    COMPANY_NAME "DrumDoctor"
    BUNDLE_ID "com.drumdoctor.plugin"
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_SYNTH FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE DrDm
    PLUGIN_CODE Ddoc
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Drum Doctor")

target_sources(DrumDoctor
    PRIVATE
        src/analysis/DrumMap.cpp
        src/analysis/PlayabilityAnalyzer.cpp
        src/plugin/PluginProcessor.cpp
        src/plugin/PluginEditor.cpp
        src/ui/TimelineComponent.cpp
        src/ui/DifficultyMeterComponent.cpp
        src/ui/ViolationListComponent.cpp)

target_include_directories(DrumDoctor PRIVATE src)

target_compile_definitions(DrumDoctor
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0)

target_link_libraries(DrumDoctor
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

add_executable(DrumDoctorTests
    tests/PlayabilityTests.cpp
    src/analysis/DrumMap.cpp
    src/analysis/PlayabilityAnalyzer.cpp)

target_include_directories(DrumDoctorTests PRIVATE src)

target_compile_definitions(DrumDoctorTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0)

target_link_libraries(DrumDoctorTests
    PRIVATE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
