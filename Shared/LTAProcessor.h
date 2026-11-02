#pragma once
#include <JuceHeader.h>
#include "LTAProfiles.h"
#include <array>
#include <atomic>
#include <vector>

class LTAProcessor final : public juce::AudioProcessor
{
public:
    LTAProcessor();
    ~LTAProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "LTA " + juce::String(lta::profile().title); }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    float level() const noexcept { return levelMeter.load(std::memory_order_relaxed); }
    float scanLevel() const noexcept { return scanMeter.load(std::memory_order_relaxed); }

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    float readDelay(int channel, float samplesBack) const noexcept;
    float lowpass(float sample, float& state, float cutoff) const noexcept;
    float randomBipolar() noexcept;
    static float saturate(float x) noexcept;

    double sr = 44100.0;
    std::array<std::vector<float>, 2> delay;
    int writePos = 0;
    int delayLength = 0;
    std::array<float, 2> filterA {}, filterB {}, filterC {}, filterD {};
    std::array<float, 2> echoState {}, previousIn {};
    float envelope = 0.0f;
    float sideEnvelope = 0.0f;
    float smoothingAmount = 0.45f;
    float smoothingTone = 0.5f;
    float smoothingSpace = 0.5f;
    float smoothingMotion = 0.5f;
    float smoothingMix = 0.65f;
    float smoothingOutput = 0.0f;
    double phase = 0.0;
    double grainProgress = 0.0;
    float grainBase = 0.0f;
    float grainRate = 1.0f;
    uint32_t randomState = 0x12345678u;
    uint64_t timeSamples = 0;
    std::atomic<float> levelMeter { 0.0f };
    std::atomic<float> scanMeter { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LTAProcessor)
};
