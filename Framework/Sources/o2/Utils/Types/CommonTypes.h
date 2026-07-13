#pragma once

#include "o2/Utils/Reflection/Enum.h"
#include "o2/Application/VKCodes.h"
#include <cstdint>
#include <cstddef>

namespace o2
{
    template<typename... Types>
    class Variant;
    using UInt64 = std::uint64_t;
    using UInt32 = std::uint32_t;
    using UInt16 = std::uint16_t;
    using UInt8  = std::uint8_t;

    using Int64  = std::int64_t;
    using Int32  = std::int32_t;
    using Int16  = std::int16_t;
    using Int8   = std::int8_t;

    using UInt   = unsigned int;
    using ULong  = unsigned long;

#if __cplusplus >= 201703L
    using Byte = std::byte;
#else
    using Byte = unsigned char;
#endif

    using VertexIndex = unsigned int;

    using KeyboardKey = int;
    using CursorId = int;
    
    using Color32Bit = UInt;

    using SceneUID = UInt64;

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
    
    enum class Platform { Windows, Mac, Linux, iOS, Android, WebAssembly };
    
    enum class DeviceType { PC, Tablet, Phone };

    enum class LineType { Solid, Dash };

    enum class PrimitiveType { Polygon, PolygonWire, Line };

    enum class PixelFormat { R8G8B8A8, R8G8B8 };

    enum class TextureFormat { R8G8B8A8, DXT5, R16G16B16A16F };

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
