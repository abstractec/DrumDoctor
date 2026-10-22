#pragma once

#include "analysis/PlayabilityAnalyzer.h"

#include <juce_gui_basics/juce_gui_basics.h>

class DifficultyMeterComponent : public juce::Component
{
public:
    DifficultyMeterComponent();

    void setPluginStyle (bool shouldUsePluginStyle);
    void setPoint (const drumdoctor::DifficultyPoint& next);

    void paint (juce::Graphics& g) override;

private:
    void paintStandalone (juce::Graphics& g);
    void paintPlugin (juce::Graphics& g);
    static void paintLedColumn (juce::Graphics& g, juce::Rectangle<float> column, float level, float peak);

    drumdoctor::DifficultyPoint point;
    bool pluginStyle = false;
    juce::Font labelFont;
    juce::Font numberFont;
    juce::Font bandFont;
    float shownDifficulty = 0.0f;
    float shownHands = 0.0f;
    float shownFeet = 0.0f;
    float peakDifficulty = 0.0f;
    float peakHands = 0.0f;
    float peakFeet = 0.0f;
    double lastSmoothMs = 0.0;
};
