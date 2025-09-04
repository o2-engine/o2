#pragma once

#include "o2/Utils/Reflection/Enum.h"
#include "o2/Application/VKCodes.h"
#include <cstdint>

namespace o2
{
    typedef std::uint64_t UInt64;
    typedef std::uint32_t UInt32;
    typedef std::uint16_t UInt16;
    typedef std::uint8_t  UInt8;

    typedef std::int64_t  Int64;
    typedef std::int32_t  Int32;
    typedef std::int16_t  Int16;
    typedef std::int8_t   Int8;

    typedef unsigned int  UInt;
    typedef unsigned long ULong;

    typedef std::byte Byte;

    typedef unsigned int VertexIndex;

    typedef int KeyboardKey;
    typedef int CursorId;
    
    typedef UInt Color32Bit;

    typedef UInt64 SceneUID;

    enum class BaseCorner { Left, Right, Top, Bottom, Center, LeftBottom, LeftTop, RightBottom, RightTop };

    enum class CursorType
    {
        AppStarting, Arrow, Cross, Hand, Help, IBeam, Icon, No, SizeAll, SizeNeSw, SizeNS, SizeNwSe,
        SizeWE, UpArrow, Wait
    };

    enum class Side { Left, Bottom, Right, Top, None };

    enum class Corner { LeftTop, RightTop, RightBottom, LeftBottom };

    enum class TwoDirection { Horizontal, Vertical };

    enum class SpriteMode 
    { 
        Default, Sliced, Tiled, FixedAspect, FillLeftToRight, FillRightToLeft, FillUpToDown, FillDownToUp, 
        Fill360CW, Fill360CCW
    };

    enum class BlendMode { Normal, Add };

    enum class VerAlign { Top, Middle, Bottom, Both };
    enum class HorAlign { Left, Middle, Right, Both };

    enum class ProtectSection { Public, Private, Protected };
    
    enum class Platform { Windows, Mac, Linux, iOS, Android };
    
    enum class DeviceType { PC, Tablet, Phone };

    enum class LineType { Solid, Dash };

    enum class PrimitiveType { Polygon, PolygonWire, Line };

    enum class PixelFormat { R8G8B8A8, R8G8B8 };

    enum class TextureFormat { R8G8B8A8, DXT5 };

    enum class Loop { None, Repeat, PingPong };

    enum class Units { Pixels, Centimeters, Millimeters, Inches };
}
// --- META ---

PRE_ENUM_META(o2::BaseCorner);

PRE_ENUM_META(o2::CursorType);

PRE_ENUM_META(o2::Side);

PRE_ENUM_META(o2::Corner);

PRE_ENUM_META(o2::TwoDirection);

PRE_ENUM_META(o2::SpriteMode);

PRE_ENUM_META(o2::BlendMode);

PRE_ENUM_META(o2::VerAlign);

PRE_ENUM_META(o2::HorAlign);

PRE_ENUM_META(o2::ProtectSection);

PRE_ENUM_META(o2::Platform);

PRE_ENUM_META(o2::DeviceType);

PRE_ENUM_META(o2::LineType);

PRE_ENUM_META(o2::PrimitiveType);

PRE_ENUM_META(o2::PixelFormat);

PRE_ENUM_META(o2::TextureFormat);

PRE_ENUM_META(o2::Loop);

PRE_ENUM_META(o2::Units);
// --- END META ---
