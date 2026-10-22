#pragma once

#include <cstdint>

namespace drumdoctor
{

enum class Voice
{
    unmapped,
    kick,
    snare,
    sideStick,
    tom,
    closedHat,
    openHat,
    hatPedal,
    cymbal,
    cowbell
};

enum class Lane
{
    feet,
    snare,
    toms,
    hat,
    cymbals
};

struct VoiceInfo
{
    Voice voice = Voice::unmapped;
    Lane lane = Lane::cymbals;
    bool hand = false;
    bool footTap = false;
    bool closesHat = false;
    bool opensHat = false;
    const char* name = nullptr;
};

inline VoiceInfo infoForNote (int note) noexcept
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
            return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Ride" };
        case 52:
            return { Voice::cymbal, Lane::cymbals, true, false, false, false, "China" };
        case 53:
            return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Ride Bell" };
        case 55:
            return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Splash" };
        case 56:
            return { Voice::cowbell, Lane::cymbals, true, false, false, false, "Cowbell" };
        case 59:
            return { Voice::cymbal, Lane::cymbals, true, false, false, false, "Ride" };
        default:
            return {};
    }
}

inline bool isMappedDrum (int note) noexcept
{
    return infoForNote (note).voice != Voice::unmapped;
}

inline int laneCount() noexcept
{
    return 5;
}

inline const char* laneName (Lane lane) noexcept
{
    switch (lane)
    {
        case Lane::feet: return "Feet";
        case Lane::snare: return "Snare";
        case Lane::toms: return "Toms";
        case Lane::hat: return "Hi-Hat";
        case Lane::cymbals: return "Cymbals";
    }

    return "";
}

} // namespace drumdoctor
