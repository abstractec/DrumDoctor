#pragma once

#include "PluginProcessor.h"
#include "ui/DifficultyMeterComponent.h"
#include "ui/TimelineComponent.h"
#include "ui/ViolationListComponent.h"

class DrumDoctorEditor : public juce::AudioProcessorEditor,
                         public juce::FileDragAndDropTarget,
                         private juce::Timer
{
public:
    explicit DrumDoctorEditor (DrumDoctorProcessor&);
    ~DrumDoctorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;
    void refresh();
    void updateStatus();
    void updatePluginChrome();
    void beginHostCapture();
    void finishHostCapture();
    void loadFile (const juce::File& file);
    bool isHostedPlugin() const;
    static void styleButton (juce::TextButton& button);
    static void styleSlider (juce::Slider& slider);

    DrumDoctorProcessor& drum;
    std::shared_ptr<const drumdoctor::AnalysisResult> snapshot;
    uint32_t seenGeneration = 0;
    bool lastCaptureShown = true;
    bool analyseSawPlay = false;
    bool analyseStartedLive = false;
    double hostStoppedAtMs = 0.0;
    juce::String statusOverride;
    juce::String promptText;
    juce::String lastButtonText;
    juce::Colour promptColour { 0xffa89880 };
    juce::Rectangle<int> promptBounds;

    juce::Label title;
    juce::Label status;
    juce::Label clusterLabel;
    juce::Label offsetLabel;
    juce::TextButton loadButton { "Load" };
    juce::TextButton recordButton { "Record" };
    juce::TextButton playButton { "Play" };
    juce::Slider clusterSlider;
    juce::Slider offsetSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clusterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> offsetAttachment;
    TimelineComponent timeline;
    DifficultyMeterComponent meter;
    ViolationListComponent violations;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumDoctorEditor)
};
