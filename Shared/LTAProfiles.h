#pragma once
#include <JuceHeader.h>
#include <array>

#ifndef LTA_PLUGIN_INDEX
 #error "LTA_PLUGIN_INDEX must be set by CMake"
#endif

namespace lta
{
constexpr int pluginIndex = LTA_PLUGIN_INDEX;
constexpr int controlCount = 6;

struct Profile
{
    const char* title;
    const char* subtitle;
    const char* motif;
    std::array<const char*, controlCount> labels;
    juce::Colour accent;
    juce::Colour background;
};

inline Profile profile()
{
    switch (pluginIndex)
    {
        case 0: return { "SKINWALKER", "VOCAL SHAPE ENGINE", "MORPH",
            {{"MUTATE", "FORMANT", "ROBOT", "MONSTER", "MIX", "OUTPUT"}},
            juce::Colour(0xffc6e0d0), juce::Colour(0xff101a18) };
        case 1: return { "DREAM JOURNAL", "GRANULAR MEMORY CAPTURE", "REMEMBER",
            {{"MEMORY", "GRAIN", "DREAM", "MOTION", "MIX", "OUTPUT"}},
            juce::Colour(0xffd6b9f3), juce::Colour(0xff1a1628) };
        case 2: return { "MOSHPIT", "DRUM IMPACT & CHAOS", "ENERGY",
            {{"AGGRESSION", "IMPACT", "CRUSH", "CROWD", "MIX", "OUTPUT"}},
            juce::Colour(0xffff6554), juce::Colour(0xff261314) };
        case 3: return { "ECLIPSE", "SHADOW FILTER ENGINE", "OCCLUSION",
            {{"ECLIPSE", "COLOUR", "EDGE", "ORBIT", "MIX", "OUTPUT"}},
            juce::Colour(0xffeec78a), juce::Colour(0xff151619) };
        case 4: return { "REDACTED", "RHYTHMIC AUDIO CENSOR", "CLASSIFIED",
            {{"BLACKOUT", "RATE", "FILTER", "SCRAMBLE", "MIX", "OUTPUT"}},
            juce::Colour(0xffe9e7df), juce::Colour(0xff191b1c) };
        case 5: return { "SECOND SUN", "PARALLEL HARMONIC GLOW", "RADIANCE",
            {{"RADIANCE", "WARMTH", "DETUNE", "WIDTH", "MIX", "OUTPUT"}},
            juce::Colour(0xffffbd62), juce::Colour(0xff231719) };
        case 6: return { "NIGHTMARE FUEL", "DARK MODULATION SPACE", "DREAD",
            {{"FEAR", "DARKNESS", "ECHO", "TENSION", "MIX", "OUTPUT"}},
            juce::Colour(0xffff555c), juce::Colour(0xff170d13) };
        case 7: return { "X-RAY", "SIDECHAIN BASS SCANNER", "SCAN",
            {{"REDUCTION", "TARGET", "RELEASE", "SENSITIVITY", "MIX", "OUTPUT"}},
            juce::Colour(0xff70e8f0), juce::Colour(0xff101a24) };
        default: return { "FALSE MEMORY", "NOSTALGIC AUDIO DECAY", "FADE",
            {{"AGE", "BLUR", "DRIFT", "GHOST", "MIX", "OUTPUT"}},
            juce::Colour(0xffb3c5ab), juce::Colour(0xff1b211c) };
    }
}

inline const std::array<const char*, controlCount>& parameterIDs()
{
    static const std::array<const char*, controlCount> ids {{
        "amount", "tone", "space", "motion", "mix", "output"
    }};
    return ids;
}
}
