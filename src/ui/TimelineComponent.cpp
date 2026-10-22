#include "TimelineComponent.h"
#include "Palette.h"

#include <algorithm>
#include <cmath>

void TimelineComponent::setAnalysis (std::shared_ptr<const drumdoctor::AnalysisResult> next)
{
    const double previousEnd = analysis != nullptr ? analysis->endPpq : 0.0;
    const double previousStart = analysis != nullptr ? analysis->startPpq : 0.0;
    analysis = std::move (next);

    const double end = analysis != nullptr ? analysis->endPpq : 0.0;
    const double start = analysis != nullptr ? analysis->startPpq : 0.0;
    if (! userAdjusted || std::abs (end - previousEnd) > 1.0 || std::abs (start - previousStart) > 1.0)
    {
        viewStart = std::max (0.0, start - 0.5);
        viewLength = std::max (4.0, end - start + 2.0);
        userAdjusted = false;
    }

    clampView();
    repaint();
}

void TimelineComponent::setPlayhead (double ppq, bool show, bool follow)
{
    showPlayhead = show;
    playheadPpq = ppq;

    if (follow && (ppq < viewStart + viewLength * 0.05 || ppq > viewStart + viewLength * 0.9))
    {
        viewStart = std::max (0.0, ppq - viewLength * 0.15);
        clampView();
    }

    repaint();
}

void TimelineComponent::setSelectedViolation (int index)
{
    selectedViolation = index;
    repaint();
}

void TimelineComponent::scrollToPpq (double ppq)
{
    viewStart = std::max (0.0, ppq - viewLength * 0.25);
    userAdjusted = true;
    clampView();
    repaint();
}

float TimelineComponent::plotWidth() const
{
    return std::max (1.0f, static_cast<float> (getWidth()) - gutter - 8.0f);
}

double TimelineComponent::contentEnd() const
{
    const double end = analysis != nullptr ? analysis->endPpq + 1.0 : 4.0;
    return std::max (4.0, end);
}

float TimelineComponent::xForPpq (double ppq) const
{
    return gutter + static_cast<float> ((ppq - viewStart) / std::max (0.001, viewLength)) * plotWidth();
}

void TimelineComponent::clampView()
{
    const double end = contentEnd();
    viewLength = std::clamp (viewLength, 1.0, end);
    if (viewStart < 0.0)
        viewStart = 0.0;
    if (viewStart + viewLength > end)
        viewStart = std::max (0.0, end - viewLength);
}

void TimelineComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (drumdoctor::palette::panel());
    g.fillRoundedRectangle (bounds, 8.0f);

    auto plot = bounds.reduced (4.0f, 4.0f);
    plot.removeFromLeft (gutter);
    plot.removeFromTop (ruler);

    const int lanes = drumdoctor::laneCount();
    const float laneHeight = plot.getHeight() / static_cast<float> (lanes);

    g.setColour (drumdoctor::palette::background());
    g.fillRect (plot);

    int numerator = 4;
    int denominator = 4;
    if (analysis != nullptr && ! analysis->score.meters.empty())
    {
        numerator = std::max (1, analysis->score.meters.front().numerator);
        denominator = std::max (1, analysis->score.meters.front().denominator);
    }

    const double quartersPerBeat = 4.0 / denominator;
    const double quartersPerBar = numerator * quartersPerBeat;
    const int firstBeat = static_cast<int> (std::floor (viewStart / quartersPerBeat));
    const int lastBeat = static_cast<int> (std::ceil ((viewStart + viewLength) / quartersPerBeat));

    for (int beat = firstBeat; beat <= lastBeat; ++beat)
    {
        const double ppq = beat * quartersPerBeat;
        const float x = xForPpq (ppq);
        if (x < gutter || x > bounds.getRight())
            continue;

        const bool barLine = beat >= 0 && numerator > 0 && (beat % numerator) == 0;
        g.setColour (barLine ? drumdoctor::palette::line().brighter (0.4f) : drumdoctor::palette::line());
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());

        if (barLine)
        {
            const int barNumber = static_cast<int> (std::floor (ppq / quartersPerBar)) + 1;
            g.setColour (drumdoctor::palette::muted());
            g.setFont (drumdoctor::palette::font (11.0f));
            g.drawText (juce::String (barNumber), juce::roundToInt (x) + 4, 6, 40, 14,
                        juce::Justification::centredLeft);
        }
    }

    for (int lane = 0; lane < lanes; ++lane)
    {
        const float y = plot.getY() + laneHeight * static_cast<float> (lane);
        g.setColour (drumdoctor::palette::line());
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setColour (drumdoctor::palette::muted());
        g.setFont (drumdoctor::palette::font (12.0f));
        g.drawText (drumdoctor::laneName (static_cast<drumdoctor::Lane> (lane)),
                    8, juce::roundToInt (y), static_cast<int> (gutter - 12.0f), juce::roundToInt (laneHeight),
                    juce::Justification::centredLeft);
    }

    if (analysis != nullptr)
    {
        for (int i = 0; i < static_cast<int> (analysis->violations.size()); ++i)
        {
            const auto& violation = analysis->violations[(size_t) i];
            const float x = xForPpq (violation.ppq);
            const bool selected = i == selectedViolation;
            g.setColour (drumdoctor::palette::red().withAlpha (selected ? 0.55f : 0.28f));
            g.fillRect (x - 5.0f, plot.getY(), selected ? 12.0f : 10.0f, plot.getHeight());
        }

        for (const auto& hit : analysis->hits)
        {
            const float x = xForPpq (hit.ppq);
            if (x < plot.getX() - 8.0f || x > plot.getRight() + 8.0f)
                continue;

            const auto info = drumdoctor::infoForNote (hit.note);
            const int laneIndex = static_cast<int> (info.lane);
            const float y = plot.getY() + laneHeight * static_cast<float> (laneIndex) + laneHeight * 0.5f - 5.0f;

            juce::Colour colour = drumdoctor::palette::blue();
            switch (info.lane)
            {
                case drumdoctor::Lane::feet: colour = drumdoctor::palette::amber(); break;
                case drumdoctor::Lane::snare: colour = drumdoctor::palette::ink(); break;
                case drumdoctor::Lane::toms: colour = juce::Colour (0xffd08b5b); break;
                case drumdoctor::Lane::hat: colour = drumdoctor::palette::green(); break;
                case drumdoctor::Lane::cymbals: colour = drumdoctor::palette::blue(); break;
            }

            g.setColour (colour);
            g.fillRoundedRectangle (x, y, 8.0f, 10.0f, 2.0f);
        }
    }

    if (showPlayhead)
    {
        const float x = xForPpq (playheadPpq);
        if (x >= plot.getX() && x <= plot.getRight())
        {
            g.setColour (drumdoctor::palette::amber());
            g.drawLine (x, plot.getY(), x, plot.getBottom(), 2.0f);
        }
    }

    if (analysis == nullptr || ! analysis->loaded)
    {
        g.setColour (drumdoctor::palette::muted());
        g.setFont (drumdoctor::palette::font (16.0f));
        g.drawText ("Drop a MIDI drum file here", plot, juce::Justification::centred);
    }
    else if (analysis->hits.empty())
    {
        g.setColour (drumdoctor::palette::muted());
        g.setFont (drumdoctor::palette::font (16.0f));
        g.drawText ("No General MIDI kit notes in this file", plot, juce::Justification::centred);
    }
}

void TimelineComponent::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (std::abs (wheel.deltaY) < 0.001f && std::abs (wheel.deltaX) < 0.001f)
        return;

    if (std::abs (wheel.deltaX) > std::abs (wheel.deltaY))
    {
        viewStart -= static_cast<double> (wheel.deltaX) * viewLength * 0.25;
    }
    else
    {
        const double anchor = viewStart + static_cast<double> ((event.position.x - gutter) / plotWidth()) * viewLength;
        const double factor = wheel.deltaY > 0.0f ? 0.85 : 1.15;
        viewLength *= factor;
        viewStart = anchor - static_cast<double> ((event.position.x - gutter) / plotWidth()) * viewLength;
    }

    userAdjusted = true;
    clampView();
    repaint();
}

void TimelineComponent::mouseDown (const juce::MouseEvent& event)
{
    dragOrigin = event.position;
    dragViewStart = viewStart;
}

void TimelineComponent::mouseDrag (const juce::MouseEvent& event)
{
    const double dx = event.position.x - dragOrigin.x;
    viewStart = dragViewStart - static_cast<double> (dx / plotWidth()) * viewLength;
    userAdjusted = true;
    clampView();
    repaint();
}

bool TimelineComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& file : files)
        if (file.endsWithIgnoreCase (".mid") || file.endsWithIgnoreCase (".midi"))
            return true;
    return false;
}

void TimelineComponent::filesDropped (const juce::StringArray& files, int, int)
{
    if (onFilesDropped != nullptr)
        onFilesDropped (files);
}
