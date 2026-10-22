#include "analysis/PlayabilityAnalyzer.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace
{
int gFailures = 0;

void expect (bool condition, const char* expression, const char* file, int line)
{
    if (! condition)
    {
        std::cerr << file << ":" << line << "  " << expression << "\n";
        ++gFailures;
    }
}

#define EXPECT(cond) expect ((cond), #cond, __FILE__, __LINE__)

juce::MidiMessageSequence sequence (const std::vector<std::pair<double, int>>& notes)
{
    juce::MidiMessageSequence events;
    for (const auto& [time, note] : notes)
    {
        auto message = juce::MidiMessage::noteOn (10, note, (juce::uint8) 100);
        events.addEvent (message, time);
    }

    events.sort();
    return events;
}

drumdoctor::AnalysisResult analyze (const std::vector<std::pair<double, int>>& notes, double windowSeconds = 0.010)
{
    drumdoctor::AnalysisSettings settings;
    settings.clusterWindowSeconds = windowSeconds;
    return drumdoctor::analyzeSequence (sequence (notes), 120.0, settings);
}

void testTwoHandsPlayable()
{
    const auto result = analyze ({ { 0.0, 49 }, { 0.0, 51 } });
    EXPECT (result.violations.empty());
}

void testThreeHandsImpossible()
{
    const auto result = analyze ({ { 0.0, 49 }, { 0.0, 51 }, { 0.0, 38 } });
    EXPECT (result.violations.size() == 1);
    if (result.violations.empty())
        return;

    EXPECT (result.violations.front().reason.contains ("3 hand hits"));
    EXPECT (result.violations.front().reason.contains ("Crash"));
    EXPECT (result.violations.front().reason.contains ("Ride"));
    EXPECT (result.violations.front().reason.contains ("Snare"));
    EXPECT (result.violations.front().bar == 1);
    EXPECT (std::abs (result.violations.front().beat - 1.0) < 0.05);
    EXPECT (! result.curve.empty());
    EXPECT (result.curve.front().impossible);
    EXPECT (result.curve.front().difficulty == 100.0f);
}

void testTwoKicksPlayable()
{
    const auto result = analyze ({ { 0.0, 35 }, { 0.0, 36 } });
    EXPECT (result.violations.empty());
}

void testThreeKicksImpossible()
{
    const auto result = analyze ({ { 0.0, 36 }, { 0.0, 36 }, { 0.0, 35 } });
    EXPECT (result.violations.size() == 1);
    if (! result.violations.empty())
        EXPECT (result.violations.front().footCount == 3);
}

void testClosedHatPlusOneKick()
{
    const auto result = analyze ({ { 0.0, 42 }, { 0.0, 36 } });
    EXPECT (result.violations.empty());
}

void testClosedHatPlusTwoKicks()
{
    const auto result = analyze ({ { 0.0, 42 }, { 0.0, 35 }, { 0.0, 36 } });
    EXPECT (result.violations.size() == 1);
    if (! result.violations.empty())
        EXPECT (result.violations.front().reason.contains ("while the hi-hat is closed"));
}

void testClosedHatHoldsFoot()
{
    const auto result = analyze ({ { 0.0, 42 }, { 0.5, 35 }, { 0.5, 36 } });
    EXPECT (result.violations.size() == 1);
    if (result.violations.empty())
        return;

    EXPECT (result.violations.front().reason.contains ("while the hi-hat is closed"));
    EXPECT (result.violations.front().bar == 1);
    EXPECT (std::abs (result.violations.front().beat - 2.0) < 0.05);
}

void testOpenHatReleasesFoot()
{
    const auto result = analyze ({ { 0.0, 42 }, { 0.5, 46 }, { 0.5, 35 }, { 0.5, 36 } });
    EXPECT (result.violations.empty());
}

void testChickPlusKick()
{
    const auto result = analyze ({ { 0.0, 44 }, { 0.0, 36 } });
    EXPECT (result.violations.empty());
}

void testChickPlusTwoKicks()
{
    const auto result = analyze ({ { 0.0, 44 }, { 0.0, 35 }, { 0.0, 36 } });
    EXPECT (result.violations.size() == 1);
}

void testChickDoesNotHoldTheHat()
{
    const auto result = analyze ({ { 0.0, 44 }, { 0.0, 36 }, { 0.5, 35 }, { 0.5, 36 } });
    EXPECT (result.violations.empty());
}

void testFlamIsNotAChord()
{
    const auto spread = analyze ({ { 0.0, 49 }, { 0.030, 51 }, { 0.060, 57 } });
    EXPECT (spread.violations.empty());

    const auto stacked = analyze ({ { 0.0, 49 }, { 0.005, 51 }, { 0.010, 57 } });
    EXPECT (stacked.violations.size() == 1);
}

void testEmptyAndUnmapped()
{
    const auto empty = analyze ({});
    EXPECT (empty.violations.empty());
    EXPECT (empty.ignoredNotes == 0);

    const auto pitched = analyze ({ { 0.0, 12 }, { 0.0, 16 }, { 0.0, 19 } });
    EXPECT (pitched.violations.empty());
    EXPECT (pitched.ignoredNotes == 3);
}

void testSixteenthNoteDifficulty()
{
    const auto result = analyze ({ { 0.125, 38 }, { 0.25, 38 }, { 0.375, 38 }, { 0.5, 38 } });
    const drumdoctor::DifficultyPoint* atOne = nullptr;
    for (const auto& point : result.curve)
        if (std::abs (point.ppq - 1.0) < 0.01)
            atOne = &point;

    EXPECT (atOne != nullptr);
    if (atOne == nullptr)
        return;

    EXPECT (! atOne->impossible);
    EXPECT (atOne->difficulty > 40.0f);
    EXPECT (atOne->difficulty < 50.0f);
    EXPECT (drumdoctor::bandFor (atOne->difficulty, atOne->impossible) == "Moderate");
}

int mappedCount (drumdoctor::KitMap map)
{
    int count = 0;
    for (int note = 0; note < 128; ++note)
        if (drumdoctor::isMappedDrum (note, map))
            ++count;
    return count;
}

void testLibraryMaps()
{
    EXPECT (mappedCount (drumdoctor::KitMap::generalMidi) == 22);
    EXPECT (mappedCount (drumdoctor::KitMap::ezDrummer3) == 125);
    EXPECT (mappedCount (drumdoctor::KitMap::superiorDrummer3) == 122);
    EXPECT (mappedCount (drumdoctor::KitMap::addictiveDrums2) == 54);
    EXPECT (mappedCount (drumdoctor::KitMap::stevenSlate5) == 63);
    EXPECT (mappedCount (drumdoctor::KitMap::bfd3) == 75);
    EXPECT (mappedCount (drumdoctor::KitMap::getGoodDrums) == 47);

    EXPECT (drumdoctor::infoForNote (42).closesHat);
    EXPECT (drumdoctor::infoForNote (36, drumdoctor::KitMap::getGoodDrums).voice == drumdoctor::Voice::tom);
    EXPECT (drumdoctor::infoForNote (24, drumdoctor::KitMap::getGoodDrums).voice == drumdoctor::Voice::kick);
    EXPECT (drumdoctor::infoForNote (43, drumdoctor::KitMap::getGoodDrums).closesHat);
    EXPECT (drumdoctor::infoForNote (70, drumdoctor::KitMap::getGoodDrums).voice == drumdoctor::Voice::cymbal);

    auto analyzeMap = [] (drumdoctor::KitMap map, std::vector<std::pair<double, int>> notes)
    {
        drumdoctor::AnalysisSettings settings;
        settings.kitMap = map;
        return drumdoctor::analyzeSequence (sequence (notes), 120.0, settings);
    };

    const auto ggd = analyzeMap (drumdoctor::KitMap::getGoodDrums, { { 0.0, 23 }, { 0.0, 24 }, { 0.0, 43 } });
    EXPECT (ggd.violations.size() == 1);
    if (! ggd.violations.empty())
        EXPECT (ggd.violations.front().footCount == 3);

    const auto ezOpen = drumdoctor::infoForNote (24, drumdoctor::KitMap::ezDrummer3);
    EXPECT (ezOpen.opensHat);
    EXPECT (drumdoctor::infoForNote (44, drumdoctor::KitMap::ezDrummer3).voice == drumdoctor::Voice::hatPedal);
    const auto ezFeet = analyzeMap (drumdoctor::KitMap::ezDrummer3, { { 0.0, 34 }, { 0.0, 36 }, { 0.0, 44 } });
    EXPECT (ezFeet.violations.size() == 1);

    EXPECT (drumdoctor::infoForNote (61, drumdoctor::KitMap::superiorDrummer3).closesHat);
    EXPECT (drumdoctor::infoForNote (52, drumdoctor::KitMap::superiorDrummer3).name == std::string ("China"));
    const auto superior = analyzeMap (drumdoctor::KitMap::superiorDrummer3, { { 0.0, 35 }, { 0.0, 36 }, { 0.0, 61 } });
    EXPECT (superior.violations.size() == 1);

    EXPECT (drumdoctor::infoForNote (42, drumdoctor::KitMap::addictiveDrums2).voice == drumdoctor::Voice::sideStick);
    EXPECT (drumdoctor::infoForNote (51, drumdoctor::KitMap::addictiveDrums2).closesHat);
    const auto addictive = analyzeMap (drumdoctor::KitMap::addictiveDrums2, { { 0.0, 36 }, { 0.0, 51 } });
    EXPECT (addictive.violations.empty());

    EXPECT (drumdoctor::infoForNote (36, drumdoctor::KitMap::bfd3).voice == drumdoctor::Voice::tom);
    EXPECT (drumdoctor::infoForNote (24, drumdoctor::KitMap::bfd3).voice == drumdoctor::Voice::kick);
    EXPECT (drumdoctor::infoForNote (32, drumdoctor::KitMap::bfd3).voice == drumdoctor::Voice::hatPedal);

    EXPECT (drumdoctor::infoForNote (42, drumdoctor::KitMap::stevenSlate5).closesHat);
    EXPECT (drumdoctor::infoForNote (63, drumdoctor::KitMap::stevenSlate5).opensHat);
    EXPECT (drumdoctor::infoForNote (44, drumdoctor::KitMap::stevenSlate5).voice == drumdoctor::Voice::hatPedal);
}

void testMidiRoundTrip()
{
    const std::vector<drumdoctor::DrumHit> hits {
        { 0.0, 0.0, 42 },
        { 0.5, 1.0, 35 },
        { 0.5, 1.0, 36 }
    };

    const auto bytes = drumdoctor::midiFromHits (hits, 120.0);
    const auto loaded = drumdoctor::loadDrumClip (bytes);
    EXPECT (loaded.has_value());
    if (! loaded.has_value())
        return;

    const auto result = drumdoctor::analyzeHits (loaded->hits, loaded->score, {});
    EXPECT (result.violations.size() == 1);
    if (! result.violations.empty())
        EXPECT (result.violations.front().reason.contains ("while the hi-hat is closed"));
}

void writeFixture (const juce::String& path)
{
    std::vector<drumdoctor::DrumHit> hits;
    auto add = [&] (double ppq, int note)
    {
        drumdoctor::DrumHit hit;
        hit.ppq = ppq;
        hit.timeSeconds = ppq * 0.5;
        hit.note = note;
        hits.push_back (hit);
    };

    add (0.0, 36);
    add (0.0, 42);
    add (0.0, 38);
    add (4.0, 49);
    add (4.0, 51);
    add (4.0, 38);
    add (8.0, 35);
    add (8.0, 36);

    const auto bytes = drumdoctor::midiFromHits (hits, 120.0);
    juce::File (path).replaceWithData (bytes.getData(), bytes.getSize());
}

} // namespace

int main (int argc, char** argv)
{
    testTwoHandsPlayable();
    testThreeHandsImpossible();
    testTwoKicksPlayable();
    testThreeKicksImpossible();
    testClosedHatPlusOneKick();
    testClosedHatPlusTwoKicks();
    testClosedHatHoldsFoot();
    testOpenHatReleasesFoot();
    testChickPlusKick();
    testChickPlusTwoKicks();
    testChickDoesNotHoldTheHat();
    testFlamIsNotAChord();
    testEmptyAndUnmapped();
    testSixteenthNoteDifficulty();
    testLibraryMaps();
    testMidiRoundTrip();

    if (argc > 2 && std::string (argv[1]) == "--fixture")
        writeFixture (argv[2]);

    if (gFailures == 0)
    {
        std::cout << "Playability tests passed\n";
        return 0;
    }

    std::cerr << gFailures << " expectation(s) failed\n";
    return 1;
}
