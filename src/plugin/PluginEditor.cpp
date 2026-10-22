#include "PluginEditor.h"
#include "ui/Palette.h"

namespace
{
bool isDrumFile (const juce::String& path)
{
    return path.endsWithIgnoreCase (".mid") || path.endsWithIgnoreCase (".midi");
}
} // namespace

DrumDoctorEditor::DrumDoctorEditor (DrumDoctorProcessor& processorIn)
    : juce::AudioProcessorEditor (&processorIn),
      drum (processorIn)
{
    title.setText ("Drum Doctor", juce::dontSendNotification);
    title.setFont (drumdoctor::palette::font (22.0f, true));
    title.setColour (juce::Label::textColourId, drumdoctor::palette::ink());
    title.setInterceptsMouseClicks (false, false);

    status.setFont (drumdoctor::palette::font (14.0f));
    status.setColour (juce::Label::textColourId, drumdoctor::palette::muted());
    status.setInterceptsMouseClicks (false, false);

    clusterLabel.setText ("Cluster ms", juce::dontSendNotification);
    offsetLabel.setText ("Playhead offset", juce::dontSendNotification);
    kitMapLabel.setText ("MIDI map", juce::dontSendNotification);
    for (auto* label : { &clusterLabel, &offsetLabel, &kitMapLabel })
    {
        label->setFont (drumdoctor::palette::font (12.0f));
        label->setColour (juce::Label::textColourId, drumdoctor::palette::muted());
        label->setJustificationType (juce::Justification::centredRight);
    }

    styleButton (loadButton);
    styleButton (recordButton);
    styleButton (playButton);
    styleSlider (clusterSlider);
    styleSlider (offsetSlider);
    styleMapBox();

    addAndMakeVisible (title);
    addAndMakeVisible (status);
    addAndMakeVisible (clusterLabel);
    addAndMakeVisible (offsetLabel);
    addAndMakeVisible (kitMapLabel);
    addAndMakeVisible (kitMapBox);
    addAndMakeVisible (loadButton);
    addAndMakeVisible (recordButton);
    addAndMakeVisible (playButton);
    addAndMakeVisible (clusterSlider);
    addAndMakeVisible (offsetSlider);
    addAndMakeVisible (timeline);
    addAndMakeVisible (meter);
    addAndMakeVisible (violations);

    clusterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        drum.apvts, "clusterMs", clusterSlider);
    offsetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        drum.apvts, "ppqOffset", offsetSlider);
    kitMapAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        drum.apvts, "kitMap", kitMapBox);

    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Open a drum MIDI file", juce::File(), "*.mid;*.midi");
        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        chooser->launchAsync (flags, [safe = juce::Component::SafePointer<DrumDoctorEditor> (this)] (const juce::FileChooser& dialog)
        {
            if (safe == nullptr)
                return;

            const auto file = dialog.getResult();
            if (file.existsAsFile())
                safe->loadFile (file);
        });
    };

    recordButton.onClick = [this]
    {
        if (isHostedPlugin())
        {
            if (drum.isCapturing())
                finishHostCapture();
            else
                beginHostCapture();
        }
        else if (drum.isCapturing())
        {
            drum.stopCapture();
        }
        else
        {
            drum.startCapture();
        }

        refresh();
    };

    playButton.onClick = [this]
    {
        if (drum.isStandalonePlaying())
        {
            drum.setStandalonePlaying (false);
        }
        else
        {
            if (snapshot != nullptr && drum.getFilePlayheadPpq() > snapshot->endPpq + 0.05)
                drum.rewindStandalone();
            drum.setStandalonePlaying (true);
        }
        refresh();
    };

    playButton.setVisible (! isHostedPlugin());

    if (isHostedPlugin())
    {
        meter.setPluginStyle (true);
        violations.setPluginStyle (true);
        juce::Component* const hidden[] { &title, &status, &clusterLabel, &offsetLabel, &loadButton,
                                          &clusterSlider, &offsetSlider, &timeline };
        for (auto* control : hidden)
            control->setVisible (false);

        setResizeLimits (320, 660, 560, 1100);
        setSize (380, 800);
    }
    else
    {
        setResizeLimits (860, 560, 1800, 1200);
        setSize (1100, 700);
    }

    timeline.onFilesDropped = [this] (const juce::StringArray& files)
    {
        filesDropped (files, 0, 0);
    };

    violations.onSelect = [this] (int index)
    {
        if (snapshot == nullptr || index < 0 || index >= static_cast<int> (snapshot->violations.size()))
            return;

        timeline.setSelectedViolation (index);
        timeline.scrollToPpq (snapshot->violations[static_cast<size_t> (index)].ppq);
    };

    setResizable (true, true);

    if (drum.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        if (auto* app = juce::JUCEApplicationBase::getInstance())
        {
            for (const auto& arg : app->getCommandLineParameterArray())
            {
                const juce::File file (arg);
                if (file.existsAsFile() && isDrumFile (file.getFullPathName()))
                {
                    loadFile (file);
                    break;
                }
            }
        }
    }

    startTimerHz (60);
    refresh();
}

DrumDoctorEditor::~DrumDoctorEditor()
{
    stopTimer();
}

void DrumDoctorEditor::styleButton (juce::TextButton& button)
{
    button.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3a2e22));
    button.setColour (juce::TextButton::buttonOnColourId, drumdoctor::palette::amber());
    button.setColour (juce::TextButton::textColourOffId, drumdoctor::palette::ink());
    button.setColour (juce::TextButton::textColourOnId, drumdoctor::palette::background());
}

void DrumDoctorEditor::styleMapBox()
{
    kitMapBox.setColour (juce::ComboBox::backgroundColourId, drumdoctor::palette::background());
    kitMapBox.setColour (juce::ComboBox::textColourId, drumdoctor::palette::ink());
    kitMapBox.setColour (juce::ComboBox::outlineColourId, drumdoctor::palette::line());
    kitMapBox.setColour (juce::ComboBox::arrowColourId, drumdoctor::palette::amber());
    kitMapLabel.setJustificationType (juce::Justification::centredLeft);
}

void DrumDoctorEditor::styleSlider (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 58, 20);
    slider.setColour (juce::Slider::textBoxTextColourId, drumdoctor::palette::ink());
    slider.setColour (juce::Slider::textBoxBackgroundColourId, drumdoctor::palette::background());
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::thumbColourId, drumdoctor::palette::amber());
    slider.setColour (juce::Slider::trackColourId, drumdoctor::palette::line());
    slider.setColour (juce::Slider::backgroundColourId, drumdoctor::palette::line());
}

void DrumDoctorEditor::paint (juce::Graphics& g)
{
    g.fillAll (drumdoctor::palette::background());

    if (isHostedPlugin() && ! promptBounds.isEmpty())
    {
        g.setColour (promptColour);
        g.setFont (drumdoctor::palette::font (13.5f));
        g.drawFittedText (promptText, promptBounds, juce::Justification::centredLeft, 4, 1.0f);
    }
}

bool DrumDoctorEditor::isHostedPlugin() const
{
    return drum.wrapperType != juce::AudioProcessor::wrapperType_Standalone;
}

void DrumDoctorEditor::resized()
{
    if (isHostedPlugin())
    {
        auto area = getLocalBounds().reduced (12);
        auto strip = area.removeFromTop (176);
        recordButton.setBounds (strip.removeFromTop (40));
        strip.removeFromTop (8);
        auto mapRow = strip.removeFromTop (28);
        kitMapLabel.setBounds (mapRow.removeFromLeft (78));
        kitMapBox.setBounds (mapRow);
        promptBounds = strip.reduced (2, 8);
        meter.setBounds (area.removeFromTop (juce::jmax (220, area.getHeight() * 46 / 100)).reduced (0, 4));
        violations.setBounds (area.reduced (0, 4));
        recordButton.toFront (false);
        return;
    }

    auto area = getLocalBounds().reduced (14);
    auto header = area.removeFromTop (76);
    auto titleRow = header.removeFromTop (36);

    title.setBounds (titleRow.removeFromLeft (180));
    if (playButton.isVisible())
        playButton.setBounds (titleRow.removeFromRight (76).reduced (4, 4));
    recordButton.setBounds (titleRow.removeFromRight (116).reduced (4, 4));
    loadButton.setBounds (titleRow.removeFromRight (84).reduced (4, 4));
    status.setBounds (titleRow.reduced (8, 6));

    kitMapLabel.setBounds (header.removeFromLeft (72));
    kitMapBox.setBounds (header.removeFromLeft (210).reduced (4, 8));
    clusterLabel.setBounds (header.removeFromLeft (78));
    clusterSlider.setBounds (header.removeFromLeft (230).reduced (4, 6));
    offsetLabel.setBounds (header.removeFromLeft (110));
    offsetSlider.setBounds (header.removeFromLeft (230).reduced (4, 6));

    auto side = area.removeFromRight (300);
    meter.setBounds (side.removeFromTop (230).reduced (0, 6));
    violations.setBounds (side.reduced (0, 6));
    timeline.setBounds (area.reduced (0, 6));
}

void DrumDoctorEditor::timerCallback()
{
    refresh();
}

void DrumDoctorEditor::refresh()
{
    drum.drainIncoming();

    if (isHostedPlugin() && drum.isCapturing())
    {
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        const double staleMs = nowMs - drum.getPositionStampMs();
        const bool transportLive = drum.isTransportPlaying() && staleMs >= 0.0 && staleMs < 400.0;
        if (transportLive)
        {
            analyseSawPlay = true;
            hostStoppedAtMs = 0.0;
        }
        else if (analyseSawPlay && ! drum.isTransportPlaying())
        {
            if (hostStoppedAtMs <= 0.0)
                hostStoppedAtMs = nowMs;
            if (nowMs - hostStoppedAtMs > 180.0)
                finishHostCapture();
        }
        else if (analyseSawPlay && staleMs > 400.0)
        {
            finishHostCapture();
        }
    }

    const auto generation = drum.getAnalysisGeneration();
    if (generation != seenGeneration || snapshot == nullptr)
    {
        snapshot = drum.getAnalysis();
        seenGeneration = generation;
        timeline.setAnalysis (snapshot);
        violations.setViolations (snapshot != nullptr ? snapshot->violations : std::vector<drumdoctor::Violation> {});
    }

    if (drum.wrapperType == juce::AudioProcessor::wrapperType_Standalone
        && drum.isStandalonePlaying()
        && snapshot != nullptr
        && snapshot->loaded
        && drum.getFilePlayheadPpq() > snapshot->endPpq + 1.0)
        drum.setStandalonePlaying (false);

    meter.setPoint (drum.meterPoint());
    if (timeline.isVisible())
        timeline.setPlayhead (drum.getFilePlayheadPpq(),
                              snapshot != nullptr && snapshot->loaded,
                              drum.isTransportPlaying());

    if (isHostedPlugin())
    {
        updatePluginChrome();
        return;
    }

    const bool capturing = drum.isCapturing();
    playButton.setButtonText (drum.isStandalonePlaying() ? "Stop" : "Play");
    if (capturing != lastCaptureShown)
    {
        lastCaptureShown = capturing;
        recordButton.setButtonText (capturing ? "Stop" : "Record");
    }

    updateStatus();
}

void DrumDoctorEditor::beginHostCapture()
{
    statusOverride.clear();
    const double staleMs = juce::Time::getMillisecondCounterHiRes() - drum.getPositionStampMs();
    analyseStartedLive = drum.isTransportPlaying() && staleMs >= 0.0 && staleMs < 400.0;
    analyseSawPlay = analyseStartedLive;
    hostStoppedAtMs = 0.0;
    drum.startCapture();
}

void DrumDoctorEditor::finishHostCapture()
{
    const bool heardPlay = analyseSawPlay;
    analyseSawPlay = false;
    analyseStartedLive = false;
    hostStoppedAtMs = 0.0;
    const bool gotNotes = drum.stopCapture();

    if (! heardPlay)
        statusOverride = "Cancelled. Press Analyse track, then play the part from the start.";
    else if (! gotNotes)
        statusOverride = "No drum notes were heard. Play the part from the start, then stop.";
    else
        statusOverride.clear();
}

void DrumDoctorEditor::updatePluginChrome()
{
    const bool capturing = drum.isCapturing();
    juce::String buttonText = "Analyse track";
    auto buttonFill = drumdoctor::palette::amber();
    auto buttonInk = drumdoctor::palette::background();
    juce::String prompt;
    auto colour = drumdoctor::palette::muted();
    juce::String empty = "They will be listed here.";

    if (capturing && ! analyseSawPlay)
    {
        buttonText = "Cancel";
        buttonFill = juce::Colour (0xff3a2e22);
        buttonInk = drumdoctor::palette::ink();
        prompt = "Press play in Logic from the start of the part, with cycle off. Press stop when it ends, and every impossible hit is listed below.";
        colour = drumdoctor::palette::amber();
        empty = "Waiting for Logic to play.";
    }
    else if (capturing)
    {
        buttonText = "Stop and analyse";
        buttonFill = drumdoctor::palette::red();
        prompt = analyseStartedLive
                     ? "Listening from the current position, not from the start. Press stop in Logic when the part ends."
                     : "Listening. Press stop in Logic when the part ends.";
        colour = drumdoctor::palette::amber();
        empty = "Listening. The list appears when playback stops.";
    }
    else if (statusOverride.isNotEmpty())
    {
        prompt = statusOverride;
        colour = drumdoctor::palette::red();
        empty = "Nothing listed yet.";
    }
    else if (snapshot == nullptr || ! snapshot->loaded)
    {
        prompt = "Press Analyse track, then play the part once. Or drop a .mid file on this window.";
    }
    else if (snapshot->hits.empty())
    {
        buttonText = "Analyse again";
        prompt = "No drum-kit notes for the selected MIDI map.";
        colour = drumdoctor::palette::amber();
        empty = "No kit notes.";
    }
    else if (snapshot->violations.empty())
    {
        buttonText = "Analyse again";
        prompt = "Playable. Two hands and two feet can play everything in this pass.";
        colour = drumdoctor::palette::green();
        empty = "None on this pass.";
    }
    else
    {
        const auto count = static_cast<int> (snapshot->violations.size());
        buttonText = "Analyse again";
        prompt = juce::String (count) + (count == 1 ? " place a drummer cannot play." : " places a drummer cannot play.");
        colour = drumdoctor::palette::red();
    }

    if (! capturing && statusOverride.isEmpty() && snapshot != nullptr && snapshot->ignoredNotes > 0)
        prompt += " " + juce::String (snapshot->ignoredNotes) + (snapshot->ignoredNotes == 1 ? " other note was ignored."
                                                                                             : " other notes were ignored.");

    if (buttonText != lastButtonText)
    {
        lastButtonText = buttonText;
        recordButton.setButtonText (buttonText);
        recordButton.setColour (juce::TextButton::buttonColourId, buttonFill);
        recordButton.setColour (juce::TextButton::textColourOffId, buttonInk);
    }

    violations.setEmptyMessage (empty);

    if (prompt != promptText || colour != promptColour)
    {
        promptText = prompt;
        promptColour = colour;
        repaint();
    }
}

void DrumDoctorEditor::updateStatus()
{
    juce::String text = statusOverride;
    auto colour = drumdoctor::palette::muted();

    if (drum.isCapturing())
    {
        text = "Recording from the host";
        colour = drumdoctor::palette::amber();
    }
    else if (text.isEmpty())
    {
        if (snapshot == nullptr || ! snapshot->loaded)
            text = "Drop a MIDI file, or record from the host";
        else if (snapshot->hits.empty())
            text = "No kit notes for this MIDI map";
        else if (snapshot->violations.empty())
        {
            text = "Playable";
            colour = drumdoctor::palette::green();
        }
        else
        {
            text = juce::String (static_cast<int> (snapshot->violations.size()))
                 + (snapshot->violations.size() == 1 ? " problem" : " problems");
            colour = drumdoctor::palette::red();
        }
    }
    else
    {
        colour = drumdoctor::palette::red();
    }

    if (snapshot != nullptr && snapshot->ignoredNotes > 0)
        text += "  ·  " + juce::String (snapshot->ignoredNotes) + " notes ignored";

    status.setText (text, juce::dontSendNotification);
    status.setColour (juce::Label::textColourId, colour);
}

void DrumDoctorEditor::loadFile (const juce::File& file)
{
    if (drum.loadMidiFile (file))
        statusOverride.clear();
    else
        statusOverride = "Couldn't read that MIDI file";

    refresh();
}

bool DrumDoctorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& file : files)
        if (isDrumFile (file))
            return true;
    return false;
}

void DrumDoctorEditor::filesDropped (const juce::StringArray& files, int, int)
{
    for (const auto& path : files)
    {
        if (! isDrumFile (path))
            continue;

        loadFile (juce::File (path));
        return;
    }
}
