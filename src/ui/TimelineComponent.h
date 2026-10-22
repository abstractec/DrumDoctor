#pragma once

#include "analysis/PlayabilityAnalyzer.h"

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class TimelineComponent : public juce::Component,
                          public juce::FileDragAndDropTarget
{
public:
    void setAnalysis (std::shared_ptr<const drumdoctor::AnalysisResult> next);
    void setPlayhead (double ppq, bool show, bool follow);
    void setSelectedViolation (int index);
    void scrollToPpq (double ppq);

    std::function<void (const juce::StringArray& files)> onFilesDropped;

    void paint (juce::Graphics& g) override;
    void mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    float plotWidth() const;
    double contentEnd() const;
    float xForPpq (double ppq) const;
    void clampView();

    std::shared_ptr<const drumdoctor::AnalysisResult> analysis;
    double viewStart = 0.0;
    double viewLength = 8.0;
    bool userAdjusted = false;
    bool showPlayhead = false;
    double playheadPpq = 0.0;
    int selectedViolation = -1;
    juce::Point<float> dragOrigin;
    double dragViewStart = 0.0;

    static constexpr float gutter = 72.0f;
    static constexpr float ruler = 22.0f;
};
