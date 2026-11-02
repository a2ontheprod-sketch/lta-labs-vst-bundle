#pragma once
#include <JuceHeader.h>
#include "LTAProcessor.h"
#include <array>
#include <memory>

class LTADialLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit LTADialLookAndFeel(juce::Colour colour) : accent(colour) {}
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;
private:
    juce::Colour accent;
};

class LTAEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LTAEditor(LTAProcessor& processor);
    ~LTAEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawArtwork(juce::Graphics&, juce::Rectangle<float>, float energy);
    void drawHeader(juce::Graphics&);

    LTAProcessor& processor;
    lta::Profile profile;
    LTADialLookAndFeel look;
    std::array<std::unique_ptr<juce::Slider>, lta::controlCount> sliders;
    std::array<std::unique_ptr<juce::Label>, lta::controlCount> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, lta::controlCount> attachments;
    float seconds = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LTAEditor)
};
