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

enum class KitMap : int
{
    generalMidi = 0,
    ezDrummer3,
    superiorDrummer3,
    addictiveDrums2,
    stevenSlate5,
    bfd3,
    getGoodDrums,
    count
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

VoiceInfo infoForNote (int note, KitMap map = KitMap::generalMidi) noexcept;

inline bool isMappedDrum (int note, KitMap map = KitMap::generalMidi) noexcept
{
    return infoForNote (note, map).voice != Voice::unmapped;
}

inline const char* kitMapName (KitMap map) noexcept
{
    switch (map)
    {
        case KitMap::generalMidi: return "General MIDI";
        case KitMap::ezDrummer3: return "EZdrummer 3";
        case KitMap::superiorDrummer3: return "Superior Drummer 3";
        case KitMap::addictiveDrums2: return "Addictive Drums 2";
        case KitMap::stevenSlate5: return "Steven Slate SSD5";
        case KitMap::bfd3: return "BFD3";
        case KitMap::getGoodDrums: return "GetGood Drums";
        case KitMap::count: break;
    }

    return "General MIDI";
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
