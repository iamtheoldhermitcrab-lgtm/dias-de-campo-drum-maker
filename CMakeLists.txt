cmake_minimum_required(VERSION 3.22)
project(DiasDeCampoDrumMaker VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: usa una copia local si se indica JUCE_PATH; si no, la descarga automáticamente
if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(DiasDeCampoDrumMaker
    COMPANY_NAME "Dias de Campo"
    PRODUCT_NAME "Dias de Campo Drum Maker"
    PLUGIN_MANUFACTURER_CODE Ddcp
    PLUGIN_CODE Ddcd
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Drum
    FORMATS VST3 AU Standalone)

juce_generate_juce_header(DiasDeCampoDrumMaker)

juce_add_binary_data(DdcFonts SOURCES
    Resources/PressStart2P-Regular.ttf
    Resources/PixelifySans.ttf)

target_sources(DiasDeCampoDrumMaker PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(DiasDeCampoDrumMaker PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(DiasDeCampoDrumMaker
    PRIVATE
        DdcFonts
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(DDC_BUILD_TEST)
    juce_add_console_app(DdcTest PRODUCT_NAME "DdcTest")
    target_sources(DdcTest PRIVATE test/Test.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(DdcTest PRIVATE ${CMAKE_BINARY_DIR}/juce_binarydata_DdcFonts/JuceLibraryCode)
    juce_generate_juce_header(DdcTest)
    target_compile_definitions(DdcTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="DdcTest")
    target_link_libraries(DdcTest PRIVATE DdcFonts juce::juce_audio_utils juce::juce_audio_formats juce::juce_recommended_config_flags)
endif()
