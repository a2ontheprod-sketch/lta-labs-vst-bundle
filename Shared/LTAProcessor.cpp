#include "LTAProcessor.h"
#include "LTAEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;
float clamp01(float x) { return juce::jlimit(0.0f, 1.0f, x); }
float lerp(float a, float b, float t) { return a + (b - a) * t; }
}

juce::AudioProcessorValueTreeState::ParameterLayout LTAProcessor::makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    const auto p = lta::profile();
    const auto& ids = lta::parameterIDs();
    for (int i = 0; i < lta::controlCount; ++i)
    {
        const bool output = i == 5;
        const float defaultValue = output ? 0.0f : (i == 4 ? 0.65f : 0.45f);
        result.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ids[static_cast<size_t>(i)], 1),
            juce::String(p.labels[static_cast<size_t>(i)]),
            juce::NormalisableRange<float>(output ? -12.0f : 0.0f,
                                           output ? 12.0f : 1.0f,
                                           output ? 0.1f : 0.001f),
            defaultValue));
    }
    return result;
}

LTAProcessor::LTAProcessor()
    : juce::AudioProcessor(
#if LTA_PLUGIN_INDEX == 7
        BusesProperties()
            .withInput("Main Input", juce::AudioChannelSet::stereo(), true)
            .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
            .withOutput("Main Output", juce::AudioChannelSet::stereo(), true)
#else
        BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ),
      parameters(*this, nullptr, "LTAState", makeParameters())
{
}

bool LTAProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    if (input != output)
        return false;
    if (input != juce::AudioChannelSet::mono() &&
        input != juce::AudioChannelSet::stereo())
        return false;
#if LTA_PLUGIN_INDEX == 7
    const auto side = layouts.getChannelSet(true, 1);
    return side.isDisabled() || side == juce::AudioChannelSet::mono() ||
           side == juce::AudioChannelSet::stereo();
#else
    return true;
#endif
}

void LTAProcessor::prepareToPlay(double sampleRate, int)
{
    sr = juce::jmax(8000.0, sampleRate);
    delayLength = static_cast<int>(std::ceil(sr * 7.0)) + 8;
    for (auto& d : delay)
        d.assign(static_cast<size_t>(delayLength), 0.0f);
    reset();
}

void LTAProcessor::reset()
{
    for (auto& d : delay)
        std::fill(d.begin(), d.end(), 0.0f);
    filterA.fill(0.0f); filterB.fill(0.0f);
    filterC.fill(0.0f); filterD.fill(0.0f);
    echoState.fill(0.0f); previousIn.fill(0.0f);
    envelope = 0.0f; sideEnvelope = 0.0f;
    writePos = 0; phase = 0.0;
    grainProgress = 0.0; grainBase = 0.0f;
    grainRate = 1.0f; timeSamples = 0;
    levelMeter.store(0.0f, std::memory_order_relaxed);
    scanMeter.store(0.0f, std::memory_order_relaxed);
}

float LTAProcessor::randomBipolar() noexcept
{
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;
    return (static_cast<float>(randomState & 0xffffu) / 32767.5f) - 1.0f;
}

float LTAProcessor::saturate(float x) noexcept
{
    // Soft limiting keeps effects bounded when several controls are pushed.
    return std::tanh(x);
}

float LTAProcessor::lowpass(float input, float& memory, float cutoff) const noexcept
{
    const float f = juce::jlimit(20.0f, static_cast<float>(sr * 0.45), cutoff);
    const float a = 1.0f - std::exp(-2.0f * pi * f / static_cast<float>(sr));
    memory += a * (input - memory);
    return memory;
}

float LTAProcessor::readDelay(int channel, float samplesBack) const noexcept
{
    if (delayLength < 8)
        return 0.0f;
    samplesBack = juce::jlimit(2.0f, static_cast<float>(delayLength - 3), samplesBack);
    float read = static_cast<float>(writePos) - samplesBack;
    if (read < 0.0f) read += static_cast<float>(delayLength);
    const int index = static_cast<int>(read);
    const int next = (index + 1) % delayLength;
    const float fraction = read - static_cast<float>(index);
    const auto& d = delay[static_cast<size_t>(channel)];
    return lerp(d[static_cast<size_t>(index)], d[static_cast<size_t>(next)], fraction);
}

void LTAProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (delayLength < 8)
        return;

    const int mainChannels = juce::jmin(getMainBusNumInputChannels(), 2);
    if (mainChannels < 1)
        return;
    const int numSamples = buffer.getNumSamples();
    auto mainAudio = getBusBuffer(buffer, true, 0);

#if LTA_PLUGIN_INDEX == 7
    const bool sideEnabled = getBus(true, 1) != nullptr && getBus(true, 1)->isEnabled();
    auto sideAudio = sideEnabled ? getBusBuffer(buffer, true, 1)
                                 : juce::AudioBuffer<float>();
#endif

    const auto& ids = lta::parameterIDs();
    const float targetAmount = parameters.getRawParameterValue(ids[0])->load();
    const float targetTone = parameters.getRawParameterValue(ids[1])->load();
    const float targetSpace = parameters.getRawParameterValue(ids[2])->load();
    const float targetMotion = parameters.getRawParameterValue(ids[3])->load();
    const float targetMix = parameters.getRawParameterValue(ids[4])->load();
    const float targetOutput = parameters.getRawParameterValue(ids[5])->load();
    float peak = 0.0f;
    float scanPeak = 0.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        constexpr float smooth = 0.0018f;
        smoothingAmount = lerp(smoothingAmount, targetAmount, smooth);
        smoothingTone = lerp(smoothingTone, targetTone, smooth);
        smoothingSpace = lerp(smoothingSpace, targetSpace, smooth);
        smoothingMotion = lerp(smoothingMotion, targetMotion, smooth);
        smoothingMix = lerp(smoothingMix, targetMix, smooth);
        smoothingOutput = lerp(smoothingOutput, targetOutput, smooth);

        const float a = smoothingAmount;
        const float b = smoothingTone;
        const float c = smoothingSpace;
        const float d = smoothingMotion;
        const float wet = smoothingMix;
        const float gain = juce::Decibels::decibelsToGain(smoothingOutput);
        const float rate = static_cast<float>(sr);
        const float t = static_cast<float>(phase);
        std::array<float, 2> dry {};
        for (int ch = 0; ch < mainChannels; ++ch)
            dry[static_cast<size_t>(ch)] = mainAudio.getSample(ch, sample);

        float sideLevel = 0.0f;
#if LTA_PLUGIN_INDEX == 7
        if (sideEnabled && sideAudio.getNumChannels() > 0)
        {
            for (int ch = 0; ch < sideAudio.getNumChannels(); ++ch)
                sideLevel += std::abs(sideAudio.getSample(ch, sample));
            sideLevel /= static_cast<float>(sideAudio.getNumChannels());
        }
        else
            sideLevel = std::abs(dry[0]);
#endif

        const float inputLevel = std::abs(dry[0]);
        envelope = lerp(envelope, inputLevel, inputLevel > envelope ? 0.07f : 0.002f);
#if LTA_PLUGIN_INDEX == 7
        const float release = lerp(0.002f, 0.00007f, c);
        sideEnvelope = lerp(sideEnvelope, sideLevel, sideLevel > sideEnvelope ? 0.09f : release);
        scanPeak = juce::jmax(scanPeak, sideEnvelope);
#endif

        const float grainLength = rate * lerp(0.08f, 0.36f, b);
        const float grainPosition = static_cast<float>(grainProgress);
        const float grainWindow = 0.5f - 0.5f * std::cos(2.0f * pi * clamp01(grainPosition / grainLength));

        for (int ch = 0; ch < mainChannels; ++ch)
        {
            const float x = dry[static_cast<size_t>(ch)];
            float y = x;
            const float widthPhase = (ch == 0 ? -1.0f : 1.0f);

            switch (lta::pluginIndex)
            {
                case 0: // SKINWALKER: formant-shaped, robot, whisper and monster modulation.
                {
                    const float formantHz = 190.0f + b * 2100.0f;
                    const float low = lowpass(x, filterA[static_cast<size_t>(ch)], formantHz);
                    const float band = low - lowpass(low, filterB[static_cast<size_t>(ch)], 130.0f + b * 260.0f);
                    const float robot = saturate(band * 3.4f) * std::sin(t * (25.0f + c * 85.0f));
                    const float whisper = randomBipolar() * clamp01(std::abs(x) * 5.0f) * 0.64f;
                    const float monster = saturate(low * 2.7f) * (0.70f + 0.30f * std::cos(t * (28.0f + d * 25.0f)));
                    const float mode = a * 3.0f;
                    if (mode < 1.0f) y = lerp(x, robot, mode);
                    else if (mode < 2.0f) y = lerp(robot, whisper, mode - 1.0f);
                    else y = lerp(whisper, monster, mode - 2.0f);
                    break;
                }
                case 1: // DREAM JOURNAL: shuffled, softly enveloped memory grains.
                {
                    const float fragment = readDelay(ch, grainBase + grainPosition * grainRate);
                    const float echo = readDelay(ch, rate * (0.4f + c * 1.8f));
                    const float hazy = lowpass(fragment, filterA[static_cast<size_t>(ch)], 1100.0f + c * 8500.0f);
                    y = x * (1.0f - 0.60f * a) + (hazy * grainWindow + echo * 0.18f) * (0.55f + a);
                    break;
                }
                case 2: // MOSHPIT: transient weight, compression, saturation and stereo movement.
                {
                    const float transient = x - lowpass(x, filterA[static_cast<size_t>(ch)], 90.0f + b * 650.0f);
                    const float attack = x + transient * b * 1.8f;
                    const float threshold = 0.40f - a * 0.33f;
                    const float compression = envelope > threshold ? threshold / juce::jmax(envelope, 0.0001f) : 1.0f;
                    const float body = saturate(attack * (1.0f + a * 10.0f)) * compression;
                    const float upper = body - lowpass(body, filterB[static_cast<size_t>(ch)], 150.0f);
                    y = body + upper * d * 0.12f * widthPhase * std::sin(t * 4.0f);
                    break;
                }
                case 3: // ECLIPSE: crossfade between low-pass and high-pass shadows.
                {
                    const float cutoff = 75.0f * std::pow(230.0f, b);
                    const float low = lowpass(x, filterA[static_cast<size_t>(ch)], cutoff);
                    const float high = x - lowpass(x, filterB[static_cast<size_t>(ch)], cutoff);
                    const float darkened = lerp(low, high, c);
                    const float resonant = darkened + (x - darkened) * d * 0.21f;
                    y = lerp(x, resonant, a);
                    break;
                }
                case 4: // REDACTED: regular rhythmic mutes, filtered gaps and repeats.
                {
                    const int subdivision = 2 + static_cast<int>(b * 13.0f);
                    const float rateHz = static_cast<float>(subdivision) * 0.5f;
                    const float stepPosition = std::fmod(static_cast<float>(timeSamples) * rateHz / rate, 1.0f);
                    const bool blackedOut = stepPosition < a * 0.85f;
                    const float repeat = readDelay(ch, rate / rateHz * (0.25f + 0.3f * d));
                    const float muted = lerp(0.0f, repeat * (0.35f + 0.4f * d), c);
                    y = blackedOut ? muted : x;
                    // 3ms smoothing at gate edges to avoid hard cuts.
                    y = lowpass(y, filterA[static_cast<size_t>(ch)], 6500.0f + (1.0f - c) * 10000.0f);
                    break;
                }
                case 5: // SECOND SUN: harmonic parallel signal with safe widening.
                {
                    const float low = lowpass(x, filterA[static_cast<size_t>(ch)], 160.0f + b * 600.0f);
                    const float upper = x - low;
                    const float excited = saturate(upper * (2.0f + a * 8.0f));
                    const float doubled = readDelay(ch, rate * (0.011f + 0.007f * c + widthPhase * 0.001f * d));
                    const float glow = 0.70f * excited + 0.30f * doubled;
                    y = x + glow * a * 0.65f;
                    break;
                }
                case 6: // NIGHTMARE FUEL: resonant dark echoes and ominous ring movement.
                {
                    const float lag = rate * (0.12f + c * 1.4f) + std::sin(t * (0.4f + d * 5.0f)) * 230.0f * d;
                    const float ghost = readDelay(ch, lag);
                    const float low = lowpass(ghost, filterA[static_cast<size_t>(ch)], 280.0f + (1.0f - b) * 5100.0f);
                    const float eerie = low * std::cos(t * (17.0f + d * 40.0f));
                    y = saturate(x * (1.0f + a)) * 0.4f + eerie * (0.4f + a * 0.8f);
                    break;
                }
                case 7: // X-RAY: sidechain responsive low-band reduction.
                {
                    const float cutoff = 45.0f + b * 250.0f;
                    const float low = lowpass(x, filterA[static_cast<size_t>(ch)], cutoff);
                    const float high = x - low;
                    const float trigger = clamp01(sideEnvelope * (2.0f + d * 11.0f));
                    const float attenuation = juce::jlimit(0.08f, 1.0f, 1.0f - a * trigger * 0.94f);
                    y = high + low * attenuation;
                    break;
                }
                default: // FALSE MEMORY: unstable faded filtering and ghost echo.
                {
                    const float fadeHz = 1200.0f + (1.0f - b) * 10000.0f - a * 850.0f;
                    const float soft = lowpass(x, filterA[static_cast<size_t>(ch)], fadeHz);
                    const float wow = std::sin(t * (0.17f + c * 2.3f)) * c * 110.0f;
                    const float aged = readDelay(ch, 80.0f + wow + c * 160.0f);
                    const float ghost = readDelay(ch, rate * (0.17f + d * 0.8f));
                    y = lerp(x, soft * 0.65f + aged * 0.22f + ghost * 0.27f, a);
                    break;
                }
            }

            const float result = lerp(x, y, wet) * gain;
            const float safe = std::isfinite(result) ? juce::jlimit(-3.0f, 3.0f, result) : 0.0f;
            mainAudio.setSample(ch, sample, safe);
            peak = juce::jmax(peak, std::abs(safe));

            // Written once after reading every delayed tap for this channel.
            const float feed = (lta::pluginIndex == 6) ? saturate(x + y * 0.15f) : x;
            delay[static_cast<size_t>(ch)][static_cast<size_t>(writePos)] = feed;
            previousIn[static_cast<size_t>(ch)] = x;
        }

        // Mono processing needs a matching second channel history for future layouts.
        if (mainChannels == 1)
            delay[1][static_cast<size_t>(writePos)] = delay[0][static_cast<size_t>(writePos)];

        if (++writePos >= delayLength) writePos = 0;
        ++timeSamples;
        phase += 2.0 * juce::MathConstants<double>::pi / sr;
        if (phase > 2.0 * juce::MathConstants<double>::pi * 10000.0)
            phase = std::fmod(phase, 2.0 * juce::MathConstants<double>::pi);

        grainProgress += 1.0;
        if (grainProgress >= grainLength)
        {
            grainProgress = 0.0;
            grainBase = rate * (0.10f + 2.3f * (randomBipolar() * 0.5f + 0.5f));
            grainRate = lerp(0.55f, 1.55f, randomBipolar() * 0.5f + 0.5f) *
                        (0.8f + 0.4f * d);
        }
    }

    const float previous = levelMeter.load(std::memory_order_relaxed);
    levelMeter.store(juce::jmax(peak, previous * 0.88f), std::memory_order_relaxed);
#if LTA_PLUGIN_INDEX == 7
    scanMeter.store(juce::jmax(scanPeak, scanMeter.load(std::memory_order_relaxed) * 0.86f),
                    std::memory_order_relaxed);
#endif
}

void LTAProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = parameters.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, dest);
}

void LTAProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* LTAProcessor::createEditor()
{
    return new LTAEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LTAProcessor();
}
