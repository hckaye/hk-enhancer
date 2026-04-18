cmake_minimum_required(VERSION 3.22)
project(HKEnhancer VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(HKEnhancer
    PRODUCT_NAME "HK Enhancer"
    COMPANY_NAME "HK Audio"
    BUNDLE_ID "com.hkaudio.hkenhancer"
    PLUGIN_MANUFACTURER_CODE Hkay
    PLUGIN_CODE Hken
    FORMATS VST3 AU Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(HKEnhancer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/MultibandSplitter.cpp
    Source/DSP/EnvelopeFollower.cpp
    Source/DSP/SubharmonicGenerator.cpp
    Source/DSP/SubBassEnhancer.cpp
    Source/DSP/TubeSaturator.cpp
    Source/DSP/HarmonicExciter.cpp
    Source/GUI/LookAndFeel.cpp
    Source/GUI/BandControl.cpp
)

target_include_directories(HKEnhancer PRIVATE Source)

target_compile_definitions(HKEnhancer PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(HKEnhancer PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# --- Lint (clang-tidy) ---
option(ENABLE_CLANG_TIDY "Enable clang-tidy analysis" OFF)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set_target_properties(HKEnhancer PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy not found")
    endif()
endif()

# --- Unit Tests ---
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(HKEnhancerTests
        Tests/TestEnvelopeFollower.cpp
        Tests/TestMultibandSplitter.cpp
        Tests/TestTubeSaturator.cpp
        Tests/TestHarmonicExciter.cpp
        Tests/TestSubharmonicGenerator.cpp
        Tests/TestSubBassEnhancer.cpp
        Source/DSP/EnvelopeFollower.cpp
        Source/DSP/MultibandSplitter.cpp
        Source/DSP/TubeSaturator.cpp
        Source/DSP/HarmonicExciter.cpp
        Source/DSP/SubharmonicGenerator.cpp
        Source/DSP/SubBassEnhancer.cpp
    )

    target_include_directories(HKEnhancerTests PRIVATE Source)

    target_compile_features(HKEnhancerTests PRIVATE cxx_std_17)

    target_compile_definitions(HKEnhancerTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
    )

    target_link_libraries(HKEnhancerTests PRIVATE
        Catch2::Catch2WithMain
        juce::juce_dsp
        juce::juce_audio_basics
    )

    include(CTest)
    include(Catch)
    catch_discover_tests(HKEnhancerTests)
endif()
