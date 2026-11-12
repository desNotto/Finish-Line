cmake_minimum_required(VERSION 3.22)

project(FinishLine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static CRT: the .vst3 does not need the VC++ redistributable on the target machine.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(FL_BUILD_TESTS "Build the offline test / render harness" ON)

# JUCE: use a local checkout in external/JUCE when present, otherwise fetch it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(FL_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

set(FL_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

juce_add_plugin(FinishLine
    COMPANY_NAME "desNotto"
    PLUGIN_MANUFACTURER_CODE Dsnt
    PLUGIN_CODE FnLn
    FORMATS VST3
    PRODUCT_NAME "Finish Line"
    DESCRIPTION "Modern metal post-production FX generator"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth Sampler)

target_sources(FinishLine PRIVATE ${FL_SOURCES})
target_include_directories(FinishLine PRIVATE Source)
target_compile_definitions(FinishLine PUBLIC ${FL_DEFS})
if(MSVC)
    target_compile_options(FinishLine PRIVATE /utf-8 /W3)
endif()
target_link_libraries(FinishLine
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)

if(FL_BUILD_TESTS)
    juce_add_console_app(FinishLineTest PRODUCT_NAME "FinishLineTest")
    target_sources(FinishLineTest PRIVATE ${FL_SOURCES} Tests/Main.cpp)
    target_include_directories(FinishLineTest PRIVATE Source)
    target_compile_definitions(FinishLineTest PRIVATE ${FL_DEFS} JUCE_MODAL_LOOPS_PERMITTED=1)
    if(MSVC)
        target_compile_options(FinishLineTest PRIVATE /utf-8 /W3)
    endif()
    target_link_libraries(FinishLineTest
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags)
endif()
