#include "DrumMap.h"

#include <cstring>

namespace drumdoctor
{
namespace
{
struct NamedNote
{
    uint8_t note;
    const char* name;
};

bool contains (const char* text, const char* key)
{
    return std::strstr (text, key) != nullptr;
}

VoiceInfo fromLibraryName (const char* name)
{
    if (contains (name, "Aux Hat"))
        return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Aux Hat" };

    if (contains (name, "Hi-Hat Pedal"))
        return { Voice::hatPedal, Lane::feet, false, true, false, false, "Hi-Hat Pedal" };

    if (contains (name, "Hi-Hat"))
    {
        if (contains (name, "Loose") || contains (name, "Open"))
            return { Voice::openHat, Lane::hat, true, false, false, true, "Open Hi-Hat" };

        return { Voice::closedHat, Lane::hat, true, false, true, false, "Closed Hi-Hat" };
    }

    if (contains (name, "Kick"))
        return { Voice::kick, Lane::feet, false, true, false, false, "Kick" };
    if (contains (name, "Side Stick"))
        return { Voice::sideStick, Lane::snare, true, false, false, false, "Side Stick" };
    if (contains (name, "Snare"))
        return { Voice::snare, Lane::snare, true, false, false, false, "Snare" };
    if (contains (name, "Tom"))
        return { Voice::tom, Lane::toms, true, false, false, false, "Tom" };
    if (contains (name, "Cowbell"))
        return { Voice::cowbell, Lane::cymbals, true, false, false, false, "Cowbell" };
    if (contains (name, "Crash"))
        return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Crash" };
    if (contains (name, "China"))
        return { Voice::cymbal, Lane::cymbals, true, false, false, false, "China" };
    if (contains (name, "Ride"))
        return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Ride" };
    if (contains (name, "Splash"))
        return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Splash" };
    if (contains (name, "Stack"))
        return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Stack" };
    if (contains (name, "Sticks"))
        return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Sticks" };
    if (contains (name, "Clap"))
        return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Clap" };
    if (contains (name, "Shaker"))
        return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Shaker" };
    if (contains (name, "Tambourine"))
        return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Tambourine" };
    if (contains (name, "Bell"))
        return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Bell" };

    return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Percussion" };
}

VoiceInfo generalMidi (int note)
{
    switch (note)
    {
        case 35:
        case 36:
            return { Voice::kick, Lane::feet, false, true, false, false, "Kick" };
        case 37:
            return { Voice::sideStick, Lane::snare, true, false, false, false, "Side Stick" };
        case 38:
        case 40:
            return { Voice::snare, Lane::snare, true, false, false, false, "Snare" };
        case 41:
        case 43:
        case 45:
        case 47:
        case 48:
        case 50:
            return { Voice::tom, Lane::toms, true, false, false, false, "Tom" };
        case 42:
            return { Voice::closedHat, Lane::hat, true, false, true, false, "Closed Hi-Hat" };
        case 44:
            return { Voice::hatPedal, Lane::feet, false, true, false, false, "Hi-Hat Pedal" };
        case 46:
            return { Voice::openHat, Lane::hat, true, false, false, true, "Open Hi-Hat" };
        case 49:
        case 57:
            return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Crash" };
        case 51:
        case 59:
            return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Ride" };
        case 52:
            return { Voice::cymbal, Lane::cymbals, true, false, false, false, "China" };
        case 53:
            return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Ride Bell" };
        case 55:
            return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Splash" };
        case 56:
            return { Voice::cowbell, Lane::cymbals, true, false, false, false, "Cowbell" };
        default:
            return {};
    }
}

constexpr NamedNote ezDrummer3Notes[] = {
    { 1, "Clap" }, { 2, "Shaker" }, { 3, "Tambourine" },
    { 6, "Snare" },
    { 7, "Hi-Hat CC Edge" }, { 8, "Hi-Hat CC Tip" }, { 9, "Hi-Hat CC Tip" }, { 10, "Hi-Hat Pedal" },
    { 11, "Hi-Hat Closed Tip" }, { 12, "Hi-Hat Loose Tip" }, { 13, "Hi-Hat Open 1 Tip" },
    { 14, "Hi-Hat Open 2 Tip" }, { 15, "Hi-Hat Open 3 Tip" }, { 16, "Hi-Hat Open 4 Tip" }, { 17, "Hi-Hat Open 5 Tip" },
    { 18, "Hi-Hat CC Edge" }, { 19, "Hi-Hat CC Tip" }, { 20, "Hi-Hat CC Edge" }, { 21, "Hi-Hat Pedal" },
    { 22, "Hi-Hat Closed Edge" }, { 23, "Hi-Hat Pedal Splash" }, { 24, "Hi-Hat Open 1" },
    { 25, "Hi-Hat Open 2" }, { 26, "Hi-Hat Open 3" },
    { 27, "Crash 2 Bow Tip" }, { 28, "Crash 2" }, { 29, "Ride Bow Tip" }, { 30, "Ride Bell" },
    { 31, "Crash 3 Bow Tip" }, { 32, "Crash 3" },
    { 33, "Snare (Side)" }, { 34, "Kick" }, { 35, "Kick" }, { 36, "Kick" },
    { 37, "Side Stick" }, { 38, "Snare" }, { 39, "Snare" }, { 40, "Snare Rimshot" },
    { 41, "Floor Tom 2" }, { 42, "Hi-Hat Closed Tip" }, { 43, "Floor Tom 1" }, { 44, "Hi-Hat Pedal" },
    { 45, "Rack Tom 3" }, { 46, "Hi-Hat Open 2" }, { 47, "Rack Tom 2" }, { 48, "Rack Tom 1" },
    { 49, "Crash 2" }, { 50, "Crash 2 (Mute)" }, { 51, "Ride Bow Tip" }, { 52, "Ride Edge" },
    { 53, "Ride Bell" }, { 54, "Ride (Mute)" }, { 55, "Crash 1" }, { 56, "Crash 1 (Mute)" },
    { 57, "Crash 3" }, { 58, "Crash 3 (Mute)" }, { 59, "Ride Edge" },
    { 60, "Hi-Hat Open 4" }, { 61, "Hi-Hat Closed Tip" }, { 62, "Hi-Hat Tight Edge" }, { 63, "Hi-Hat Tight Tip" },
    { 64, "Hi-Hat Loose" }, { 65, "Hi-Hat Closed Edge" },
    { 66, "Snare" }, { 67, "Side Stick" }, { 68, "Snare" }, { 69, "Snare" }, { 70, "Snare" }, { 71, "Snare Rim" },
    { 72, "Floor Tom 2 Rim" }, { 73, "Floor Tom 2 Rimshot" }, { 74, "Floor Tom 1 Rim" }, { 75, "Floor Tom 1 Rimshot" },
    { 76, "Snare Rim" },
    { 77, "Rack Tom 3 Rim" }, { 78, "Rack Tom 3 Rimshot" }, { 79, "Rack Tom 2 Rim" }, { 80, "Rack Tom 2 Rimshot" },
    { 81, "Rack Tom 1 Rim" }, { 82, "Rack Tom 1 Rimshot" },
    { 83, "Crash 1 (Mute)" }, { 84, "Crash 1 Bow Tip" }, { 85, "Ride Bell" }, { 86, "Crash 1" },
    { 87, "Crash 1 Bow Tip" }, { 88, "Ride Bell" }, { 89, "Crash 2 Bow Tip" }, { 90, "Ride Bell" },
    { 91, "Crash 2" }, { 92, "Ride Bell" }, { 93, "Ride Bell" }, { 94, "Crash 2 (Mute)" },
    { 95, "Crash 4 (Mute)" }, { 96, "Crash 4 Bow Tip" }, { 97, "Ride Bell" }, { 98, "Crash 4" },
    { 99, "Crash 4 (Mute)" }, { 100, "Ride Bell" }, { 101, "Crash 3 Bow Tip" }, { 102, "Ride Bell" },
    { 103, "Crash 3" }, { 104, "Ride Bow Tip" }, { 105, "Ride Bell" }, { 106, "Crash 3 (Mute)" },
    { 107, "Crash 3 (Mute)" }, { 108, "Ride Bow Tip" }, { 109, "Ride Bell" }, { 110, "Ride Edge" },
    { 111, "Ride Bow Tip" }, { 112, "Ride Bell" }, { 113, "Ride Bow Tip" }, { 114, "Ride Bell" },
    { 115, "Ride Edge" }, { 116, "Ride Bow Tip" }, { 117, "Ride Bell" }, { 118, "Ride (Mute)" },
    { 119, "Hi-Hat Closed Tip" }, { 120, "Hi-Hat Loose" }, { 121, "Hi-Hat Open 4" },
    { 122, "Hi-Hat Closed Edge" }, { 123, "Hi-Hat Open 2" }, { 124, "Hi-Hat Open 5" },
    { 125, "Snare" }, { 126, "Snare Rimshot" }, { 127, "Side Stick" },
};

constexpr NamedNote superiorDrummer3Notes[] = {
    { 6, "Snare" },
    { 7, "Hi-Hat CC Edge" }, { 8, "Hi-Hat CC Tip" }, { 9, "Hi-Hat CC Bell" }, { 10, "Hi-Hat Pedal" },
    { 11, "Hi-Hat Closed Tip" }, { 12, "Hi-Hat Loose Tip" }, { 13, "Hi-Hat Open 1 Tip" },
    { 14, "Hi-Hat Open 2 Tip" }, { 15, "Hi-Hat Open 3 Tip" }, { 16, "Hi-Hat Open 4 Tip" }, { 17, "Hi-Hat Open 5 Tip" },
    { 18, "Hi-Hat CC Edge" }, { 19, "Hi-Hat CC Tip" }, { 20, "Hi-Hat CC Bell" }, { 21, "Hi-Hat Pedal" },
    { 22, "Hi-Hat Closed Edge" }, { 23, "Hi-Hat Pedal Splash" }, { 24, "Hi-Hat Open 1 Edge" },
    { 25, "Hi-Hat Open 2 Edge" }, { 26, "Hi-Hat Open 3 Edge" },
    { 27, "Crash 2 Bow Tip" }, { 28, "Crash 2 Bell" }, { 29, "Ride" }, { 30, "Ride Bell Tip" },
    { 31, "Crash 4 Bow Tip" }, { 32, "Crash 4 Bell" },
    { 33, "Snare (Side)" }, { 34, "Kick" }, { 35, "Kick" }, { 36, "Kick" },
    { 37, "Side Stick" }, { 38, "Snare" }, { 39, "Snare" }, { 40, "Snare Rimshot" },
    { 41, "Floor Tom 2" }, { 42, "Hi-Hat Closed Tip" }, { 43, "Floor Tom 1" }, { 44, "Hi-Hat Pedal" },
    { 45, "Rack Tom 3" }, { 46, "Hi-Hat Open 2 Edge" }, { 47, "Rack Tom 2" }, { 48, "Rack Tom 1" },
    { 49, "Crash 2" }, { 50, "Crash 2 (Mute)" }, { 51, "Ride Bow Tip" }, { 52, "China 1" },
    { 53, "Ride Bell" }, { 54, "China 1 (Mute)" }, { 55, "Crash 3" }, { 56, "Crash 3 (Mute)" },
    { 57, "Crash 4" }, { 58, "Crash 4 (Mute)" }, { 59, "Ride Edge" },
    { 60, "Hi-Hat Open 4 Edge" }, { 61, "Hi-Hat Closed Tip" }, { 62, "Hi-Hat Tight Edge" }, { 63, "Hi-Hat Tight Tip" },
    { 64, "Hi-Hat Loose Edge" }, { 65, "Percussion" },
    { 66, "Snare" }, { 67, "Side Stick" }, { 68, "Snare" }, { 69, "Snare Flam" }, { 70, "Snare" }, { 71, "Snare Rim" },
    { 72, "Floor Tom 2 Rim" }, { 73, "Floor Tom 2 Rimshot" }, { 74, "Floor Tom 1 Rim" }, { 75, "Floor Tom 1 Rimshot" },
    { 76, "Side Stick" },
    { 77, "Rack Tom 3 Rim" }, { 78, "Rack Tom 3 Rimshot" }, { 79, "Rack Tom 2 Rim" }, { 80, "Rack Tom 2 Rimshot" },
    { 81, "Rack Tom 1 Rim" }, { 82, "Rack Tom 1 Rimshot" },
    { 83, "Crash 1 (Mute)" }, { 84, "Crash 1 Bow Tip" }, { 85, "Crash 1 Bell" }, { 86, "Crash 1" },
    { 87, "Crash 1 Bow" }, { 88, "Crash 1 Bell Tip" }, { 89, "Crash 2 Bow Tip" }, { 90, "Crash 2 Bell" },
    { 91, "Crash 2" }, { 92, "Crash 2 Bow" }, { 93, "Crash 2 Bell Tip" }, { 94, "Crash 2 (Mute)" },
    { 95, "Crash 3 (Mute)" }, { 96, "Crash 3 Bow Tip" }, { 97, "Crash 3 Bell" }, { 98, "Crash 3" },
    { 99, "Crash 3 Bow" }, { 100, "Crash 3 Bell Tip" }, { 101, "Crash 4 Bow Tip" }, { 102, "Crash 4 Bell" },
    { 103, "Crash 4" }, { 104, "Crash 4 Bow" }, { 105, "Crash 4 Bell Tip" }, { 106, "Crash 4 (Mute)" },
    { 107, "China 1 (Mute)" }, { 108, "China 1 Bow Tip" }, { 109, "China 1 Bell" }, { 110, "China 1" },
    { 111, "China 1 Bow" }, { 112, "China 1 Bell Tip" }, { 113, "Ride Bow Tip" }, { 114, "Ride Bell" },
    { 115, "Ride Edge" }, { 116, "Ride" }, { 117, "Ride Bell Tip" }, { 118, "Ride (Mute)" },
    { 119, "Hi-Hat Closed Bell" }, { 120, "Hi-Hat Loose Bell" }, { 121, "Hi-Hat Open 1 Bell" },
    { 122, "Hi-Hat Open 2 Bell" }, { 123, "Hi-Hat Open 3 Bell" }, { 124, "Hi-Hat Open 4 Bell" },
    { 125, "Snare" }, { 126, "Snare" }, { 127, "Side Stick" },
};

constexpr NamedNote addictiveDrums2Notes[] = {
    { 36, "Kick" },
    { 37, "Snare Rimshot" }, { 38, "Snare" }, { 39, "Snare Rimshot" }, { 40, "Snare" },
    { 41, "Snare Rimshot" }, { 42, "Side Stick" }, { 43, "Snare" }, { 44, "Snare Rim" },
    { 45, "Ride" }, { 46, "Crash 1" }, { 47, "Cowbell" },
    { 48, "Hi-Hat Pedal" }, { 49, "Hi-Hat Tight Tip" }, { 50, "Hi-Hat Tight Edge" },
    { 51, "Hi-Hat Closed Tip" }, { 52, "Hi-Hat Closed Edge" }, { 53, "Hi-Hat Closed Bell" },
    { 54, "Hi-Hat Loose" }, { 55, "Hi-Hat Open 1" }, { 56, "Hi-Hat Open 2" }, { 57, "Hi-Hat Open 3" },
    { 58, "Hi-Hat Open 1 Bell" }, { 59, "Hi-Hat Pedal Splash" },
    { 60, "Ride" }, { 61, "Ride Bell" }, { 62, "Ride Edge" }, { 63, "Ride (Mute)" },
    { 65, "Floor Tom 1" }, { 66, "Floor Tom 1 Rimshot" }, { 67, "Rack Tom 3" }, { 68, "Rack Tom 3 Rimshot" },
    { 69, "Rack Tom 2" }, { 70, "Rack Tom 2 Rimshot" }, { 71, "Rack Tom 1" }, { 72, "Rack Tom 1 Rimshot" },
    { 75, "Sticks" },
    { 77, "Crash 1" }, { 78, "Crash 1 (Mute)" }, { 79, "Crash 2" }, { 80, "Crash 2 (Mute)" },
    { 81, "Crash 3" }, { 82, "Crash 3 (Mute)" },
    { 84, "Ride 2" }, { 85, "Ride 2 Bell" }, { 86, "Ride 2 Edge" }, { 87, "Ride 2 (Mute)" },
    { 89, "Crash 4" }, { 90, "Crash 4 (Mute)" }, { 91, "China 1" }, { 92, "China 1 (Mute)" },
    { 93, "Crash 6" }, { 94, "Crash 6 (Mute)" },
    { 101, "Snare" },
};

constexpr NamedNote stevenSlate5Notes[] = {
    { 12, "Floor Tom 2 Rim" }, { 14, "Floor Tom 1 Rim" }, { 16, "Rack Tom 3 Rim" },
    { 17, "Rack Tom 2 Rim" }, { 19, "Rack Tom 1 Rim" },
    { 21, "Snare Ruff" }, { 22, "Hi-Hat Closed Edge" },
    { 26, "Hi-Hat CC" },
    { 27, "Crash 2 (Mute)" }, { 28, "Crash 2" }, { 29, "Crash 2" }, { 30, "Crash 2 (Mute)" },
    { 31, "China 1" }, { 32, "China 1 (Mute)" },
    { 33, "Snare Rimshot" }, { 34, "Snare (Side)" }, { 35, "Kick (Left)" }, { 36, "Kick" },
    { 37, "Side Stick" }, { 38, "Snare" }, { 39, "Snare Rim" }, { 40, "Snare Rimshot" },
    { 41, "Floor Tom 2" }, { 42, "Hi-Hat Closed Tip" }, { 43, "Floor Tom 1" }, { 44, "Hi-Hat Pedal" },
    { 45, "Rack Tom 3" }, { 46, "Hi-Hat CC" }, { 47, "Rack Tom 2" }, { 48, "Rack Tom 1" },
    { 49, "Splash 1 (Mute)" }, { 50, "Splash 1" }, { 51, "Ride Bow Tip" }, { 52, "Ride" },
    { 53, "Ride Bell" }, { 54, "Crash 1 (Mute)" }, { 55, "Crash 1" }, { 56, "Crash 2 (Mute)" },
    { 57, "Crash 2" }, { 58, "Ride (Mute)" }, { 59, "Ride Edge" },
    { 60, "Hi-Hat Tight Edge" }, { 61, "Hi-Hat Closed Edge" }, { 62, "Hi-Hat Loose" },
    { 63, "Hi-Hat Open 1" }, { 64, "Hi-Hat Open 2" }, { 65, "Hi-Hat Open 3" },
    { 66, "Hi-Hat Tight Tip" }, { 67, "Hi-Hat Closed Tip" }, { 68, "Hi-Hat Loose Tip" },
    { 69, "Hi-Hat Open 1 Tip" }, { 70, "Hi-Hat Open 2 Tip" }, { 71, "Hi-Hat Open 3 Tip" },
    { 72, "Hi-Hat Pedal" }, { 73, "Hi-Hat Pedal Splash" },
    { 83, "Ride Bell" }, { 84, "Ride Bell" },
    { 90, "Splash 1" }, { 91, "Crash 1 Bow Tip" }, { 92, "Crash 2 Bow Tip" },
    { 93, "China 1 Bow Tip" }, { 94, "Crash 2 Bow Tip" }, { 95, "Crash 2 Bow Tip" },
};

constexpr NamedNote bfd3Notes[] = {
    { 18, "Hi-Hat CC Tip" }, { 20, "Hi-Hat CC Edge" }, { 22, "Hi-Hat CC Bell" },
    { 24, "Kick" }, { 25, "Side Stick" }, { 26, "Snare" }, { 27, "Snare Ruff" }, { 28, "Snare" }, { 29, "Snare Rimshot" },
    { 30, "Hi-Hat Closed Tip" }, { 31, "Floor Tom 1" }, { 32, "Hi-Hat Pedal" }, { 33, "Floor Tom 2" },
    { 34, "Hi-Hat Open 3 Tip" }, { 35, "Rack Tom 3" }, { 36, "Rack Tom 4" }, { 37, "Hi-Hat Closed Edge" },
    { 38, "Rack Tom 1" }, { 39, "Hi-Hat Open 3 Tip" }, { 40, "Rack Tom 2" },
    { 41, "Crash 1" }, { 42, "Hi-Hat Loose Tip" }, { 43, "Crash 2" }, { 44, "Hi-Hat Loose Edge" },
    { 45, "Ride Edge" }, { 46, "Hi-Hat Open 1 Tip" }, { 47, "Crash 3" }, { 48, "Crash 4" },
    { 49, "Hi-Hat Open 1 Edge" }, { 50, "Ride 2 Edge" }, { 51, "Hi-Hat Open 2 Tip" },
    { 52, "Crash 1 Bow Tip" }, { 53, "Crash 2 Bow Tip" }, { 54, "Hi-Hat Open 2 Edge" },
    { 55, "Ride" }, { 56, "Hi-Hat Pedal Splash" }, { 57, "Crash 3 Bow" }, { 58, "Hi-Hat Closed Bell" },
    { 59, "Crash 4 Bow" }, { 60, "Ride 2" },
    { 61, "Floor Tom 1 Rimshot" }, { 62, "Crash 1 Bell" }, { 63, "Floor Tom 2 Rimshot" }, { 64, "Crash 2 Bell" },
    { 65, "Ride Bell" }, { 66, "Rack Tom 3 Rimshot" }, { 67, "Crash 3 Bell" }, { 68, "Rack Tom 4 Rimshot" },
    { 69, "Crash 4 Bell" }, { 70, "Rack Tom 1 Rimshot" }, { 71, "Ride 2 Bell" }, { 72, "Crash 1" },
    { 73, "Rack Tom 2 Rimshot" }, { 74, "Crash 2" }, { 75, "Crash 1 (Mute)" }, { 76, "Ride Edge" },
    { 77, "Crash 3" }, { 78, "Crash 2 (Mute)" }, { 79, "Crash 4" }, { 80, "Ride (Mute)" },
    { 81, "Ride 2 Edge" }, { 82, "Crash 3 (Mute)" }, { 83, "Clap" }, { 84, "Kick" },
    { 85, "Crash 4 (Mute)" }, { 86, "Snare Flam" }, { 87, "Ride 2 (Mute)" }, { 88, "Snare Rim" },
    { 89, "Floor Tom 1 Rim" }, { 90, "Floor Tom 2 Rim" }, { 91, "Rack Tom 3 Rim" }, { 92, "Rack Tom 4 Rim" },
    { 93, "Rack Tom 1 Rim" }, { 94, "Rack Tom 2 Rim" }, { 95, "Percussion" },
};

constexpr NamedNote getGoodDrumsNotes[] = {
    { 17, "Hi-Hat CC" },
    { 22, "Kick (Alt)" }, { 23, "Kick (Left)" }, { 24, "Kick" },
    { 26, "Snare" }, { 27, "Snare Flam" }, { 28, "Snare Ruff" }, { 29, "Snare (Off)" }, { 30, "Side Stick" },
    { 33, "Rack Tom 1" }, { 34, "Rack Tom 2" }, { 35, "Rack Tom 3" },
    { 36, "Floor Tom 1" }, { 37, "Floor Tom 2" }, { 38, "Floor Tom 3" },
    { 41, "Hi-Hat Tight Tip" }, { 42, "Hi-Hat Tight Edge" }, { 43, "Hi-Hat Closed Tip" }, { 44, "Hi-Hat Closed Edge" },
    { 45, "Hi-Hat Open 1" }, { 46, "Hi-Hat Open 2" }, { 47, "Hi-Hat Open 3" }, { 48, "Hi-Hat Pedal" },
    { 52, "Crash 1" }, { 53, "Crash 1 (Mute)" }, { 54, "Crash 2" }, { 55, "Crash 2 (Mute)" },
    { 56, "Crash 3" }, { 57, "Crash 3 (Mute)" }, { 58, "Crash 4" }, { 59, "Crash 4 (Mute)" },
    { 61, "Ride Bell" }, { 62, "Ride" }, { 63, "Ride Edge" },
    { 65, "China 1" }, { 66, "China 1 (Mute)" }, { 67, "China 2" }, { 68, "China 2 (Mute)" },
    { 70, "Aux Hat 1 Closed" }, { 71, "Aux Hat 1 Open 1" },
    { 73, "Splash 1" }, { 74, "Splash 1 (Mute)" }, { 75, "Splash 2" }, { 76, "Splash 2 (Mute)" },
    { 78, "Stack 1" }, { 79, "Bell 1" }, { 80, "Bell 2" },
};

VoiceInfo fromTable (const NamedNote* notes, int count, int note)
{
    if (note < 0 || note > 127)
        return {};

    for (int i = 0; i < count; ++i)
        if (notes[i].note == note)
            return fromLibraryName (notes[i].name);

    return {};
}
} // namespace

VoiceInfo infoForNote (int note, KitMap map) noexcept
{
    switch (map)
    {
        case KitMap::generalMidi:
            return generalMidi (note);
        case KitMap::ezDrummer3:
            return fromTable (ezDrummer3Notes, (int) (sizeof (ezDrummer3Notes) / sizeof (NamedNote)), note);
        case KitMap::superiorDrummer3:
            return fromTable (superiorDrummer3Notes, (int) (sizeof (superiorDrummer3Notes) / sizeof (NamedNote)), note);
        case KitMap::addictiveDrums2:
            return fromTable (addictiveDrums2Notes, (int) (sizeof (addictiveDrums2Notes) / sizeof (NamedNote)), note);
        case KitMap::stevenSlate5:
            return fromTable (stevenSlate5Notes, (int) (sizeof (stevenSlate5Notes) / sizeof (NamedNote)), note);
        case KitMap::bfd3:
            return fromTable (bfd3Notes, (int) (sizeof (bfd3Notes) / sizeof (NamedNote)), note);
        case KitMap::getGoodDrums:
            return fromTable (getGoodDrumsNotes, (int) (sizeof (getGoodDrumsNotes) / sizeof (NamedNote)), note);
        case KitMap::count:
            break;
    }

    return {};
}
} // namespace drumdoctor
