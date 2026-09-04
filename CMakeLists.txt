cmake_minimum_required(VERSION 3.22)

project(LiveCutVariant VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor/JUCE)

juce_add_plugin(LiveCutVariant
    PRODUCT_NAME "LiveCut Enhanced"
    COMPANY_NAME "LiveCut Community"
    BUNDLE_ID com.livecutcommunity.livecut
    PLUGIN_MANUFACTURER_CODE LcCo
    PLUGIN_CODE Lcv1
    FORMATS VST3 Standalone
    VST3_CATEGORIES Fx
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(LiveCutVariant PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/dsp/BBCutter.cpp
    src/dsp/BitCrusher.cpp
    src/dsp/Comb.cpp
    src/dsp/DelayLine.cpp
    src/dsp/FirstOrderLowpass.cpp
)

target_compile_definitions(LiveCutVariant PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(LiveCutVariant
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
)

enable_testing()

add_executable(KernelSilenceTest
    tests/KernelSilenceTest.cpp
    src/dsp/BBCutter.cpp
    src/dsp/BitCrusher.cpp
    src/dsp/Comb.cpp
    src/dsp/DelayLine.cpp
    src/dsp/FirstOrderLowpass.cpp
)
target_include_directories(KernelSilenceTest PRIVATE src/dsp)

add_test(NAME KernelSilenceTest COMMAND KernelSilenceTest)

add_executable(SubdivSmokeTest
    tests/SubdivSmokeTest.cpp
    src/dsp/BBCutter.cpp
    src/dsp/BitCrusher.cpp
    src/dsp/Comb.cpp
    src/dsp/DelayLine.cpp
    src/dsp/FirstOrderLowpass.cpp
)
target_include_directories(SubdivSmokeTest PRIVATE src/dsp)

add_test(NAME SubdivSmokeTest COMMAND SubdivSmokeTest)
