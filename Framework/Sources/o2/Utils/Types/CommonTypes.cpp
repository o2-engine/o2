#include "o2/stdafx.h"
#include "CommonTypes.h"

#include "o2/Utils/Reflection/Reflection.h"
// --- META ---

ENUM_META(o2::BaseCorner, o2__BaseCorner)
{
    ENUM_ENTRY(Bottom);
    ENUM_ENTRY(Center);
    ENUM_ENTRY(Left);
    ENUM_ENTRY(LeftBottom);
    ENUM_ENTRY(LeftTop);
    ENUM_ENTRY(Right);
    ENUM_ENTRY(RightBottom);
    ENUM_ENTRY(RightTop);
    ENUM_ENTRY(Top);
}
END_ENUM_META;

ENUM_META(o2::CursorType, o2__CursorType)
{
    ENUM_ENTRY(AppStarting);
    ENUM_ENTRY(Arrow);
    ENUM_ENTRY(Cross);
    ENUM_ENTRY(Hand);
    ENUM_ENTRY(Help);
    ENUM_ENTRY(IBeam);
    ENUM_ENTRY(Icon);
    ENUM_ENTRY(No);
    ENUM_ENTRY(SizeAll);
    ENUM_ENTRY(SizeNS);
    ENUM_ENTRY(SizeNeSw);
    ENUM_ENTRY(SizeNwSe);
    ENUM_ENTRY(SizeWE);
    ENUM_ENTRY(UpArrow);
    ENUM_ENTRY(Wait);
}
END_ENUM_META;

ENUM_META(o2::Side, o2__Side)
{
    ENUM_ENTRY(Bottom);
    ENUM_ENTRY(Left);
    ENUM_ENTRY(None);
    ENUM_ENTRY(Right);
    ENUM_ENTRY(Top);
}
END_ENUM_META;

ENUM_META(o2::Corner, o2__Corner)
{
    ENUM_ENTRY(LeftBottom);
    ENUM_ENTRY(LeftTop);
    ENUM_ENTRY(RightBottom);
    ENUM_ENTRY(RightTop);
}
END_ENUM_META;

ENUM_META(o2::TwoDirection, o2__TwoDirection)
{
    ENUM_ENTRY(Horizontal);
    ENUM_ENTRY(Vertical);
}
END_ENUM_META;

ENUM_META(o2::SpriteMode, o2__SpriteMode)
{
    ENUM_ENTRY(Default);
    ENUM_ENTRY(Fill360CCW);
    ENUM_ENTRY(Fill360CW);
    ENUM_ENTRY(FillDownToUp);
    ENUM_ENTRY(FillLeftToRight);
    ENUM_ENTRY(FillRightToLeft);
    ENUM_ENTRY(FillUpToDown);
    ENUM_ENTRY(FixedAspect);
    ENUM_ENTRY(Sliced);
    ENUM_ENTRY(Tiled);
}
END_ENUM_META;

ENUM_META(o2::BlendMode, o2__BlendMode)
{
    ENUM_ENTRY(Add);
    ENUM_ENTRY(Normal);
}
END_ENUM_META;

ENUM_META(o2::VerAlign, o2__VerAlign)
{
    ENUM_ENTRY(Both);
    ENUM_ENTRY(Bottom);
    ENUM_ENTRY(Middle);
    ENUM_ENTRY(Top);
}
END_ENUM_META;

ENUM_META(o2::HorAlign, o2__HorAlign)
{
    ENUM_ENTRY(Both);
    ENUM_ENTRY(Left);
    ENUM_ENTRY(Middle);
    ENUM_ENTRY(Right);
}
END_ENUM_META;

ENUM_META(o2::ProtectSection, o2__ProtectSection)
{
    ENUM_ENTRY(Private);
    ENUM_ENTRY(Protected);
    ENUM_ENTRY(Public);
}
END_ENUM_META;

ENUM_META(o2::Platform, o2__Platform)
{
    ENUM_ENTRY(Android);
    ENUM_ENTRY(Linux);
    ENUM_ENTRY(Mac);
    ENUM_ENTRY(WebAssembly);
    ENUM_ENTRY(Windows);
    ENUM_ENTRY(iOS);
}
END_ENUM_META;

ENUM_META(o2::DeviceType, o2__DeviceType)
{
    ENUM_ENTRY(PC);
    ENUM_ENTRY(Phone);
    ENUM_ENTRY(Tablet);
}
END_ENUM_META;

ENUM_META(o2::LineType, o2__LineType)
{
    ENUM_ENTRY(Dash);
    ENUM_ENTRY(Solid);
}
END_ENUM_META;

ENUM_META(o2::PrimitiveType, o2__PrimitiveType)
{
    ENUM_ENTRY(Line);
    ENUM_ENTRY(Polygon);
    ENUM_ENTRY(PolygonWire);
}
END_ENUM_META;

ENUM_META(o2::PixelFormat, o2__PixelFormat)
{
    ENUM_ENTRY(R8G8B8);
    ENUM_ENTRY(R8G8B8A8);
}
END_ENUM_META;

ENUM_META(o2::TextureFormat, o2__TextureFormat)
{
    ENUM_ENTRY(ASTC4x4);
    ENUM_ENTRY(BC7);
    ENUM_ENTRY(DXT1);
    ENUM_ENTRY(DXT5);
    ENUM_ENTRY(R16G16B16A16F);
    ENUM_ENTRY(R8G8B8A8);
}
END_ENUM_META;

ENUM_META(o2::TextureCompression, o2__TextureCompression)
{
    ENUM_ENTRY(ASTC4x4);
    ENUM_ENTRY(BC7);
    ENUM_ENTRY(DXT1);
    ENUM_ENTRY(DXT5);
    ENUM_ENTRY(None);
}
END_ENUM_META;

ENUM_META(o2::Loop, o2__Loop)
{
    ENUM_ENTRY(None);
    ENUM_ENTRY(PingPong);
    ENUM_ENTRY(Repeat);
}
END_ENUM_META;

ENUM_META(o2::Units, o2__Units)
{
    ENUM_ENTRY(Centimeters);
    ENUM_ENTRY(Inches);
    ENUM_ENTRY(Millimeters);
    ENUM_ENTRY(Pixels);
}
END_ENUM_META;
// --- END META ---
