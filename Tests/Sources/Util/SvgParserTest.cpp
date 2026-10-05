#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "VectorGraphicsTestHelpers.h"

using namespace o2;
using namespace VectorGraphicsTest;

namespace
{
    Vector<VectorSubPath> PathData(const char* data, bool expectedResult = true)
    {
        Vector<VectorSubPath> subPaths;
        EXPECT_EQ(SvgParser::ParsePathData(data, subPaths), expectedResult) << data;
        return subPaths;
    }

    void ExpectPoint(const Vec2F& point, float x, float y, float tolerance = 1e-4f)
    {
        EXPECT_NEAR(point.x, x, tolerance);
        EXPECT_NEAR(point.y, y, tolerance);
    }

    Vector<String> Warnings(const String& svg)
    {
        VectorImage image;
        String error;
        Vector<String> warnings;
        EXPECT_TRUE(SvgParser::Parse(svg, image, error, warnings));
        return warnings;
    }

    bool HasWarning(const Vector<String>& warnings, const char* part)
    {
        for (const String& warning : warnings)
        {
            if (warning.find(part) != std::string::npos)
                return true;
        }

        return false;
    }

    Color4 ParsedColor(const char* text)
    {
        Color4 color;
        EXPECT_TRUE(SvgParser::ParseColor(text, color)) << text;
        return color;
    }
}

TEST(SvgParser, NotXmlFailsWithError)
{
    VectorImage image;
    String error;
    Vector<String> warnings;
    EXPECT_FALSE(SvgParser::Parse("just some text", image, error, warnings));
    EXPECT_FALSE(error.IsEmpty());

    EXPECT_FALSE(SvgParser::Parse("", image, error, warnings));
    EXPECT_FALSE(SvgParser::Parse("<svg><rect", image, error, warnings));
}

TEST(SvgParser, NoSvgRootFailsWithError)
{
    VectorImage image;
    String error;
    Vector<String> warnings;
    EXPECT_FALSE(SvgParser::Parse("<html><body/></html>", image, error, warnings));
    EXPECT_FALSE(error.IsEmpty());
}

TEST(SvgParser, SizeFromWidthAndHeight)
{
    VectorImage image = Parse("<svg width=\"20px\" height=\"12\"/>");
    EXPECT_EQ(image.size, Vec2F(20, 12));
    EXPECT_EQ(image.viewBoxSize, Vec2F(20, 12));
}

TEST(SvgParser, SizeFromViewBox)
{
    VectorImage image = Parse("<svg viewBox=\"5 6 30 40\"/>");
    EXPECT_EQ(image.size, Vec2F(30, 40));
    EXPECT_EQ(image.viewBoxOrigin, Vec2F(5, 6));

    EXPECT_EQ(Parse("<svg width=\"60\" viewBox=\"0 0 30 40\"/>").size, Vec2F(60, 80));
}

TEST(SvgParser, ViewBoxIsMappedToSize)
{
    VectorImage image = Parse("<svg width=\"20\" height=\"20\" viewBox=\"10 10 10 10\">"
                              "<rect x=\"11\" y=\"12\" width=\"2\" height=\"3\" stroke=\"red\" stroke-width=\"1\"/></svg>");
    ASSERT_EQ(image.shapes.size(), 1u);

    Vec2F min, max;
    ASSERT_TRUE(image.shapes[0].GetBounds(min, max));
    ExpectPoint(min, 2, 4);
    ExpectPoint(max, 6, 10);
    EXPECT_FLOAT_EQ(image.shapes[0].strokeWidth, 2.0f);
}

TEST(SvgParser, RectElement)
{
    VectorImage image = Parse(Svg(20, 20, "<rect x=\"2\" y=\"3\" width=\"10\" height=\"5\"/>"));
    ASSERT_EQ(image.shapes.size(), 1u);
    ASSERT_EQ(image.shapes[0].subPaths.size(), 1u);

    const VectorSubPath& path = image.shapes[0].subPaths[0];
    EXPECT_TRUE(path.closed);
    ExpectPoint(path.start, 2, 3);
    ASSERT_EQ(path.segments.size(), 3u);
    ExpectPoint(path.segments[0].end, 12, 3);
    ExpectPoint(path.segments[1].end, 12, 8);
    ExpectPoint(path.segments[2].end, 2, 8);
    EXPECT_FALSE(path.segments[0].cubic);
}

TEST(SvgParser, RoundedRectElement)
{
    VectorImage image = Parse(Svg(20, 20, "<rect x=\"0\" y=\"0\" width=\"10\" height=\"8\" rx=\"2\"/>"
                                          "<rect width=\"10\" height=\"8\" rx=\"50\" ry=\"1\"/>"));
    ASSERT_EQ(image.shapes.size(), 2u);

    const VectorSubPath& path = image.shapes[0].subPaths[0];
    ASSERT_EQ(path.segments.size(), 8u);
    ExpectPoint(path.start, 2, 0);
    EXPECT_TRUE(path.segments[1].cubic);
    ExpectPoint(path.segments[1].end, 10, 2);

    ExpectPoint(image.shapes[1].subPaths[0].start, 5, 0);
    ExpectPoint(image.shapes[1].subPaths[0].segments[1].end, 10, 1);
}

TEST(SvgParser, CircleAndEllipseElements)
{
    VectorImage image = Parse(Svg(40, 40, "<circle cx=\"10\" cy=\"10\" r=\"4\"/>"
                                          "<ellipse cx=\"20\" cy=\"20\" rx=\"6\" ry=\"3\"/>"));
    ASSERT_EQ(image.shapes.size(), 2u);
    EXPECT_EQ(image.shapes[0].subPaths[0].segments.size(), 4u);
    EXPECT_TRUE(image.shapes[0].subPaths[0].closed);

    Vec2F min, max;
    ASSERT_TRUE(image.shapes[0].GetBounds(min, max));
    ExpectPoint(min, 6, 6, 0.01f);
    ExpectPoint(max, 14, 14, 0.01f);

    ASSERT_TRUE(image.shapes[1].GetBounds(min, max));
    ExpectPoint(min, 14, 17, 0.01f);
    ExpectPoint(max, 26, 23, 0.01f);
}

TEST(SvgParser, LinePolylinePolygonElements)
{
    VectorImage image = Parse(Svg(40, 40, "<line x1=\"1\" y1=\"2\" x2=\"3\" y2=\"4\" stroke=\"black\"/>"
                                          "<polyline points=\"0,0 10,0 10,10\" stroke=\"black\"/>"
                                          "<polygon points=\"0 0 10 0 10 10\"/>"));
    ASSERT_EQ(image.shapes.size(), 3u);

    EXPECT_FALSE(image.shapes[0].subPaths[0].closed);
    EXPECT_TRUE(image.shapes[0].fill.IsNone());
    ExpectPoint(image.shapes[0].subPaths[0].start, 1, 2);
    ExpectPoint(image.shapes[0].subPaths[0].segments[0].end, 3, 4);

    EXPECT_FALSE(image.shapes[1].subPaths[0].closed);
    EXPECT_EQ(image.shapes[1].subPaths[0].segments.size(), 2u);
    EXPECT_FALSE(image.shapes[1].fill.IsNone());

    EXPECT_TRUE(image.shapes[2].subPaths[0].closed);
}

TEST(SvgParser, LineWithoutStrokeIsSkipped)
{
    EXPECT_EQ(Parse(Svg(10, 10, "<line x1=\"1\" y1=\"2\" x2=\"3\" y2=\"4\"/>")).shapes.size(), 0u);
}

TEST(SvgParser, PathAbsoluteLines)
{
    auto paths = PathData("M10 10 L20 10 H30 V20 Z");
    ASSERT_EQ(paths.size(), 1u);
    EXPECT_TRUE(paths[0].closed);
    ExpectPoint(paths[0].start, 10, 10);
    ASSERT_EQ(paths[0].segments.size(), 3u);
    ExpectPoint(paths[0].segments[0].end, 20, 10);
    ExpectPoint(paths[0].segments[1].end, 30, 10);
    ExpectPoint(paths[0].segments[2].end, 30, 20);
}

TEST(SvgParser, PathRelativeLines)
{
    auto paths = PathData("m10 10 l10 0 h10 v10 z m5 5 l1 1");
    ASSERT_EQ(paths.size(), 2u);
    ExpectPoint(paths[0].segments[0].end, 20, 10);
    ExpectPoint(paths[0].segments[1].end, 30, 10);
    ExpectPoint(paths[0].segments[2].end, 30, 20);

    EXPECT_FALSE(paths[1].closed);
    ExpectPoint(paths[1].start, 15, 15);
    ExpectPoint(paths[1].segments[0].end, 16, 16);
}

TEST(SvgParser, PathCubicCommands)
{
    auto paths = PathData("M0 0 C1 2 3 4 5 6 S9 10 11 12 c1 1 2 2 3 3 s1 1 2 2");
    ASSERT_EQ(paths.size(), 1u);
    ASSERT_EQ(paths[0].segments.size(), 4u);

    EXPECT_TRUE(paths[0].segments[0].cubic);
    ExpectPoint(paths[0].segments[0].control1, 1, 2);
    ExpectPoint(paths[0].segments[0].control2, 3, 4);
    ExpectPoint(paths[0].segments[0].end, 5, 6);

    ExpectPoint(paths[0].segments[1].control1, 7, 8);
    ExpectPoint(paths[0].segments[1].control2, 9, 10);
    ExpectPoint(paths[0].segments[1].end, 11, 12);

    ExpectPoint(paths[0].segments[2].control1, 12, 13);
    ExpectPoint(paths[0].segments[2].end, 14, 15);

    ExpectPoint(paths[0].segments[3].control1, 15, 16);
    ExpectPoint(paths[0].segments[3].control2, 15, 16);
    ExpectPoint(paths[0].segments[3].end, 16, 17);
}

TEST(SvgParser, PathSmoothCubicWithoutPreviousCurveUsesCurrentPoint)
{
    auto paths = PathData("M2 2 S4 4 6 6");
    ExpectPoint(paths[0].segments[0].control1, 2, 2);
}

TEST(SvgParser, PathQuadraticCommands)
{
    auto paths = PathData("M0 0 Q3 6 6 0 T12 0 q3 3 6 0 t6 0");
    ASSERT_EQ(paths[0].segments.size(), 4u);

    ExpectPoint(paths[0].segments[0].control1, 2, 4);
    ExpectPoint(paths[0].segments[0].control2, 4, 4);
    ExpectPoint(paths[0].segments[0].end, 6, 0);

    // T reflects the previous control point (3,6) around (6,0) into (9,-6)
    ExpectPoint(paths[0].segments[1].control1, 8, -4);
    ExpectPoint(paths[0].segments[1].control2, 10, -4);
    ExpectPoint(paths[0].segments[1].end, 12, 0);

    ExpectPoint(paths[0].segments[2].control1, 14, 2);
    ExpectPoint(paths[0].segments[2].end, 18, 0);

    ExpectPoint(paths[0].segments[3].control1, 20, -2);
    ExpectPoint(paths[0].segments[3].end, 24, 0);
}

TEST(SvgParser, PathArcCommands)
{
    VectorShape shape;
    shape.subPaths = PathData("M0 0 A10 10 0 0 1 20 0");
    ASSERT_EQ(shape.subPaths[0].segments.size(), 2u);
    ExpectPoint(shape.subPaths[0].segments.back().end, 20, 0);

    Vec2F min, max;
    ASSERT_TRUE(shape.GetBounds(min, max));
    ExpectPoint(min, 0, -10, 0.01f);
    ExpectPoint(max, 20, 0, 0.01f);

    shape.subPaths = PathData("M0 0 A10 10 0 0 0 20 0");
    ASSERT_TRUE(shape.GetBounds(min, max));
    ExpectPoint(max, 20, 10, 0.01f);

    shape.subPaths = PathData("M10 0 A10 10 0 1 1 0 10");
    ASSERT_TRUE(shape.GetBounds(min, max));
    ExpectPoint(min, 0, 0, 0.02f);
    ExpectPoint(max, 20, 20, 0.02f);
}

TEST(SvgParser, PathRelativeArcWithCompactFlags)
{
    auto paths = PathData("M5 5 a10 10 0 0120 0");
    ASSERT_EQ(paths.size(), 1u);
    ExpectPoint(paths[0].segments.back().end, 25, 5);
}

TEST(SvgParser, PathArcWithTooSmallRadiusIsScaled)
{
    VectorShape shape;
    shape.subPaths = PathData("M0 0 A1 1 0 0 1 20 0");

    Vec2F min, max;
    ASSERT_TRUE(shape.GetBounds(min, max));
    ExpectPoint(min, 0, -10, 0.01f);

    EXPECT_FALSE(PathData("M0 0 A0 5 0 0 1 20 0")[0].segments[0].cubic);
}

TEST(SvgParser, PathNumberGrammar)
{
    auto paths = PathData("M1.5.5-1-2 1e1,2E-1+.5-.25");
    ASSERT_EQ(paths[0].segments.size(), 3u);
    ExpectPoint(paths[0].start, 1.5f, 0.5f);
    ExpectPoint(paths[0].segments[0].end, -1, -2);
    ExpectPoint(paths[0].segments[1].end, 10, 0.2f);
    ExpectPoint(paths[0].segments[2].end, 0.5f, -0.25f);
}

TEST(SvgParser, PathImplicitRepeats)
{
    auto paths = PathData("M0 0 10 0 10 10 m1 1 2 0 0 2 h1 2 v1 2");
    ASSERT_EQ(paths.size(), 2u);
    ASSERT_EQ(paths[0].segments.size(), 2u);
    ExpectPoint(paths[0].segments[1].end, 10, 10);

    ExpectPoint(paths[1].start, 11, 11);
    ASSERT_EQ(paths[1].segments.size(), 6u);
    ExpectPoint(paths[1].segments[0].end, 13, 11);
    ExpectPoint(paths[1].segments[1].end, 13, 13);
    ExpectPoint(paths[1].segments[3].end, 16, 13);
    ExpectPoint(paths[1].segments[5].end, 16, 16);
}

TEST(SvgParser, PathSyntaxErrorKeepsParsedPrefix)
{
    auto paths = PathData("M0 0 L10 10 L 5 x", false);
    ASSERT_EQ(paths.size(), 1u);
    EXPECT_EQ(paths[0].segments.size(), 1u);

    EXPECT_EQ(PathData("L10 10", false).size(), 0u);
    EXPECT_EQ(PathData("M0 0 X", false).size(), 0u);

    EXPECT_TRUE(HasWarning(Warnings(Svg(10, 10, "<path d=\"M0 0 L5 5 L\"/>")), "path data"));
}

TEST(SvgParser, Colors)
{
    EXPECT_EQ(ParsedColor("#f80"), Color4(255, 136, 0, 255));
    EXPECT_EQ(ParsedColor("#1A2b3C"), Color4(26, 43, 60, 255));
    EXPECT_EQ(ParsedColor("rgb(10, 20,30)"), Color4(10, 20, 30, 255));
    EXPECT_EQ(ParsedColor("rgb(100%, 0%, 50%)"), Color4(255, 0, 128, 255));
    EXPECT_EQ(ParsedColor("rgba(1,2,3,0.5)"), Color4(1, 2, 3, 128));
    EXPECT_EQ(ParsedColor(" Black "), Color4(0, 0, 0, 255));
    EXPECT_EQ(ParsedColor("white"), Color4(255, 255, 255, 255));
    EXPECT_EQ(ParsedColor("orange"), Color4(255, 165, 0, 255));
    EXPECT_EQ(ParsedColor("transparent").a, 0);
    EXPECT_EQ(ParsedColor("currentColor"), Color4(0, 0, 0, 255));

    Color4 color;
    EXPECT_FALSE(SvgParser::ParseColor("#12", color));
    EXPECT_FALSE(SvgParser::ParseColor("#ggg", color));
    EXPECT_FALSE(SvgParser::ParseColor("notacolor", color));
    EXPECT_FALSE(SvgParser::ParseColor("rgb(1,2)", color));
}

TEST(SvgParser, FillNoneAndUnknownColor)
{
    VectorImage image = Parse(Svg(10, 10, "<rect width=\"5\" height=\"5\" fill=\"none\" stroke=\"#f00\"/>"));
    ASSERT_EQ(image.shapes.size(), 1u);
    EXPECT_TRUE(image.shapes[0].fill.IsNone());
    EXPECT_EQ(image.shapes[0].stroke.color, Color4(255, 0, 0, 255));

    EXPECT_EQ(Parse(Svg(10, 10, "<rect width=\"5\" height=\"5\" fill=\"none\"/>")).shapes.size(), 0u);
    EXPECT_TRUE(HasWarning(Warnings(Svg(10, 10, "<rect width=\"5\" height=\"5\" fill=\"blurple\"/>")), "blurple"));
}

TEST(SvgParser, Transforms)
{
    Basis transform;

    ASSERT_TRUE(SvgParser::ParseTransform("translate(10, 5)", transform));
    ExpectPoint(transform.Transform(Vec2F(1, 1)), 11, 6);

    ASSERT_TRUE(SvgParser::ParseTransform("translate(10)", transform));
    ExpectPoint(transform.Transform(Vec2F(1, 1)), 11, 1);

    ASSERT_TRUE(SvgParser::ParseTransform("scale(2 3)", transform));
    ExpectPoint(transform.Transform(Vec2F(1, 1)), 2, 3);

    ASSERT_TRUE(SvgParser::ParseTransform("scale(2)", transform));
    ExpectPoint(transform.Transform(Vec2F(1, 1)), 2, 2);

    ASSERT_TRUE(SvgParser::ParseTransform("rotate(90)", transform));
    ExpectPoint(transform.Transform(Vec2F(1, 0)), 0, 1);

    ASSERT_TRUE(SvgParser::ParseTransform("rotate(90, 10, 10)", transform));
    ExpectPoint(transform.Transform(Vec2F(11, 10)), 10, 11);

    ASSERT_TRUE(SvgParser::ParseTransform("matrix(1 2 3 4 5 6)", transform));
    ExpectPoint(transform.Transform(Vec2F(1, 1)), 9, 12);

    ASSERT_TRUE(SvgParser::ParseTransform("skewX(45)", transform));
    ExpectPoint(transform.Transform(Vec2F(0, 2)), 2, 2);

    ASSERT_TRUE(SvgParser::ParseTransform("skewY(45)", transform));
    ExpectPoint(transform.Transform(Vec2F(2, 0)), 2, 2);

    ASSERT_TRUE(SvgParser::ParseTransform("translate(10,0) scale(2)", transform));
    ExpectPoint(transform.Transform(Vec2F(1, 1)), 12, 2);

    EXPECT_FALSE(SvgParser::ParseTransform("spin(10)", transform));
    EXPECT_FALSE(SvgParser::ParseTransform("translate(10", transform));
}

TEST(SvgParser, TransformsAreBakedThroughGroups)
{
    VectorImage image = Parse(Svg(40, 40, "<g transform=\"translate(10 10)\"><g transform=\"scale(2)\">"
                                          "<rect x=\"1\" y=\"1\" width=\"2\" height=\"2\" transform=\"translate(1,0)\""
                                          " stroke=\"black\" stroke-width=\"1.5\"/></g></g>"));
    ASSERT_EQ(image.shapes.size(), 1u);

    Vec2F min, max;
    ASSERT_TRUE(image.shapes[0].GetBounds(min, max));
    ExpectPoint(min, 14, 12);
    ExpectPoint(max, 18, 16);
    EXPECT_FLOAT_EQ(image.shapes[0].strokeWidth, 3.0f);
}

TEST(SvgParser, StrokeProperties)
{
    VectorImage image = Parse(Svg(10, 10, "<path d=\"M0 0L5 5\" stroke=\"#000\" stroke-width=\"2.5\" stroke-linecap=\"round\""
                                          " stroke-linejoin=\"bevel\" stroke-miterlimit=\"7\" stroke-opacity=\"0.25\""
                                          " fill-opacity=\"0.75\" fill-rule=\"evenodd\"/>"
                                          "<path d=\"M0 0L5 5\" stroke=\"#000\" stroke-linecap=\"square\" stroke-linejoin=\"round\"/>"));
    ASSERT_EQ(image.shapes.size(), 2u);

    const VectorShape& shape = image.shapes[0];
    EXPECT_FLOAT_EQ(shape.strokeWidth, 2.5f);
    EXPECT_EQ(shape.strokeCap, VectorLineCap::Round);
    EXPECT_EQ(shape.strokeJoin, VectorLineJoin::Bevel);
    EXPECT_FLOAT_EQ(shape.strokeMiterLimit, 7.0f);
    EXPECT_FLOAT_EQ(shape.strokeOpacity, 0.25f);
    EXPECT_FLOAT_EQ(shape.fillOpacity, 0.75f);
    EXPECT_EQ(shape.fillRule, VectorFillRule::EvenOdd);

    EXPECT_EQ(image.shapes[1].strokeCap, VectorLineCap::Square);
    EXPECT_EQ(image.shapes[1].strokeJoin, VectorLineJoin::Round);
    EXPECT_EQ(image.shapes[1].fillRule, VectorFillRule::NonZero);
    EXPECT_FLOAT_EQ(image.shapes[1].strokeMiterLimit, 4.0f);
}

TEST(SvgParser, DefaultsAreBlackFillAndNoStroke)
{
    VectorImage image = Parse(Svg(10, 10, "<rect width=\"5\" height=\"5\"/>"));
    ASSERT_EQ(image.shapes.size(), 1u);
    EXPECT_EQ(image.shapes[0].fill.type, VectorPaintType::Solid);
    EXPECT_EQ(image.shapes[0].fill.color, Color4(0, 0, 0, 255));
    EXPECT_TRUE(image.shapes[0].stroke.IsNone());
}

TEST(SvgParser, StyleAttributeOverridesPresentationAttributes)
{
    VectorImage image = Parse(Svg(10, 10, "<rect width=\"5\" height=\"5\" fill=\"red\" stroke-width=\"3\""
                                          " style=\"fill: #00ff00; stroke:blue;stroke-width:2 ; opacity:0.5\"/>"));
    ASSERT_EQ(image.shapes.size(), 1u);
    EXPECT_EQ(image.shapes[0].fill.color, Color4(0, 255, 0, 255));
    EXPECT_EQ(image.shapes[0].stroke.color, Color4(0, 0, 255, 255));
    EXPECT_FLOAT_EQ(image.shapes[0].strokeWidth, 2.0f);
    EXPECT_FLOAT_EQ(image.shapes[0].opacity, 0.5f);
}

TEST(SvgParser, ClassSelectorsOfStyleElement)
{
    VectorImage image = Parse(Svg(10, 10, "<style>.a, .b { fill: #ff0000; } .b { stroke: #0000ff }</style>"
                                          "<rect class=\"a\" width=\"5\" height=\"5\"/>"
                                          "<rect class=\"x b\" width=\"5\" height=\"5\"/>"));
    ASSERT_EQ(image.shapes.size(), 2u);
    EXPECT_EQ(image.shapes[0].fill.color, Color4(255, 0, 0, 255));
    EXPECT_TRUE(image.shapes[0].stroke.IsNone());
    EXPECT_EQ(image.shapes[1].fill.color, Color4(255, 0, 0, 255));
    EXPECT_EQ(image.shapes[1].stroke.color, Color4(0, 0, 255, 255));
}

TEST(SvgParser, AttributesAreInheritedThroughGroups)
{
    VectorImage image = Parse(Svg(10, 10, "<g fill=\"#ff0000\" stroke=\"#00ff00\" stroke-width=\"3\" stroke-linecap=\"round\">"
                                          "<g fill-rule=\"evenodd\"><rect width=\"5\" height=\"5\"/>"
                                          "<rect width=\"5\" height=\"5\" fill=\"#0000ff\" stroke=\"none\"/></g></g>"
                                          "<rect width=\"5\" height=\"5\"/>"));
    ASSERT_EQ(image.shapes.size(), 3u);

    EXPECT_EQ(image.shapes[0].fill.color, Color4(255, 0, 0, 255));
    EXPECT_EQ(image.shapes[0].stroke.color, Color4(0, 255, 0, 255));
    EXPECT_FLOAT_EQ(image.shapes[0].strokeWidth, 3.0f);
    EXPECT_EQ(image.shapes[0].strokeCap, VectorLineCap::Round);
    EXPECT_EQ(image.shapes[0].fillRule, VectorFillRule::EvenOdd);

    EXPECT_EQ(image.shapes[1].fill.color, Color4(0, 0, 255, 255));
    EXPECT_TRUE(image.shapes[1].stroke.IsNone());

    EXPECT_EQ(image.shapes[2].fill.color, Color4(0, 0, 0, 255));
    EXPECT_TRUE(image.shapes[2].stroke.IsNone());
}

TEST(SvgParser, GroupOpacityIsMultipliedIntoChildren)
{
    VectorImage image = Parse(Svg(10, 10, "<g opacity=\"0.5\"><g opacity=\"0.5\">"
                                          "<rect width=\"5\" height=\"5\" opacity=\"0.5\" fill-opacity=\"0.5\"/></g>"
                                          "<rect width=\"5\" height=\"5\"/></g>"));
    ASSERT_EQ(image.shapes.size(), 2u);
    EXPECT_FLOAT_EQ(image.shapes[0].opacity, 0.125f);
    EXPECT_FLOAT_EQ(image.shapes[0].fillOpacity, 0.5f);
    EXPECT_FLOAT_EQ(image.shapes[1].opacity, 0.5f);
}

TEST(SvgParser, HiddenAndEmptyElementsAreSkipped)
{
    VectorImage image = Parse(Svg(10, 10, "<rect width=\"5\" height=\"5\" display=\"none\"/>"
                                          "<g style=\"display:none\"><rect width=\"5\" height=\"5\"/></g>"
                                          "<rect width=\"5\" height=\"5\" visibility=\"hidden\"/>"
                                          "<rect width=\"0\" height=\"5\"/><circle r=\"0\"/><path d=\"\"/>"
                                          "<defs><rect id=\"r\" width=\"5\" height=\"5\"/></defs>"));
    EXPECT_EQ(image.shapes.size(), 0u);
}

TEST(SvgParser, LinearGradientInBoundingBoxUnits)
{
    VectorImage image = Parse(Svg(40, 40, "<defs><linearGradient id=\"g\">"
                                          "<stop offset=\"0\" stop-color=\"#ff0000\"/>"
                                          "<stop offset=\"50%\" style=\"stop-color:#00ff00;stop-opacity:0.5\"/>"
                                          "<stop offset=\"1\" stop-color=\"blue\" stop-opacity=\"0.25\"/>"
                                          "</linearGradient></defs>"
                                          "<rect x=\"10\" y=\"20\" width=\"20\" height=\"10\" fill=\"url(#g)\"/>"));
    ASSERT_EQ(image.shapes.size(), 1u);

    const VectorPaint& fill = image.shapes[0].fill;
    ASSERT_EQ(fill.type, VectorPaintType::LinearGradient);
    ASSERT_EQ(fill.stops.size(), 3u);
    EXPECT_EQ(fill.stops[0].color, Color4(255, 0, 0, 255));
    EXPECT_FLOAT_EQ(fill.stops[1].offset, 0.5f);
    EXPECT_EQ(fill.stops[1].color, Color4(0, 255, 0, 128));
    EXPECT_EQ(fill.stops[2].color, Color4(0, 0, 255, 64));

    EXPECT_NEAR(fill.GetRampOffset(Vec2F(10, 25)), 0.0f, 1e-4f);
    EXPECT_NEAR(fill.GetRampOffset(Vec2F(20, 21)), 0.5f, 1e-4f);
    EXPECT_NEAR(fill.GetRampOffset(Vec2F(30, 29)), 1.0f, 1e-4f);
    EXPECT_EQ(fill.GetRampColor(0.25f), Color4(128, 128, 0, 192));
}

TEST(SvgParser, LinearGradientInUserSpaceWithTransforms)
{
    VectorImage image = Parse(Svg(40, 40, "<linearGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"10\""
                                          " gradientTransform=\"translate(0 5)\">"
                                          "<stop offset=\"0\" stop-color=\"#000\"/><stop offset=\"1\" stop-color=\"#fff\"/></linearGradient>"
                                          "<g transform=\"scale(2)\"><rect width=\"10\" height=\"20\" style=\"fill:url(#g)\"/></g>"));
    ASSERT_EQ(image.shapes.size(), 1u);

    const VectorPaint& fill = image.shapes[0].fill;
    ASSERT_EQ(fill.type, VectorPaintType::LinearGradient);
    EXPECT_NEAR(fill.GetRampOffset(Vec2F(3, 10)), 0.0f, 1e-4f);
    EXPECT_NEAR(fill.GetRampOffset(Vec2F(7, 20)), 0.5f, 1e-4f);
    EXPECT_NEAR(fill.GetRampOffset(Vec2F(0, 30)), 1.0f, 1e-4f);
}

TEST(SvgParser, RadialGradient)
{
    VectorImage image = Parse(Svg(40, 40, "<radialGradient id=\"g\"><stop offset=\"0\" stop-color=\"#fff\"/>"
                                          "<stop offset=\"1\" stop-color=\"#000\"/></radialGradient>"
                                          "<radialGradient id=\"u\" gradientUnits=\"userSpaceOnUse\" cx=\"10\" cy=\"10\" r=\"5\" fx=\"8\" xlink:href=\"#g\"/>"
                                          "<rect x=\"10\" y=\"10\" width=\"20\" height=\"20\" fill=\"url(#g)\"/>"
                                          "<rect width=\"20\" height=\"20\" fill=\"url(#u)\"/>"));
    ASSERT_EQ(image.shapes.size(), 2u);

    const VectorPaint& fill = image.shapes[0].fill;
    ASSERT_EQ(fill.type, VectorPaintType::RadialGradient);
    EXPECT_NEAR(fill.GetRampOffset(Vec2F(20, 20)), 0.0f, 1e-4f);
    EXPECT_NEAR(fill.GetRampOffset(Vec2F(25, 20)), 0.5f, 1e-4f);
    EXPECT_NEAR(fill.GetRampOffset(Vec2F(20, 10)), 1.0f, 1e-4f);

    const VectorPaint& focal = image.shapes[1].fill;
    ASSERT_EQ(focal.type, VectorPaintType::RadialGradient);
    EXPECT_EQ(focal.stops.size(), 2u);
    EXPECT_NEAR(focal.GetRampOffset(Vec2F(8, 10)), 0.0f, 1e-4f);
    EXPECT_NEAR(focal.GetRampOffset(Vec2F(15, 10)), 1.0f, 1e-4f);
    EXPECT_NEAR(focal.GetRampOffset(Vec2F(5, 10)), 1.0f, 1e-4f);
}

TEST(SvgParser, GradientInheritsStopsAndAttributesByHref)
{
    VectorImage image = Parse(Svg(40, 40, "<linearGradient id=\"base\" gradientUnits=\"userSpaceOnUse\" x1=\"0\" x2=\"40\">"
                                          "<stop offset=\"0\" stop-color=\"#f00\"/><stop offset=\"1\" stop-color=\"#00f\"/></linearGradient>"
                                          "<linearGradient id=\"mid\" href=\"#base\"/>"
                                          "<linearGradient id=\"top\" xlink:href=\"#mid\" x2=\"20\"/>"
                                          "<rect width=\"40\" height=\"40\" fill=\"url(#top)\"/>"
                                          "<rect width=\"40\" height=\"40\" fill=\"url('#mid')\"/>"));
    ASSERT_EQ(image.shapes.size(), 2u);

    ASSERT_EQ(image.shapes[0].fill.stops.size(), 2u);
    EXPECT_EQ(image.shapes[0].fill.stops[1].color, Color4(0, 0, 255, 255));
    EXPECT_NEAR(image.shapes[0].fill.GetRampOffset(Vec2F(10, 0)), 0.5f, 1e-4f);
    EXPECT_NEAR(image.shapes[1].fill.GetRampOffset(Vec2F(10, 0)), 0.25f, 1e-4f);
}

TEST(SvgParser, GradientDegenerateCases)
{
    VectorImage image = Parse(Svg(40, 40, "<linearGradient id=\"one\"><stop offset=\"0\" stop-color=\"#0f0\"/></linearGradient>"
                                          "<linearGradient id=\"none\"/>"
                                          "<rect width=\"4\" height=\"4\" fill=\"url(#one)\"/>"
                                          "<rect width=\"4\" height=\"4\" fill=\"url(#none)\"/>"
                                          "<rect width=\"4\" height=\"4\" fill=\"url(#missing) #123456\"/>"
                                          "<rect width=\"4\" height=\"4\" fill=\"url(#missing)\"/>"));
    ASSERT_EQ(image.shapes.size(), 2u);
    EXPECT_EQ(image.shapes[0].fill.type, VectorPaintType::Solid);
    EXPECT_EQ(image.shapes[0].fill.color, Color4(0, 255, 0, 255));
    EXPECT_EQ(image.shapes[1].fill.color, Color4(0x12, 0x34, 0x56, 255));
}

TEST(SvgParser, UnsupportedElementsProduceWarnings)
{
    String svg = Svg(20, 20, "<text x=\"1\" y=\"1\">hi</text><image href=\"a.png\"/><use href=\"#r\"/>"
                             "<clipPath id=\"c\"><rect width=\"1\" height=\"1\"/></clipPath><mask id=\"m\"/>"
                             "<filter id=\"f\"/><pattern id=\"p\"/><foreignObject/>"
                             "<rect id=\"r\" width=\"5\" height=\"5\" clip-path=\"url(#c)\" stroke-dasharray=\"1 2\"/>");

    Vector<String> warnings = Warnings(svg);
    for (const char* name : { "<text>", "<image>", "<filter>", "<pattern>", "clip-path", "stroke-dasharray" })
        EXPECT_TRUE(HasWarning(warnings, name)) << name;

    EXPECT_FALSE(HasWarning(warnings, "<clipPath>"));
    EXPECT_EQ(Parse(svg).shapes.size(), 2u);
}

TEST(SvgParser, PatternPaintWarnsAndDrawsNothing)
{
    String svg = Svg(20, 20, "<pattern id=\"p\"/><rect width=\"5\" height=\"5\" fill=\"url(#p)\"/>");
    EXPECT_TRUE(HasWarning(Warnings(svg), "paint server"));
    EXPECT_EQ(Parse(svg).shapes.size(), 0u);
}

TEST(SvgParser, MalformedValuesDoNotCrash)
{
    VectorImage image = Parse("<svg width=\"abc\" height=\"-5\" viewBox=\"0 0 0 x\">"
                              "<rect x=\"nan\" y=\"1e999\" width=\"5\" height=\"5\" transform=\"rotate(\" fill=\"url(\"/>"
                              "<path d=\"M 1e400 5 L 3 4 Q\" stroke-width=\"-3\" stroke=\"red\"/>"
                              "<polygon points=\"1,2,3\"/><polyline points=\"\"/><circle r=\"-1\"/>"
                              "<g transform=\"matrix(1 2 3)\"><ellipse rx=\"1\"/></g>"
                              "<linearGradient id=\"g\" href=\"#g\"/><rect width=\"1\" height=\"1\" fill=\"url(#g)\"/>"
                              "</svg>");

    VectorMesh mesh;
    VectorTessellator::Tessellate(image, mesh);
    SUCCEED();
}

TEST(SvgParser, XmlDeclarationCommentsAndNamespacePrefixes)
{
    VectorImage image = Parse("<?xml version=\"1.0\" encoding=\"UTF-8\"?><!-- comment -->"
                              "<svg:svg xmlns:svg=\"http://www.w3.org/2000/svg\" width=\"10\" height=\"10\">"
                              "<svg:title>t</svg:title><svg:rect width=\"5\" height=\"5\" fill=\"&#35;ff0000\"/></svg:svg>");
    ASSERT_EQ(image.shapes.size(), 1u);
    EXPECT_EQ(image.shapes[0].fill.color, Color4(255, 0, 0, 255));
}

TEST(SvgParser, RootSizeUnits)
{
    const char* svg = "<svg width=\"20pt\" height=\"15pt\" viewBox=\"0 0 20 15\"><rect width=\"20\" height=\"15\"/></svg>";

    VectorImage image = Parse(svg);
    ExpectPoint(image.size, 26.6667f, 20.0f, 1e-3f);
    ExpectPoint(image.shapes[0].subPaths[0].segments[1].end, 26.6667f, 20.0f, 1e-3f);

    SvgParseOptions options;
    options.unitsAsPixels = true;

    String error;
    Vector<String> warnings;
    ASSERT_TRUE(SvgParser::Parse(svg, image, error, warnings, options));
    ExpectPoint(image.size, 20.0f, 15.0f);
    ExpectPoint(image.shapes[0].subPaths[0].segments[1].end, 20.0f, 15.0f);

    ExpectPoint(Parse("<svg width=\"25.4mm\" height=\"1in\"/>").size, 96.0f, 96.0f, 1e-3f);
    ExpectPoint(Parse("<svg width=\"100%\" height=\"100%\" viewBox=\"0 0 30 10\"/>").size, 30.0f, 10.0f);
}

TEST(SvgParser, PercentAndUnknownUnitsInGeometry)
{
    VectorImage image = Parse("<svg width=\"20\" height=\"10\"><rect width=\"100%\" height=\"50%\"/>"
                              "<circle cx=\"50%\" cy=\"50%\" r=\"2\" stroke=\"red\" stroke-width=\"10%\"/></svg>");
    ASSERT_EQ(image.shapes.size(), 2u);
    ExpectPoint(image.shapes[0].subPaths[0].segments[1].end, 20.0f, 5.0f);
    ExpectPoint(image.shapes[1].subPaths[0].start, 12.0f, 5.0f);
    EXPECT_NEAR(image.shapes[1].strokeWidth, 0.1f*sqrtf((400.0f + 100.0f)*0.5f), 1e-3f);

    EXPECT_TRUE(HasWarning(Warnings(Svg(20, 20, "<rect width=\"1em\" height=\"2\"/>")), "unsupported unit"));
    EXPECT_TRUE(Warnings(Svg(20, 20, "<rect width=\"3px\" height=\"2\"/>")).IsEmpty());
}

TEST(SvgParser, UseElement)
{
    VectorImage image = Parse(Svg(40, 40, "<defs><rect id=\"r\" width=\"4\" height=\"2\"/>"
                                          "<g id=\"g\" fill=\"#00f\"><rect x=\"1\" width=\"1\" height=\"1\"/><use xlink:href=\"#r\"/></g>"
                                          "<symbol id=\"s\"><circle r=\"1\"/></symbol></defs>"
                                          "<use href=\"#r\" x=\"10\" y=\"20\" fill=\"#f00\"/>"
                                          "<use xlink:href=\"#g\" transform=\"translate(5 5) scale(2)\"/>"
                                          "<use href=\"#s\" x=\"3\"/>"));
    ASSERT_EQ(image.shapes.size(), 4u);

    ExpectPoint(image.shapes[0].subPaths[0].start, 10.0f, 20.0f);
    ExpectPoint(image.shapes[0].subPaths[0].segments[1].end, 14.0f, 22.0f);
    EXPECT_EQ(image.shapes[0].fill.color, Color4(255, 0, 0, 255));

    ExpectPoint(image.shapes[1].subPaths[0].start, 7.0f, 5.0f);
    EXPECT_EQ(image.shapes[1].fill.color, Color4(0, 0, 255, 255));
    ExpectPoint(image.shapes[2].subPaths[0].segments[1].end, 13.0f, 9.0f);
    ExpectPoint(image.shapes[3].subPaths[0].start, 4.0f, 0.0f);

    Vector<String> warnings = Warnings(Svg(20, 20, "<use href=\"#missing\"/><g id=\"a\"><use href=\"#a\"/><rect width=\"1\" height=\"1\"/></g>"));
    EXPECT_TRUE(HasWarning(warnings, "#missing"));
    EXPECT_TRUE(HasWarning(warnings, "refers to itself"));
}

TEST(SvgParser, PdfToCairoDocument)
{
    VectorImage image;
    String error;
    Vector<String> warnings;
    SvgParseOptions options;
    options.unitsAsPixels = true;

    ASSERT_TRUE(SvgParser::Parse(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"20pt\" height=\"20pt\""
        " viewBox=\"0 0 20 20\" version=\"1.2\"><defs><clipPath id=\"clip-0\"><path d=\"M 0 0 L 20 0 L 20 20 L 0 20 Z\"/></clipPath>"
        "<g id=\"source-5\"><path fill-rule=\"nonzero\" fill=\"rgb(33.724976%, 40%, 100%)\" fill-opacity=\"0.5\""
        " d=\"M 2 2 L 6 2 L 6 6 L 2 6 Z M 2 2 \"/></g></defs><g clip-path=\"url(#clip-0)\">"
        "<use xlink:href=\"#source-5\" transform=\"matrix(1, 0, 0, -1, 4, 20)\"/></g></svg>", image, error, warnings, options));

    ExpectPoint(image.size, 20.0f, 20.0f);
    ASSERT_EQ(image.shapes.size(), 1u);
    EXPECT_EQ(image.shapes[0].fill.color, Color4(86, 102, 255, 255));
    EXPECT_FLOAT_EQ(image.shapes[0].fillOpacity, 0.5f);
    ExpectPoint(image.shapes[0].subPaths[0].start, 6.0f, 18.0f);
    ExpectPoint(image.shapes[0].subPaths[0].segments[1].end, 10.0f, 14.0f);
    EXPECT_TRUE(HasWarning(warnings, "clip-path"));
}

TEST(SvgParser, SwitchIsReadAsGroup)
{
    VectorImage image = Parse(Svg(20, 20, "<switch><foreignObject width=\"1\" height=\"1\"/><g><rect width=\"3\" height=\"3\"/></g></switch>"));
    EXPECT_EQ(image.shapes.size(), 1u);
    EXPECT_TRUE(Warnings(Svg(20, 20, "<switch><foreignObject/><g/></switch>")).IsEmpty());
}

TEST(SvgParser, CssCommentsRulesAndImportant)
{
    String svg = Svg(20, 20, "<style>/* c { */ @import url(a.css); .a{fill:#ff0000} /* d */ @media print { .a{fill:#000} }"
                             " .b { stroke : #00f ; }</style>"
                             "<rect class=\"a b\" width=\"1\" height=\"1\"/>"
                             "<rect fill=\"red\" style=\"fill:#00ff00 !important\" width=\"1\" height=\"1\"/>");
    VectorImage image = Parse(svg);
    ASSERT_EQ(image.shapes.size(), 2u);
    EXPECT_EQ(image.shapes[0].fill.color, Color4(255, 0, 0, 255));
    EXPECT_EQ(image.shapes[0].stroke.color, Color4(0, 0, 255, 255));
    EXPECT_EQ(image.shapes[1].fill.color, Color4(0, 255, 0, 255));
    EXPECT_FALSE(HasWarning(Warnings(svg), "selector"));
}

TEST(SvgParser, NamedHslAndAlphaColors)
{
    auto color = [](const char* text)
    {
        Color4 res;
        EXPECT_TRUE(SvgParser::ParseColor(text, res)) << text;
        return res;
    };

    EXPECT_EQ(color("lightyellow"), Color4(255, 255, 224, 255));
    EXPECT_EQ(color("DarkSlateGray"), Color4(47, 79, 79, 255));
    EXPECT_EQ(color("aliceblue"), Color4(240, 248, 255, 255));
    EXPECT_EQ(color("rebeccapurple"), Color4(102, 51, 153, 255));
    EXPECT_EQ(color("hsl(0, 100%, 50%)"), Color4(255, 0, 0, 255));
    EXPECT_EQ(color("hsl(120 100% 25%)"), Color4(0, 128, 0, 255));
    EXPECT_EQ(color("hsla(240, 100%, 50%, 0.5)"), Color4(0, 0, 255, 128));
    EXPECT_EQ(color("hsl(210deg, 50%, 40%)"), Color4(51, 102, 153, 255));
    EXPECT_EQ(color("  #11223380 "), Color4(0x11, 0x22, 0x33, 0x80));
    EXPECT_EQ(color("#1238"), Color4(0x11, 0x22, 0x33, 0x88));
    EXPECT_EQ(color("rgb(33.724976%, 40%, 100%)"), Color4(86, 102, 255, 255));
    EXPECT_EQ(color("rgba(255, 0, 0, 50%)"), Color4(255, 0, 0, 128));
    EXPECT_EQ(color("rgb(300, -5, 0)"), Color4(255, 0, 0, 255));

    Color4 unused;
    EXPECT_FALSE(SvgParser::ParseColor("rgb(1e999, 0, 0)", unused));
    EXPECT_FALSE(SvgParser::ParseColor("notacolor", unused));

    VectorImage image = Parse(Svg(20, 20, "<g fill=\"red\"><rect fill=\"lightyellow\" width=\"1\" height=\"1\"/></g>"));
    EXPECT_EQ(image.shapes[0].fill.color, Color4(255, 255, 224, 255));
}

TEST(SvgParser, CurrentColor)
{
    VectorImage image = Parse("<svg width=\"9\" height=\"9\" color=\"red\"><rect fill=\"currentColor\" width=\"1\" height=\"1\"/>"
                              "<g color=\"#00f\"><rect fill=\"none\" stroke=\"currentcolor\" width=\"1\" height=\"1\"/></g>"
                              "<rect style=\"color:lime;fill:currentColor\" width=\"1\" height=\"1\"/></svg>");
    ASSERT_EQ(image.shapes.size(), 3u);
    EXPECT_EQ(image.shapes[0].fill.color, Color4(255, 0, 0, 255));
    EXPECT_EQ(image.shapes[1].stroke.color, Color4(0, 0, 255, 255));
    EXPECT_EQ(image.shapes[2].fill.color, Color4(0, 255, 0, 255));
}

TEST(SvgParser, PreserveAspectRatio)
{
    auto corner = [](const char* aspectRatio)
    {
        String svg = String("<svg width=\"40\" height=\"20\" viewBox=\"0 0 20 20\"") + aspectRatio +
            "><rect width=\"20\" height=\"20\"/></svg>";

        const VectorSubPath path = Parse(svg).shapes[0].subPaths[0];
        return RectF(path.start.x, path.start.y, path.segments[1].end.x, path.segments[1].end.y);
    };

    EXPECT_EQ(corner(""), RectF(10, 0, 30, 20));
    EXPECT_EQ(corner(" preserveAspectRatio=\"xMinYMin meet\""), RectF(0, 0, 20, 20));
    EXPECT_EQ(corner(" preserveAspectRatio=\"xMaxYMid\""), RectF(20, 0, 40, 20));
    EXPECT_EQ(corner(" preserveAspectRatio=\"xMidYMid slice\""), RectF(0, -10, 40, 30));
    EXPECT_EQ(corner(" preserveAspectRatio=\"xMidYMax slice\""), RectF(0, -20, 40, 20));
    EXPECT_EQ(corner(" preserveAspectRatio=\"none\""), RectF(0, 0, 40, 20));
}

TEST(SvgParser, NestedSvgAndRootTransform)
{
    VectorImage image = Parse(Svg(20, 20, "<svg x=\"10\" y=\"5\" width=\"10\" height=\"10\" viewBox=\"0 0 100 100\">"
                                          "<rect width=\"100\" height=\"50%\"/></svg>"
                                          "<svg x=\"2\" y=\"2\"><rect width=\"50%\" height=\"1\"/></svg>"));
    ASSERT_EQ(image.shapes.size(), 2u);
    ExpectPoint(image.shapes[0].subPaths[0].start, 10.0f, 5.0f);
    ExpectPoint(image.shapes[0].subPaths[0].segments[1].end, 20.0f, 10.0f);
    ExpectPoint(image.shapes[1].subPaths[0].start, 2.0f, 2.0f);
    ExpectPoint(image.shapes[1].subPaths[0].segments[1].end, 12.0f, 3.0f);

    image = Parse("<svg width=\"20\" height=\"20\" viewBox=\"0 0 10 10\" transform=\"translate(1 2)\">"
                  "<rect width=\"5\" height=\"5\"/></svg>");
    ExpectPoint(image.shapes[0].subPaths[0].start, 1.0f, 2.0f);
    ExpectPoint(image.shapes[0].subPaths[0].segments[1].end, 11.0f, 12.0f);
}

TEST(SvgParser, VisibilityIsInheritedAndDisplayCutsTheBranch)
{
    EXPECT_EQ(Parse(Svg(9, 9, "<g visibility=\"hidden\"><rect visibility=\"visible\" width=\"1\" height=\"1\"/>"
                              "<rect width=\"1\" height=\"1\"/></g>")).shapes.size(), 1u);
    EXPECT_EQ(Parse(Svg(9, 9, "<g display=\"none\"><rect visibility=\"visible\" display=\"inline\" width=\"1\" height=\"1\"/></g>"))
              .shapes.size(), 0u);
    EXPECT_EQ(Parse(Svg(9, 9, "<rect display=\"none\" visibility=\"visible\" width=\"1\" height=\"1\"/>")).shapes.size(), 0u);
    EXPECT_EQ(Parse(Svg(9, 9, "<rect style=\"visibility:collapse\" width=\"1\" height=\"1\"/>")).shapes.size(), 0u);
}

TEST(SvgParser, PathZeroLengthAndContinuationAfterClose)
{
    Vector<VectorSubPath> dot = PathData("M10 10z");
    ASSERT_EQ(dot.size(), 1u);
    EXPECT_TRUE(dot[0].closed);
    EXPECT_TRUE(dot[0].segments.IsEmpty());
    ExpectPoint(dot[0].start, 10, 10);

    EXPECT_EQ(PathData("M10 10").size(), 0u);
    EXPECT_EQ(PathData("M10 10 M20 20 h0").size(), 1u);

    Vector<VectorSubPath> relative = PathData("M10 10 L20 10 L20 20 Z l5 5");
    ASSERT_EQ(relative.size(), 2u);
    ExpectPoint(relative[1].start, 10, 10);
    ExpectPoint(relative[1].segments[0].end, 15, 15);
    EXPECT_FALSE(relative[1].closed);

    Vector<VectorSubPath> absolute = PathData("M1 2 H5 Z H9 V7");
    ASSERT_EQ(absolute.size(), 2u);
    ExpectPoint(absolute[1].start, 1, 2);
    ExpectPoint(absolute[1].segments[1].end, 9, 7);

    PathData("M0 0 Z 5 5", false);
}

TEST(SvgParser, PathSmoothCommandsAfterOtherCurveTypes)
{
    Vector<VectorSubPath> cubicAfterQuad = PathData("M0 0 Q5 10 10 0 S20 10 30 0");
    ExpectPoint(cubicAfterQuad[0].segments[1].control1, 10, 0);

    Vector<VectorSubPath> quadAfterCubic = PathData("M0 0 C0 9 9 9 9 0 T18 0");
    ExpectPoint(quadAfterCubic[0].segments[1].control1, 9, 0);
    ExpectPoint(quadAfterCubic[0].segments[1].control2, 12, 0);

    Vector<VectorSubPath> chain = PathData("M0 0 Q5 10 10 0 T20 0");
    ExpectPoint(chain[0].segments[1].control1, 10.0f + 10.0f/3.0f, -20.0f/3.0f, 1e-3f);
}

TEST(SvgParser, NumbersOutOfRangeCutTheData)
{
    Vector<VectorSubPath> subPaths;
    EXPECT_FALSE(SvgParser::ParsePathData("M 1 2 L 3 4 L 1e400 5 L 7 8", subPaths));
    ASSERT_EQ(subPaths.size(), 1u);
    EXPECT_EQ(subPaths[0].segments.size(), 1u);

    VectorImage image = Parse(Svg(9, 9, "<rect x=\"1e999\" y=\"1\" width=\"2\" height=\"3\"/>"));
    ExpectPoint(image.shapes[0].subPaths[0].start, 0, 1);

    Vector<VectorSubPath> precise = PathData("M-.5e1,+12.75 L1.5E-2 .125");
    ExpectPoint(precise[0].start, -5.0f, 12.75f);
    ExpectPoint(precise[0].segments[0].end, 0.015f, 0.125f, 1e-6f);
}

TEST(SvgParser, RectRadiusVariants)
{
    auto firstCurveEnd = [](const char* attributes)
    {
        VectorImage image = Parse(Svg(40, 40, String("<rect width=\"20\" height=\"10\" ") + attributes + "/>"));
        return image.shapes[0].subPaths[0].segments[1].end;
    };

    ExpectPoint(firstCurveEnd("ry=\"3\""), 20, 3);
    ExpectPoint(firstCurveEnd("rx=\"auto\" ry=\"2\""), 20, 2);
    ExpectPoint(firstCurveEnd("rx=\"4\" ry=\"auto\""), 20, 4);
    ExpectPoint(firstCurveEnd("rx=\"50\" ry=\"50\""), 20, 5);
}

TEST(SvgParser, TransformVariants)
{
    Basis mirror;
    EXPECT_TRUE(SvgParser::ParseTransform("scale(-1, 1)", mirror));
    ExpectPoint(mirror.Transform(Vec2F(2, 3)), -2, 3);

    Basis list;
    EXPECT_TRUE(SvgParser::ParseTransform("translate(10,0), scale(2) ,rotate(90)", list));
    ExpectPoint(list.Transform(Vec2F(1, 0)), 10, 2);

    VectorImage image = Parse(Svg(40, 40, "<rect width=\"4\" height=\"2\" stroke=\"red\" transform=\"scale(-1,1) translate(-10 0)\"/>"));
    ExpectPoint(image.shapes[0].subPaths[0].start, 10, 0);
    ExpectPoint(image.shapes[0].subPaths[0].segments[0].end, 6, 0);
    EXPECT_FLOAT_EQ(image.shapes[0].strokeWidth, 1.0f);

    EXPECT_TRUE(HasWarning(Warnings(Svg(40, 40, "<rect width=\"4\" height=\"2\" stroke=\"red\" transform=\"scale(4,1)\"/>")),
                           "non-uniform"));
    EXPECT_TRUE(Warnings(Svg(40, 40, "<rect width=\"4\" height=\"2\" transform=\"scale(4,1)\"/>")).IsEmpty());
}

TEST(SvgParser, OpacityVariants)
{
    VectorImage image = Parse(Svg(9, 9, "<g fill-opacity=\"0.5\" opacity=\"50%\"><rect fill-opacity=\"25%\" width=\"1\" height=\"1\"/>"
                                        "<rect fill-opacity=\"inherit\" opacity=\"2\" width=\"1\" height=\"1\"/></g>"));
    EXPECT_FLOAT_EQ(image.shapes[0].fillOpacity, 0.25f);
    EXPECT_FLOAT_EQ(image.shapes[0].opacity, 0.5f);
    EXPECT_FLOAT_EQ(image.shapes[1].fillOpacity, 0.5f);
    EXPECT_FLOAT_EQ(image.shapes[1].opacity, 0.5f);
}

TEST(SvgParser, GradientStopsAndUnitsVariants)
{
    VectorImage image = Parse("<svg width=\"40\" height=\"20\">"
                              "<linearGradient id=\"a\" gradientUnits=\"userSpaceOnUse\" x1=\"25%\" y1=\"0\" x2=\"75%\" y2=\"100%\">"
                              "<stop offset=\"50%\" style=\"stop-color:#f00;stop-opacity:0.5\"/><stop offset=\"20%\" stop-color=\"#00f\"/>"
                              "<stop offset=\"150%\" stop-color=\"lime\"/></linearGradient>"
                              "<linearGradient id=\"b\" href=\"#c\"/><linearGradient id=\"c\" href=\"#b\"/>"
                              "<rect width=\"40\" height=\"20\" fill=\"url(#a)\"/><rect width=\"1\" height=\"1\" fill=\"url(#b)\"/></svg>");
    ASSERT_EQ(image.shapes.size(), 1u);

    const VectorPaint& paint = image.shapes[0].fill;
    ExpectPoint(paint.begin, 10, 0);
    ExpectPoint(paint.end, 30, 20);
    ASSERT_EQ(paint.stops.size(), 3u);
    EXPECT_FLOAT_EQ(paint.stops[0].offset, 0.5f);
    EXPECT_EQ(paint.stops[0].color, Color4(255, 0, 0, 128));
    EXPECT_FLOAT_EQ(paint.stops[1].offset, 0.5f);
    EXPECT_FLOAT_EQ(paint.stops[2].offset, 1.0f);
    EXPECT_EQ(paint.stops[2].color, Color4(0, 255, 0, 255));
}

TEST(SvgParser, ByteOrderMarkAndNonAsciiIds)
{
    VectorImage image = Parse("\xEF\xBB\xBF<svg width=\"9\" height=\"9\"><linearGradient id=\"\xD0\xB3\xD1\x80\xD0\xB0\xD0\xB4\">"
                              "<stop offset=\"0\" stop-color=\"#f00\"/><stop offset=\"1\" stop-color=\"#00f\"/></linearGradient>"
                              "<rect width=\"5\" height=\"5\" fill=\"url(#\xD0\xB3\xD1\x80\xD0\xB0\xD0\xB4)\"/></svg>");
    ASSERT_EQ(image.shapes.size(), 1u);
    EXPECT_EQ(image.shapes[0].fill.type, VectorPaintType::LinearGradient);
}

TEST(SvgParser, UseOfItselfIsCutWithoutMultiplying)
{
    String error;
    Vector<String> warnings;
    VectorImage image;
    ASSERT_TRUE(SvgParser::Parse(Svg(10, 10, "<g id=\"a\"><rect width=\"2\" height=\"2\"/><use href=\"#a\"/><use href=\"#a\"/>"
                                             "<use href=\"#a\"/></g>"), image, error, warnings));
    EXPECT_EQ(image.shapes.Count(), 4) << "the group and its three copies, the copies do not copy themselves";
    EXPECT_TRUE(HasWarning(warnings, "refers to itself"));
}

TEST(SvgParser, NotFiniteLengthsAreIgnored)
{
    VectorImage image = Parse(Svg(20, 20, "<path d=\"M2 10 L18 10\" stroke=\"#000000\" stroke-width=\"1e39\"/>"));
    ASSERT_EQ(image.shapes.Count(), 1);
    EXPECT_FLOAT_EQ(image.shapes[0].strokeWidth, 1.0f);
}
