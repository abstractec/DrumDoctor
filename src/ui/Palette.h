#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace drumdoctor::palette
{

inline juce::Colour background() { return juce::Colour (0xff14120f); }
inline juce::Colour panel() { return juce::Colour (0xff221c16); }
inline juce::Colour ink() { return juce::Colour (0xfff4ead7); }
inline juce::Colour muted() { return juce::Colour (0xffa89880); }
inline juce::Colour amber() { return juce::Colour (0xffe6a23c); }
inline juce::Colour red() { return juce::Colour (0xffe15a3a); }
inline juce::Colour green() { return juce::Colour (0xff7dba6a); }
inline juce::Colour blue() { return juce::Colour (0xff7eb6d6); }
inline juce::Colour line() { return juce::Colour (0xff3a3228); }

inline juce::Font font (float size, bool bold = false)
{
    auto styled = juce::Font (juce::FontOptions (size));
    return bold ? styled.boldened() : styled;
}

} // namespace drumdoctor::palette
