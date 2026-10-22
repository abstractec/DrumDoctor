#pragma once

#include "DrumMap.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>
#include <optional>
#include <vector>

namespace drumdoctor
{

struct DrumHit
{
    double timeSeconds = 0.0;
    double ppq = 0.0;
    int note = 0;
    double receivedMs = 0.0;
};

struct TempoChange
{
    double ppq = 0.0;
    double bpm = 120.0;
};

struct TimeSignatureChange
{
    double ppq = 0.0;
    int numerator = 4;
    int denominator = 4;
};

struct ScoreMap
{
    std::vector<TempoChange> tempos { { 0.0, 120.0 } };
    std::vector<TimeSignatureChange> meters { { 0.0, 4, 4 } };
};

struct AnalysisSettings
{
    double clusterWindowSeconds = 0.010;
    KitMap kitMap = KitMap::generalMidi;
    double curveEndPpq = -1.0e9;
};

struct Violation
{
    double timeSeconds = 0.0;
    double ppq = 0.0;
    int bar = 1;
    double beat = 1.0;
    int handCount = 0;
    int footCount = 0;
    juce::String reason;
};

struct DifficultyPoint
{
    double ppq = 0.0;
    float difficulty = 0.0f;
    float handLoad = 0.0f;
    float footLoad = 0.0f;
    bool impossible = false;
};

struct AnalysisResult
{
    bool loaded = false;
    KitMap kitMap = KitMap::generalMidi;
    double startPpq = 0.0;
    double endPpq = 0.0;
    int ignoredNotes = 0;
    ScoreMap score;
    std::vector<DrumHit> hits;
    std::vector<Violation> violations;
    std::vector<DifficultyPoint> curve;
};

struct LoadedClip
{
    std::vector<DrumHit> hits;
    ScoreMap score;
};

inline juce::String bandFor (float difficulty, bool impossible)
{
    if (impossible)
        return "Impossible";
    if (difficulty < 25.0f)
        return "Comfortable";
    if (difficulty < 50.0f)
        return "Moderate";
    if (difficulty < 75.0f)
        return "Demanding";
    return "Extreme";
}

inline juce::String formatBeat (double beat)
{
    const auto nearest = std::round (beat);
    if (std::abs (beat - nearest) < 0.05)
        return juce::String (static_cast<int> (nearest));
    return juce::String (beat, 2);
}

inline juce::String violationLabel (const Violation& violation)
{
    return "Bar " + juce::String (violation.bar) + ", beat " + formatBeat (violation.beat)
         + "   " + violation.reason;
}

double secondsAtPpq (const ScoreMap& score, double ppq);

AnalysisResult analyzeHits (const std::vector<DrumHit>& hits,
                            const ScoreMap& score,
                            const AnalysisSettings& settings);

AnalysisResult analyzeSequence (const juce::MidiMessageSequence& sequence,
                                double bpm,
                                const AnalysisSettings& settings);

DifficultyPoint sampleCurve (const std::vector<DifficultyPoint>& curve, double ppq);

std::optional<LoadedClip> loadDrumClip (const juce::MemoryBlock& bytes);

juce::MemoryBlock midiFromHits (const std::vector<DrumHit>& hits, double bpm);

} // namespace drumdoctor
