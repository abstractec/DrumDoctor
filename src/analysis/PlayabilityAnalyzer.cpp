#include "PlayabilityAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drumdoctor
{
namespace
{

juce::String joinNames (std::vector<juce::String> names)
{
    std::sort (names.begin(), names.end());
    juce::StringArray parts;
    for (const auto& name : names)
        parts.add (name);
    return parts.joinIntoString (", ");
}

juce::String kickPhrase (int kicks)
{
    switch (kicks)
    {
        case 1: return "one kick";
        case 2: return "two kicks";
        case 3: return "three kicks";
        default: return juce::String (kicks) + " kicks";
    }
}

struct BarBeat
{
    int bar = 1;
    double beat = 1.0;
};

BarBeat locate (const ScoreMap& score, double ppq)
{
    ppq = std::max (0.0, ppq);
    if (score.meters.empty())
        return {};

    const auto& meters = score.meters;
    int barCount = 1;
    for (size_t i = 0; i < meters.size(); ++i)
    {
        const double start = meters[i].ppq;
        const double next = (i + 1 < meters.size())
                                ? meters[i + 1].ppq
                                : std::numeric_limits<double>::infinity();
        const double quartersPerBar = meters[i].numerator * 4.0 / meters[i].denominator;
        const double quartersPerBeat = 4.0 / meters[i].denominator;
        if (quartersPerBar <= 0.0)
            continue;

        if (ppq < start)
            break;

        if (ppq < next)
        {
            const double into = ppq - start;
            const int barsIn = static_cast<int> (std::floor (into / quartersPerBar));
            const double beat = std::fmod (into, quartersPerBar) / quartersPerBeat + 1.0;
            return { barCount + barsIn, beat };
        }

        const double span = next - start;
        barCount += static_cast<int> (std::floor ((span + 1.0e-9) / quartersPerBar));
    }

    return {};
}

struct Cluster
{
    double timeSeconds = 0.0;
    double ppq = 0.0;
    std::vector<int> notes;
};

struct Summary
{
    double ppq = 0.0;
    int handHits = 0;
    int footTaps = 0;
    int hands = 0;
    int feet = 0;
    bool hatOccupied = false;
    bool violation = false;
};

} // namespace

double secondsAtPpq (const ScoreMap& score, double ppq)
{
    const double openingBpm = score.tempos.empty() ? 120.0 : std::max (1.0, score.tempos.front().bpm);
    if (ppq <= 0.0)
        return ppq * (60.0 / openingBpm);

    if (score.tempos.empty())
        return ppq * 0.5;

    double seconds = 0.0;
    for (size_t i = 0; i < score.tempos.size(); ++i)
    {
        const double start = score.tempos[i].ppq;
        if (ppq <= start)
            break;

        const double next = (i + 1 < score.tempos.size()) ? score.tempos[i + 1].ppq : ppq;
        const double segmentEnd = std::min (ppq, next);
        const double bpm = std::max (1.0, score.tempos[i].bpm);
        seconds += (segmentEnd - start) * (60.0 / bpm);
        if (ppq <= next)
            break;
    }

    return seconds;
}

AnalysisResult analyzeHits (const std::vector<DrumHit>& hits,
                            const ScoreMap& score,
                            const AnalysisSettings& settings)
{
    AnalysisResult result;
    result.kitMap = settings.kitMap;
    result.score = score.tempos.empty() ? ScoreMap {} : score;
    if (result.score.meters.empty())
        result.score.meters.push_back ({ 0.0, 4, 4 });
    if (result.score.tempos.empty())
        result.score.tempos.push_back ({ 0.0, 120.0 });

    std::vector<DrumHit> mapped;
    mapped.reserve (hits.size());
    for (const auto& hit : hits)
    {
        if (! isMappedDrum (hit.note, settings.kitMap))
        {
            ++result.ignoredNotes;
            continue;
        }

        mapped.push_back (hit);
    }

    std::stable_sort (mapped.begin(), mapped.end(), [] (const DrumHit& a, const DrumHit& b)
    {
        if (a.timeSeconds < b.timeSeconds)
            return true;
        if (b.timeSeconds < a.timeSeconds)
            return false;
        return a.note < b.note;
    });

    result.hits = mapped;
    if (! mapped.empty())
    {
        result.startPpq = mapped.front().ppq;
        result.endPpq = mapped.back().ppq;
    }

    const double window = std::max (0.0, settings.clusterWindowSeconds);
    std::vector<Cluster> clusters;
    for (const auto& hit : mapped)
    {
        if (clusters.empty() || hit.timeSeconds - clusters.back().timeSeconds > window)
        {
            Cluster cluster;
            cluster.timeSeconds = hit.timeSeconds;
            cluster.ppq = hit.ppq;
            clusters.push_back (std::move (cluster));
        }

        clusters.back().notes.push_back (hit.note);
    }

    // A closed hi-hat holds one foot until an open hi-hat. A pedal chick is that
    // same foot at the moment it lands, and does not keep the hat closed afterwards.
    bool hatDown = false;
    std::vector<Summary> summaries;
    summaries.reserve (clusters.size());

    for (const auto& cluster : clusters)
    {
        int kicks = 0;
        int chicks = 0;
        int closed = 0;
        int open = 0;
        int hands = 0;
        std::vector<juce::String> handNames;

        for (const int note : cluster.notes)
        {
            const auto info = infoForNote (note, settings.kitMap);
            if (info.hand)
            {
                ++hands;
                if (info.name != nullptr)
                    handNames.emplace_back (info.name);
            }

            if (info.voice == Voice::kick)
                ++kicks;
            else if (info.voice == Voice::hatPedal)
                ++chicks;

            if (info.closesHat)
                ++closed;
            if (info.opensHat)
                ++open;
        }

        const bool opensNow = open > 0 && closed == 0;
        const bool hatOccupied = opensNow ? false : (closed > 0 || chicks > 0 || hatDown);
        const int feet = kicks + (hatOccupied ? 1 : 0);

        Summary summary;
        summary.ppq = cluster.ppq;
        summary.handHits = hands;
        summary.footTaps = kicks + chicks;
        summary.hands = hands;
        summary.feet = feet;
        summary.hatOccupied = hatOccupied;
        summary.violation = hands > 2 || feet > 2;

        if (summary.violation)
        {
            Violation violation;
            violation.timeSeconds = cluster.timeSeconds;
            violation.ppq = cluster.ppq;
            const auto place = locate (result.score, cluster.ppq);
            violation.bar = place.bar;
            violation.beat = place.beat;
            violation.handCount = hands;
            violation.footCount = feet;

            juce::StringArray reasons;
            if (hands > 2)
                reasons.add (juce::String (hands) + " hand hits: " + joinNames (handNames));

            if (feet > 2)
            {
                if (hatOccupied && kicks >= 2 && chicks == 0)
                {
                    reasons.add (juce::String (feet) + " feet: " + kickPhrase (kicks)
                                 + " while the hi-hat is closed");
                }
                else
                {
                    std::vector<juce::String> footNames (static_cast<size_t> (kicks), "Kick");
                    if (chicks > 0)
                        footNames.emplace_back ("Hi-Hat Pedal");
                    reasons.add (juce::String (feet) + " foot taps: " + joinNames (std::move (footNames)));
                }
            }

            violation.reason = reasons.joinIntoString ("; ");
            result.violations.push_back (std::move (violation));
        }

        summaries.push_back (summary);

        if (opensNow)
            hatDown = false;
        else if (closed > 0)
            hatDown = true;
    }

    const double firstPpq = summaries.empty() ? 0.0 : summaries.front().ppq;
    double lastPpq = summaries.empty() ? 0.0 : summaries.back().ppq;
    if (settings.curveEndPpq > lastPpq)
        lastPpq = settings.curveEndPpq;
    constexpr double stepsPerQuarter = 16.0;
    const int startStep = static_cast<int> (std::floor (firstPpq * stepsPerQuarter + 1.0e-9));
    const int endStep = static_cast<int> (std::ceil (lastPpq * stepsPerQuarter - 1.0e-9));

    if (summaries.empty())
    {
        result.curve.push_back ({});
        return result;
    }

    for (int step = startStep; step <= endStep; ++step)
    {
        const double grid = step / stepsPerQuarter;
        const double windowStart = grid - 1.0;
        int handHits = 0;
        int footTaps = 0;
        int maxHands = 0;
        int maxFeet = 0;
        bool anyHand = false;
        bool anyFoot = false;
        bool impossible = false;
        bool hatHeld = false;

        for (const auto& summary : summaries)
        {
            if (summary.ppq <= windowStart || summary.ppq > grid + 1.0e-9)
                continue;

            handHits += summary.handHits;
            footTaps += summary.footTaps;
            maxHands = std::max (maxHands, summary.hands);
            maxFeet = std::max (maxFeet, summary.feet);
            anyHand = anyHand || summary.hands > 0;
            anyFoot = anyFoot || summary.feet > 0;
            impossible = impossible || summary.violation;
            hatHeld = hatHeld || summary.hatOccupied;
        }

        const double duration = std::max (0.001, secondsAtPpq (result.score, grid)
                                                    - secondsAtPpq (result.score, windowStart));
        const float handHz = static_cast<float> (handHits / duration);
        const float footHz = static_cast<float> (footTaps / duration);
        const float speed = std::clamp (std::max (handHz, footHz) / 16.0f, 0.0f, 1.0f);

        float coordination = 0.0f;
        if (maxHands >= 2 && maxFeet >= 2)
            coordination = 1.0f;
        else if (anyHand && anyFoot)
            coordination = 0.66f;
        else if (anyHand || anyFoot)
            coordination = 0.33f;

        DifficultyPoint point;
        point.ppq = grid;
        point.difficulty = 100.0f * (0.65f * speed + 0.35f * coordination);
        point.handLoad = std::clamp (handHz / 16.0f, 0.0f, 1.0f);
        point.footLoad = std::clamp (footHz / 16.0f, 0.0f, 1.0f);
        if (hatHeld)
            point.footLoad = std::max (point.footLoad, 0.35f);
        point.impossible = impossible;
        if (impossible)
            point.difficulty = 100.0f;

        result.curve.push_back (point);
    }

    return result;
}

AnalysisResult analyzeSequence (const juce::MidiMessageSequence& sequence,
                                double bpm,
                                const AnalysisSettings& settings)
{
    if (bpm <= 1.0)
        bpm = 120.0;

    std::vector<DrumHit> hits;
    hits.reserve (static_cast<size_t> (sequence.getNumEvents()));

    for (int i = 0; i < sequence.getNumEvents(); ++i)
    {
        const auto& message = sequence.getEventPointer (i)->message;
        if (! message.isNoteOn())
            continue;

        DrumHit hit;
        hit.timeSeconds = message.getTimeStamp();
        hit.ppq = hit.timeSeconds * bpm / 60.0;
        hit.note = message.getNoteNumber();
        hits.push_back (hit);
    }

    ScoreMap score;
    score.tempos = { { 0.0, bpm } };
    score.meters = { { 0.0, 4, 4 } };
    return analyzeHits (hits, score, settings);
}

DifficultyPoint sampleCurve (const std::vector<DifficultyPoint>& curve, double ppq)
{
    if (curve.empty())
        return {};

    const auto upper = std::upper_bound (curve.begin(), curve.end(), ppq,
                                          [] (double value, const DifficultyPoint& point)
                                          {
                                              return value < point.ppq;
                                          });
    if (upper == curve.begin())
        return curve.front();

    return *std::prev (upper);
}

namespace
{

void ensureOpeningTempo (ScoreMap& score)
{
    std::stable_sort (score.tempos.begin(), score.tempos.end(),
                      [] (const TempoChange& a, const TempoChange& b) { return a.ppq < b.ppq; });
    std::stable_sort (score.meters.begin(), score.meters.end(),
                      [] (const TimeSignatureChange& a, const TimeSignatureChange& b) { return a.ppq < b.ppq; });

    if (score.tempos.empty() || score.tempos.front().ppq > 0.0)
        score.tempos.insert (score.tempos.begin(), { 0.0, 120.0 });
    if (score.meters.empty() || score.meters.front().ppq > 0.0)
        score.meters.insert (score.meters.begin(), { 0.0, 4, 4 });
}

bool readTimeSignature (const juce::MidiMessage& message, int& numerator, int& denominator)
{
    if (! message.isTimeSignatureMetaEvent() || message.getMetaEventLength() < 2)
        return false;

    const auto* data = message.getMetaEventData();
    numerator = data[0];
    const int power = data[1];
    if (numerator <= 0 || power < 0 || power > 8)
        return false;

    denominator = 1 << power;
    return denominator > 0;
}

} // namespace

std::optional<LoadedClip> loadDrumClip (const juce::MemoryBlock& bytes)
{
    juce::MemoryInputStream stream (bytes.getData(), bytes.getSize(), false);
    juce::MidiFile midi;
    if (! midi.readFrom (stream))
        return std::nullopt;

    LoadedClip clip;
    clip.score.tempos.clear();
    clip.score.meters.clear();
    const int timeFormat = midi.getTimeFormat();

    if (timeFormat <= 0)
    {
        midi.convertTimestampTicksToSeconds();
        for (int trackIndex = 0; trackIndex < midi.getNumTracks(); ++trackIndex)
        {
            const auto* track = midi.getTrack (trackIndex);
            for (int eventIndex = 0; eventIndex < track->getNumEvents(); ++eventIndex)
            {
                const auto& message = track->getEventPointer (eventIndex)->message;
                if (! message.isNoteOn())
                    continue;

                DrumHit hit;
                hit.timeSeconds = message.getTimeStamp();
                hit.ppq = hit.timeSeconds * 2.0;
                hit.note = message.getNoteNumber();
                clip.hits.push_back (hit);
            }
        }

        ensureOpeningTempo (clip.score);
        return clip;
    }

    const double ticksPerQuarter = static_cast<double> (timeFormat);
    for (int trackIndex = 0; trackIndex < midi.getNumTracks(); ++trackIndex)
    {
        const auto* track = midi.getTrack (trackIndex);
        for (int eventIndex = 0; eventIndex < track->getNumEvents(); ++eventIndex)
        {
            const auto& message = track->getEventPointer (eventIndex)->message;
            const double ppq = message.getTimeStamp() / ticksPerQuarter;

            if (message.isTempoMetaEvent())
            {
                const double secondsPerQuarter = message.getTempoSecondsPerQuarterNote();
                if (secondsPerQuarter > 0.0)
                    clip.score.tempos.push_back ({ ppq, 60.0 / secondsPerQuarter });
                continue;
            }

            int numerator = 4;
            int denominator = 4;
            if (readTimeSignature (message, numerator, denominator))
            {
                clip.score.meters.push_back ({ ppq, numerator, denominator });
                continue;
            }

            if (! message.isNoteOn())
                continue;

            DrumHit hit;
            hit.ppq = ppq;
            hit.note = message.getNoteNumber();
            clip.hits.push_back (hit);
        }
    }

    ensureOpeningTempo (clip.score);
    for (auto& hit : clip.hits)
        hit.timeSeconds = secondsAtPpq (clip.score, hit.ppq);

    return clip;
}

juce::MemoryBlock midiFromHits (const std::vector<DrumHit>& hits, double bpm)
{
    if (bpm <= 1.0)
        bpm = 120.0;

    constexpr int ticksPerQuarter = 960;
    juce::MidiMessageSequence sequence;
    const int microseconds = juce::roundToInt (60000000.0 / bpm);
    sequence.addEvent (juce::MidiMessage::tempoMetaEvent (microseconds), 0.0);

    for (const auto& hit : hits)
    {
        const double ticks = std::max (0.0, hit.ppq) * ticksPerQuarter;
        sequence.addEvent (juce::MidiMessage::noteOn (10, hit.note, (juce::uint8) 100), ticks);
        sequence.addEvent (juce::MidiMessage::noteOff (10, hit.note), ticks + 10.0);
    }

    sequence.sort();
    juce::MidiFile file;
    file.setTicksPerQuarterNote (ticksPerQuarter);
    file.addTrack (sequence);

    juce::MemoryOutputStream output;
    file.writeTo (output);
    return output.getMemoryBlock();
}

} // namespace drumdoctor
