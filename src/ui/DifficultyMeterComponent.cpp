#include "DifficultyMeterComponent.h"
#include "Palette.h"

#include <cmath>

namespace
{
void holdPeak (float& peak, float shown)
{
    if (shown >= peak)
        peak = shown;
    else
        peak = juce::jmax (0.0f, peak - 0.03f);
}

void smoothToward (float& shown, float target, float dtSeconds)
{
    const float seconds = target >= shown ? 0.06f : 0.32f;
    const float amount = 1.0f - std::exp (-dtSeconds / seconds);
    shown += (target - shown) * amount;
    if (std::abs (target - shown) < 0.0015f)
        shown = target;
}

juce::Colour zoneColour (float position)
{
    if (position >= 0.85f)
        return juce::Colour (0xffe23b2f);
    if (position >= 0.6f)
        return juce::Colour (0xffe6b03a);
    return juce::Colour (0xff3dce6e);
}
} // namespace

DifficultyMeterComponent::DifficultyMeterComponent()
    : labelFont (drumdoctor::palette::font (11.0f, true)),
      numberFont (juce::Font (juce::FontOptions (32.0f).withName (juce::Font::getDefaultMonospacedFontName())).boldened()),
      bandFont (drumdoctor::palette::font (13.0f, true))
{
    setOpaque (true);
}

void DifficultyMeterComponent::setPluginStyle (bool shouldUsePluginStyle)
{
    pluginStyle = shouldUsePluginStyle;
    repaint();
}

void DifficultyMeterComponent::setPoint (const drumdoctor::DifficultyPoint& next)
{
    point = next;

    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const float dt = lastSmoothMs <= 0.0
                         ? (1.0f / 60.0f)
                         : juce::jlimit (0.0f, 0.05f, static_cast<float> ((nowMs - lastSmoothMs) * 0.001));
    lastSmoothMs = nowMs;

    smoothToward (shownDifficulty,
                  next.impossible ? 1.0f : juce::jlimit (0.0f, 1.0f, next.difficulty / 100.0f),
                  dt);
    smoothToward (shownHands, juce::jlimit (0.0f, 1.0f, next.handLoad), dt);
    smoothToward (shownFeet, juce::jlimit (0.0f, 1.0f, next.footLoad), dt);
    holdPeak (peakDifficulty, shownDifficulty);
    holdPeak (peakHands, shownHands);
    holdPeak (peakFeet, shownFeet);

    repaint();
}

void DifficultyMeterComponent::paint (juce::Graphics& g)
{
    if (pluginStyle)
        paintPlugin (g);
    else
        paintStandalone (g);
}

void DifficultyMeterComponent::paintLedColumn (juce::Graphics& g, juce::Rectangle<float> column, float level, float peak)
{
    constexpr int segments = 22;
    const float gap = 2.0f;
    const float segmentHeight = (column.getHeight() - gap * static_cast<float> (segments - 1)) / static_cast<float> (segments);
    const float scaled = juce::jlimit (0.0f, 1.0f, level) * static_cast<float> (segments);
    const int full = static_cast<int> (std::floor (scaled));
    const float fraction = scaled - static_cast<float> (full);
    const int peakIndex = juce::roundToInt (juce::jlimit (0.0f, 1.0f, peak) * static_cast<float> (segments)) - 1;

    for (int i = 0; i < segments; ++i)
    {
        const float y = column.getBottom() - (static_cast<float> (i + 1) * segmentHeight) - (static_cast<float> (i) * gap);
        const float position = (static_cast<float> (i) + 0.5f) / static_cast<float> (segments);
        const auto on = zoneColour (position);
        float amount = 0.0f;
        if (i < full)
            amount = 1.0f;
        else if (i == full)
            amount = fraction;
        if (i == peakIndex)
            amount = 1.0f;

        g.setColour (on.withMultipliedBrightness (0.16f).interpolatedWith (on, amount));
        g.fillRoundedRectangle (column.getX(), y, column.getWidth(), segmentHeight, 1.0f);
    }
}

void DifficultyMeterComponent::paintPlugin (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff2c2823));
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colour (0xff161310));
    g.fillRoundedRectangle (bounds.reduced (1.5f), 7.0f);

    bounds = bounds.reduced (14.0f);
    auto header = bounds.removeFromTop (64.0f);

    g.setColour (juce::Colour (0xff8d8272));
    g.setFont (labelFont);
    g.drawText ("DIFFICULTY", header.removeFromTop (14.0f), juce::Justification::centredLeft);

    const float shownScore = shownDifficulty * 100.0f;
    const auto band = drumdoctor::bandFor (point.impossible ? 100.0f : shownScore, point.impossible);
    auto numberColour = zoneColour (point.impossible ? 1.0f : shownDifficulty);
    if (shownScore <= 0.5f && ! point.impossible)
        numberColour = juce::Colour (0xffd7ccb8);

    auto readout = header.removeFromTop (42.0f);
    g.setColour (numberColour);
    g.setFont (numberFont);
    g.drawText (juce::String (juce::roundToInt (shownScore)), readout.removeFromLeft (78.0f),
                juce::Justification::centredLeft);
    g.setFont (bandFont);
    g.drawText (band.toUpperCase(), readout, juce::Justification::centredLeft);

    bounds.removeFromTop (8.0f);

    auto drawColumn = [&] (juce::Rectangle<float> area, const juce::String& name, float level, float peak)
    {
        g.setColour (juce::Colour (0xff8d8272));
        g.setFont (labelFont);
        g.drawText (name, area.removeFromTop (14.0f), juce::Justification::centred);
        area = area.reduced (6.0f, 0.0f);
        g.setColour (juce::Colour (0xff0b0a09));
        g.fillRoundedRectangle (area.expanded (4.0f, 3.0f), 3.0f);
        paintLedColumn (g, area, level, peak);
    };

    auto columns = bounds;
    const float gap = 8.0f;
    const float width = (columns.getWidth() - gap * 2.0f) / 3.0f;
    drawColumn (columns.removeFromLeft (width), "LEVEL", shownDifficulty, peakDifficulty);
    columns.removeFromLeft (gap);
    drawColumn (columns.removeFromLeft (width), "HANDS", shownHands, peakHands);
    columns.removeFromLeft (gap);
    drawColumn (columns, "FEET", shownFeet, peakFeet);
}

void DifficultyMeterComponent::paintStandalone (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (drumdoctor::palette::panel());
    g.fillRoundedRectangle (bounds, 8.0f);

    bounds = bounds.reduced (14.0f);
    g.setColour (drumdoctor::palette::muted());
    g.setFont (drumdoctor::palette::font (12.0f, true));
    g.drawText ("DIFFICULTY", bounds.removeFromTop (16.0f), juce::Justification::centredLeft);

    const float shownScore = shownDifficulty * 100.0f;
    const auto band = drumdoctor::bandFor (point.impossible ? 100.0f : shownScore, point.impossible);
    auto numberColour = drumdoctor::palette::ink();
    if (point.impossible)
        numberColour = drumdoctor::palette::red();
    else if (shownScore >= 75.0f)
        numberColour = juce::Colour (0xffe07a3d);
    else if (shownScore >= 25.0f)
        numberColour = drumdoctor::palette::amber();
    else
        numberColour = drumdoctor::palette::green();

    auto readout = bounds.removeFromTop (78.0f);
    auto gauge = readout.removeFromLeft (36.0f).reduced (4.0f, 2.0f);
    g.setColour (drumdoctor::palette::background());
    g.fillRoundedRectangle (gauge, 4.0f);

    const float level = shownDifficulty;
    auto fill = gauge;
    fill.setTop (gauge.getBottom() - gauge.getHeight() * level);
    g.setColour (numberColour);
    g.fillRoundedRectangle (fill.reduced (3.0f), 3.0f);

    readout.removeFromLeft (8.0f);
    g.setColour (numberColour);
    g.setFont (drumdoctor::palette::font (42.0f, true));
    g.drawText (juce::String (juce::roundToInt (shownScore)), readout.removeFromTop (48.0f),
                juce::Justification::centredLeft);
    g.setColour (numberColour);
    g.setFont (drumdoctor::palette::font (16.0f, true));
    g.drawText (band, readout, juce::Justification::centredLeft);

    bounds.removeFromTop (8.0f);

    auto drawLoad = [&] (const juce::String& name, float load, juce::Colour colour)
    {
        auto row = bounds.removeFromTop (28.0f);
        g.setColour (drumdoctor::palette::muted());
        g.setFont (drumdoctor::palette::font (12.0f));
        g.drawText (name, row.removeFromTop (14.0f), juce::Justification::centredLeft);
        auto track = row.reduced (0.0f, 3.0f);
        g.setColour (drumdoctor::palette::background());
        g.fillRoundedRectangle (track, 3.0f);
        auto filled = track;
        filled.setWidth (track.getWidth() * juce::jlimit (0.0f, 1.0f, load));
        g.setColour (colour);
        g.fillRoundedRectangle (filled, 3.0f);
        bounds.removeFromTop (6.0f);
    };

    drawLoad ("Hands", shownHands, drumdoctor::palette::blue());
    drawLoad ("Feet", shownFeet, drumdoctor::palette::amber());
}
