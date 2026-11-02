#include "LTAEditor.h"
#include <cmath>

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;
constexpr float twopi = 2.0f * pi;
float clamp(float v) { return juce::jlimit(0.0f, 1.0f, v); }

void ring(juce::Graphics& g, juce::Point<float> c, float r,
          juce::Colour colour, float alpha, float width)
{
    g.setColour(colour.withMultipliedAlpha(alpha));
    g.drawEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f, width);
}

void line(juce::Graphics& g, float x1, float y1, float x2, float y2,
          juce::Colour colour, float alpha, float thickness = 1.0f)
{
    g.setColour(colour.withMultipliedAlpha(alpha));
    g.drawLine(x1, y1, x2, y2, thickness);
}

void monoText(juce::Graphics& g, const juce::String& text,
              juce::Rectangle<int> area, float size, juce::Colour colour)
{
    g.setColour(colour);
    g.setFont(juce::Font("Consolas", size, juce::Font::plain));
    g.drawText(text, area, juce::Justification::centred);
}
}

void LTADialLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y,
                                          int width, int height, float pos,
                                          float start, float end, juce::Slider&)
{
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x),
        static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.37f;
    const auto centre = bounds.getCentre();
    const float angle = start + pos * (end - start);

    g.setColour(juce::Colour(0xff080a0c));
    g.fillEllipse(centre.x - radius + 2.0f, centre.y - radius + 4.0f,
                  radius * 2.0f, radius * 2.0f);
    g.setColour(juce::Colour(0xff32383b));
    g.fillEllipse(centre.x - radius, centre.y - radius,
                  radius * 2.0f, radius * 2.0f);
    g.setColour(juce::Colour(0xff555e61));
    g.drawEllipse(centre.x - radius, centre.y - radius,
                  radius * 2.0f, radius * 2.0f, 1.0f);

    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y,
        radius + 5.0f, radius + 5.0f, 0.0f, start, angle, true);
    g.setColour(accent);
    g.strokePath(arc, juce::PathStrokeType(3.0f));

    const float dx = std::sin(angle) * radius * 0.68f;
    const float dy = -std::cos(angle) * radius * 0.68f;
    line(g, centre.x, centre.y, centre.x + dx, centre.y + dy, accent, 0.98f, 3.0f);
    g.setColour(accent);
    g.fillEllipse(centre.x - 2.0f, centre.y - 2.0f, 4.0f, 4.0f);
}

LTAEditor::LTAEditor(LTAProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p),
      profile(lta::profile()), look(profile.accent)
{
    const auto& ids = lta::parameterIDs();
    for (int i = 0; i < lta::controlCount; ++i)
    {
        const auto index = static_cast<size_t>(i);
        sliders[index] = std::make_unique<juce::Slider>();
        auto& s = *sliders[index];
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 21);
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff20252a));
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setDoubleClickReturnValue(true, i == 5 ? 0.0 : i == 4 ? 0.65 : 0.45);
        s.setLookAndFeel(&look);
        addAndMakeVisible(s);

        labels[index] = std::make_unique<juce::Label>();
        labels[index]->setText(profile.labels[index], juce::dontSendNotification);
        labels[index]->setJustificationType(juce::Justification::centred);
        labels[index]->setColour(juce::Label::textColourId,
                                 juce::Colour(0xffc6c9c9));
        labels[index]->setFont(juce::Font("Consolas", 13.0f, juce::Font::bold));
        addAndMakeVisible(*labels[index]);

        attachments[index] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, ids[index], s);
    }

    setOpaque(true);
    setSize(920, 600);
    setResizable(false, false);
    startTimerHz(30);
}

LTAEditor::~LTAEditor()
{
    stopTimer();
    // Attachments must release their slider references before look-and-feel reset.
    for (auto& a : attachments) a.reset();
    for (auto& s : sliders) s->setLookAndFeel(nullptr);
}

void LTAEditor::resized()
{
    constexpr int count = lta::controlCount;
    constexpr int left = 26;
    constexpr int usable = 868;
    constexpr int cell = usable / count;
    for (int i = 0; i < count; ++i)
    {
        const int x = left + i * cell;
        sliders[static_cast<size_t>(i)]->setBounds(x + 24, 443, cell - 48, 108);
        labels[static_cast<size_t>(i)]->setBounds(x + 4, 542, cell - 8, 24);
    }
}

void LTAEditor::timerCallback()
{
    seconds += 1.0f / 30.0f;
    if (seconds > 10000.0f) seconds = 0.0f;
    repaint(0, 0, getWidth(), 430);
}

void LTAEditor::drawHeader(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xfff1f2ef));
    g.setFont(juce::Font("Arial", 29.0f, juce::Font::bold));
    g.drawText("LTA " + juce::String(profile.title), 35, 25, 750, 41,
               juce::Justification::centredLeft);

    g.setColour(profile.accent.withAlpha(0.80f));
    g.setFont(juce::Font("Consolas", 12.0f, juce::Font::plain));
    g.drawText(profile.subtitle, 36, 68, 700, 23,
               juce::Justification::centredLeft);

    monoText(g, "LABS / VOL 01   V1.0", {698, 63, 184, 22},
             11.0f, profile.accent.withAlpha(0.65f));
    line(g, 32.0f, 97.0f, 888.0f, 97.0f, profile.accent, 0.22f);
}

void LTAEditor::drawArtwork(juce::Graphics& g, juce::Rectangle<float> r, float energy)
{
    const auto accent = profile.accent;
    const float t = seconds;
    const float strength = 0.2f + clamp(energy) * 0.8f;
    const float a = processor.parameters.getRawParameterValue("amount")->load();
    const float b = processor.parameters.getRawParameterValue("tone")->load();
    const float c = processor.parameters.getRawParameterValue("space")->load();
    const float d = processor.parameters.getRawParameterValue("motion")->load();
    const auto center = r.getCentre();

    g.setColour(juce::Colour(0xff0a0e12).withAlpha(0.78f));
    g.fillRoundedRectangle(r, 16.0f);
    g.setColour(accent.withAlpha(0.16f));
    g.drawRoundedRectangle(r, 16.0f, 1.0f);

    g.saveState();
    g.reduceClipRegion(r.reduced(2.0f).toNearestInt());

    switch (lta::pluginIndex)
    {
        case 0: // Shapeshifting face.
        {
            const float wiggle = std::sin(t * 2.0f) * (5.0f + a * 11.0f);
            juce::Path face;
            face.startNewSubPath(center.x, center.y - 104.0f);
            face.cubicTo(center.x + 85.0f + wiggle, center.y - 96.0f,
                         center.x + 82.0f + a * 18.0f, center.y + 53.0f,
                         center.x, center.y + 112.0f);
            face.cubicTo(center.x - 82.0f - a * 18.0f, center.y + 53.0f,
                         center.x - 85.0f - wiggle, center.y - 96.0f,
                         center.x, center.y - 104.0f);
            g.setColour(accent.withAlpha(0.14f + 0.14f * strength));
            g.fillPath(face);
            g.setColour(accent.withAlpha(0.8f));
            g.strokePath(face, juce::PathStrokeType(2.5f));
            for (int eye = -1; eye <= 1; eye += 2)
            {
                float x = center.x + eye * (30.0f + wiggle);
                g.setColour(accent.withAlpha(0.65f));
                g.fillRoundedRectangle(x - 14.0f, center.y - 22.0f,
                                       27.0f, 4.0f + strength * 5.0f, 3.0f);
            }
            line(g, center.x - 27.0f, center.y + 54.0f,
                 center.x + 27.0f, center.y + 54.0f + a * 18.0f,
                 accent, 0.8f, 2.4f);
            for (int i = 0; i < 11; ++i)
            {
                float y = center.y - 90.0f + i * 17.0f;
                line(g, center.x + 100.0f, y,
                     center.x + 100.0f + std::sin(t * 3.0f + i) * (a * 40.0f), y,
                     accent, 0.3f);
            }
            break;
        }
        case 1: // Fragments of a memory journal.
        {
            for (int i = 0; i < 6; ++i)
            {
                const float wobble = std::sin(t * 0.7f + i * 1.25f) * (4.0f + d * 15.0f);
                const float x = r.getX() + 62.0f + (i % 3) * 215.0f;
                const float y = r.getY() + 33.0f + (i / 3) * 119.0f + wobble;
                const juce::Rectangle<float> photo(x, y, 172.0f, 93.0f);
                g.setColour(juce::Colour(0xff303043));
                g.fillRoundedRectangle(photo, 3.0f);
                g.setColour(accent.withAlpha(0.35f));
                g.drawRoundedRectangle(photo, 3.0f, 1.5f);
                juce::Path wave;
                for (int n = 0; n < 40; ++n)
                {
                    const float xx = photo.getX() + 7.0f + n * 4.0f;
                    const float yy = photo.getCentreY() +
                        std::sin(n * 0.48f + t * (0.2f + b)) *
                        (5.0f + a * 23.0f) * std::sin(n * 0.13f);
                    if (n == 0) wave.startNewSubPath(xx, yy);
                    else wave.lineTo(xx, yy);
                }
                g.setColour(accent.withAlpha(0.5f + strength * 0.35f));
                g.strokePath(wave, juce::PathStrokeType(1.5f));
                monoText(g, "MEMORY 0" + juce::String(i + 1),
                    {static_cast<int>(x + 7), static_cast<int>(y + 69), 157, 18},
                    10.0f, accent.withAlpha(0.6f));
            }
            break;
        }
        case 2: // Reactive pit of tiny crowd figures.
        {
            const float cx = center.x;
            for (int i = 0; i < 70; ++i)
            {
                const int col = i % 14;
                const int row = i / 14;
                const float crowdX = cx - 290.0f + col * 45.0f + (row % 2) * 10.0f;
                const float crowdY = center.y - 89.0f + row * 47.0f;
                const float jump = std::abs(std::sin(t * (1.0f + b * 3.0f) + i * 1.1f)) *
                                    (3.0f + strength * a * 23.0f);
                g.setColour(accent.withAlpha(0.2f + strength * 0.7f));
                g.fillEllipse(crowdX, crowdY - jump, 9.0f, 9.0f);
                line(g, crowdX + 4.0f, crowdY + 9.0f - jump,
                     crowdX + 4.0f, crowdY + 26.0f - jump, accent, 0.54f, 2.0f);
            }
            ring(g, center, 50.0f + strength * a * 90.0f,
                 accent, 0.12f + strength * 0.28f, 3.0f);
            break;
        }
        case 3: // Two eclipsing orbs.
        {
            const float radius = 91.0f + b * 36.0f;
            const float distance = (1.0f - a) * 225.0f;
            const float offset = std::sin(t * 0.35f) * d * 25.0f;
            const juce::Point<float> first(center.x - distance * 0.5f, center.y);
            const juce::Point<float> second(center.x + distance * 0.5f, center.y + offset);
            g.setColour(accent.withAlpha(0.16f));
            g.fillEllipse(first.x - radius - 10, first.y - radius - 10,
                          radius * 2 + 20, radius * 2 + 20);
            g.setColour(accent.withAlpha(0.84f));
            g.fillEllipse(first.x - radius, first.y - radius, radius * 2, radius * 2);
            g.setColour(juce::Colour(0xff090d12));
            g.fillEllipse(second.x - radius, second.y - radius, radius * 2, radius * 2);
            ring(g, second, radius, accent, 0.63f, 1.6f);
            ring(g, first, radius + 17.0f, accent, 0.2f, 1.0f);
            break;
        }
        case 4: // Redacted document with rhythmic scanner.
        {
            const juce::Rectangle<float> document(center.x - 265.0f, center.y - 115.0f,
                                                   530.0f, 230.0f);
            g.setColour(juce::Colour(0xffc8c7bc));
            g.fillRoundedRectangle(document, 4.0f);
            g.setColour(juce::Colour(0xff242424));
            g.setFont(juce::Font("Consolas", 18.0f, juce::Font::bold));
            g.drawText("CLASSIFIED / LTA - 118", document.toNearestInt().reduced(16, 8),
                       juce::Justification::topLeft);
            for (int i = 0; i < 7; ++i)
            {
                float barY = document.getY() + 53.0f + i * 24.0f;
                float barW = 170.0f + (i % 3) * 90.0f;
                g.setColour(juce::Colour(0xff55554f).withAlpha(0.50f));
                g.fillRect(document.getX() + 18.0f, barY + 5.0f, barW, 2.0f);
                if ((i % 5) < static_cast<int>(1.0f + a * 4.0f))
                {
                    g.setColour(juce::Colour(0xff111010));
                    g.fillRect(document.getX() + 18.0f, barY,
                               80.0f + barW * a, 17.0f);
                }
            }
            const float scannerX = document.getX() + 15.0f +
                std::fmod(t * (40.0f + b * 170.0f), document.getWidth() - 30.0f);
            line(g, scannerX, document.getY(), scannerX, document.getBottom(),
                 juce::Colour(0xffff524b), 0.86f, 3.0f);
            break;
        }
        case 5: // Twin suns responding to harmonics.
        {
            for (int i = 0; i < 2; ++i)
            {
                const float x = center.x + (i == 0 ? -130.0f : 130.0f);
                const float radius = (i == 0 ? 62.0f : 24.0f + 63.0f * a);
                const float drift = std::sin(t * 0.5f) * 12.0f * d;
                const juce::Point<float> sun(x, center.y + (i == 0 ? 0.0f : drift));
                for (int halo = 4; halo >= 0; --halo)
                {
                    float rr = radius + halo * (12.0f + b * 8.0f);
                    g.setColour(accent.withAlpha((0.035f + strength * 0.016f) * (5 - halo)));
                    g.fillEllipse(sun.x - rr, sun.y - rr, rr * 2.0f, rr * 2.0f);
                }
                g.setColour(accent.withAlpha(i == 0 ? 0.94f : 0.53f + a * 0.42f));
                g.fillEllipse(sun.x - radius, sun.y - radius, radius * 2, radius * 2);
            }
            line(g, center.x - 54.0f, center.y,
                 center.x + 70.0f, center.y, accent, 0.36f, 1.7f);
            break;
        }
        case 6: // Descending ominous corridor.
        {
            for (int i = 0; i < 12; ++i)
            {
                float depth = static_cast<float>(i) / 12.0f;
                float width = 680.0f * (1.0f - depth);
                float height = 255.0f * (1.0f - depth);
                float wiggle = std::sin(t * (0.4f + d * 1.4f) + i * 0.9f) * a * 19.0f;
                juce::Rectangle<float> door(center.x - width * 0.5f + wiggle,
                                             center.y - height * 0.5f,
                                             width, height);
                g.setColour(accent.withAlpha((0.07f + 0.47f * depth) * strength));
                g.drawRect(door, 1.0f + depth * 2.5f);
            }
            g.setColour(accent.withAlpha(0.3f * strength));
            g.fillEllipse(center.x - 12.0f, center.y - 12.0f, 24.0f, 24.0f);
            break;
        }
        case 7: // X-ray spectrum-inspired main/side energy scan.
        {
            const float sideEnergy = clamp(processor.scanLevel());
            const float baseline = r.getBottom() - 20.0f;
            for (int i = 0; i < 45; ++i)
            {
                float x = r.getX() + 25.0f + i * (r.getWidth() - 50.0f) / 45.0f;
                float shape = std::exp(-std::pow((i - 14.0f) / 11.0f, 2.0f));
                float oscillate = 0.5f + 0.5f * std::sin(t * 2.0f + i * 0.18f);
                float h = (16.0f + 155.0f * shape * (0.18f + strength)) *
                          (0.80f + oscillate * 0.20f);
                g.setColour(accent.withAlpha(0.26f + shape * 0.67f));
                g.fillRoundedRectangle(x, baseline - h, 7.0f, h, 2.0f);
                g.setColour(juce::Colour(0xffff9876).withAlpha(0.25f + sideEnergy * 0.5f));
                float sideH = 95.0f * shape * sideEnergy * (0.7f + a);
                g.fillRect(x, baseline - sideH, 3.0f, sideH);
            }
            const float scanX = r.getX() + std::fmod(t * (70.0f + d * 90.0f), r.getWidth());
            line(g, scanX, r.getY() + 4.0f, scanX, r.getBottom() - 4.0f,
                 accent, 0.74f, 2.0f);
            break;
        }
        default: // Multiple faded photos of the same memory.
        {
            for (int i = 4; i >= 0; --i)
            {
                const float opacity = (0.14f + 0.11f * (4 - i)) * (1.0f - a * i * 0.13f);
                const float drift = i * (9.0f + c * 12.0f);
                const juce::Rectangle<float> frame(center.x - 194.0f + drift,
                    center.y - 109.0f - drift * 0.3f, 388.0f, 218.0f);
                g.setColour(accent.withAlpha(opacity));
                g.drawRoundedRectangle(frame, 4.0f, 1.5f);
                juce::Path horizon;
                horizon.startNewSubPath(frame.getX() + 20, frame.getCentreY() + 34);
                horizon.cubicTo(frame.getX() + 80, frame.getCentreY() - 40,
                    frame.getX() + 190, frame.getCentreY() + 79,
                    frame.getRight() - 20, frame.getCentreY() - 15);
                g.strokePath(horizon, juce::PathStrokeType(1.0f + strength));
                ring(g, {frame.getRight() - 74.0f, frame.getY() + 71.0f},
                     30.0f + i * 4.0f, accent, opacity * 0.75f, 1.5f);
            }
            break;
        }
    }

    // Consistent audio input activity meter along the bottom of the artwork.
    const float fill = clamp(energy * 0.85f);
    g.setColour(accent.withAlpha(0.12f));
    g.fillRoundedRectangle(r.getX() + 18.0f, r.getBottom() - 13.0f,
                           r.getWidth() - 36.0f, 3.0f, 1.0f);
    g.setColour(accent.withAlpha(0.78f));
    g.fillRoundedRectangle(r.getX() + 18.0f, r.getBottom() - 13.0f,
                           juce::jmax(1.0f, (r.getWidth() - 36.0f) * fill), 3.0f, 1.0f);
    g.restoreState();
}

void LTAEditor::paint(juce::Graphics& g)
{
    g.fillAll(profile.background);
    drawHeader(g);
    const auto region = juce::Rectangle<float>(30.0f, 109.0f, 860.0f, 300.0f);
    drawArtwork(g, region, processor.level());

    g.setColour(juce::Colour(0xff101419));
    g.fillRoundedRectangle(juce::Rectangle<float>(28.0f, 432.0f, 864.0f, 154.0f), 13.0f);
    g.setColour(profile.accent.withAlpha(0.16f));
    g.drawRoundedRectangle(juce::Rectangle<float>(28.0f, 432.0f, 864.0f, 154.0f), 13.0f, 1.0f);
    monoText(g, "LTA LABS / CREATIVE AUDIO SYSTEMS", {30, 410, 360, 20},
             10.0f, profile.accent.withAlpha(0.54f));
}
