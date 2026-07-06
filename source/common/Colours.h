#pragma once

#include <JuceHeader.h>

namespace Theme
{
    inline const Colour background    { 0xFF1A1A3E };
    inline const Colour surface       { 0xFF12122A };
    inline const Colour accent        { 0xFFC41E3A };
    inline const Colour text          { 0xFFFFFFFF };
    inline const Colour textDim       { 0x80FFFFFF };
    inline const Colour bandHighlight { 0x30FFFFFF };
    inline const Colour separator     { 0x40FFFFFF };

    inline Colour accentFill()   { return accent.withAlpha(0.25f); }
    inline Colour gridMajor()    { return Colour(0x40FFFFFF); }
    inline Colour gridMinor()    { return Colour(0x20FFFFFF); }
    inline Colour regionFill()   { return accent.withAlpha(0.10f); }
}
