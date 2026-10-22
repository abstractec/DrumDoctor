#pragma once

#include "analysis/PlayabilityAnalyzer.h"

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class ViolationListComponent : public juce::Component,
                               public juce::ListBoxModel
{
public:
    ViolationListComponent();
    ~ViolationListComponent() override;

    void setPluginStyle (bool shouldUsePluginStyle);
    void setEmptyMessage (const juce::String& text);
    void setViolations (const std::vector<drumdoctor::Violation>& next);
    std::function<void (int index)> onSelect;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label header;
    juce::Label emptyLabel;
    juce::ListBox list { "violations", nullptr };
    std::vector<drumdoctor::Violation> violations;
    bool pluginStyle = false;
};
