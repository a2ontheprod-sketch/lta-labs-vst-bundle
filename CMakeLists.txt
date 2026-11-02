cmake_minimum_required(VERSION 3.22)
project(LTA_Labs_Vol1 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    URL https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.6.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(JUCE)

function(add_lta_effect target display_name code index slug)
    juce_add_plugin(${target}
        COMPANY_NAME "LTA Labs"
        BUNDLE_ID "audio.ltalabs.${slug}"
        PLUGIN_MANUFACTURER_CODE Ltal
        PLUGIN_CODE ${code}
        FORMATS VST3
        PRODUCT_NAME "${display_name}"
        DESCRIPTION "LTA Labs Volume 1 creative effect"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE)

    juce_generate_juce_header(${target})

    target_sources(${target} PRIVATE
        Shared/LTAProfiles.h
        Shared/LTAProcessor.h
        Shared/LTAProcessor.cpp
        Shared/LTAEditor.h
        Shared/LTAEditor.cpp)

    target_compile_definitions(${target} PRIVATE
        LTA_PLUGIN_INDEX=${index}
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(${target} PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endfunction()

add_lta_effect(LTA_Skinwalker     "LTA Skinwalker"     SkW1 0 skinwalker)
add_lta_effect(LTA_DreamJournal   "LTA Dream Journal"   DrJ1 1 dreamjournal)
add_lta_effect(LTA_Moshpit        "LTA Moshpit"        MsP1 2 moshpit)
add_lta_effect(LTA_Eclipse        "LTA Eclipse"        EcL1 3 eclipse)
add_lta_effect(LTA_Redacted       "LTA Redacted"       RdC1 4 redacted)
add_lta_effect(LTA_SecondSun      "LTA Second Sun"      ScS1 5 secondsun)
add_lta_effect(LTA_NightmareFuel  "LTA Nightmare Fuel"  NmF1 6 nightmarefuel)
add_lta_effect(LTA_XRay           "LTA X-Ray"          XrA1 7 xray)
add_lta_effect(LTA_FalseMemory    "LTA False Memory"    FlM1 8 falsememory)
