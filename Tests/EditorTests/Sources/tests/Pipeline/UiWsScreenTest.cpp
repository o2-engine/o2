#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "UiDocTestKit.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsRunActs.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsScreen.h"

// The AI UI layout's screen without a window (port of RefCompare.tsx's settings, uiLayoutState.ts's units and preview words,
// DevicePicker.tsx's rows and StylePicker.tsx's samples): the numbers are the web's own

namespace
{
    using namespace UiDocTest;
    namespace Ws = Editor::UiDoc::Ws;

    const char* const older =
        "screen Menu 1080x1920\n"
        "  button play anchor=bottom-center pos=0,60 size=400x120 \"PLAY\"\n";

    String PatchText(const Vector<Pair<String, Json>>& patch)
    {
        Json o = Json::Object();
        for (auto& kv : patch)
            o[kv.first] = kv.second;
        return o.Dump();
    }
}

TEST(UiWsScreen, ReferenceCompareSettings)
{
    Json config = Json::Object();
    auto s = Ws::RefSettingOf(config, true);
    EXPECT_FALSE(s.on);
    EXPECT_STR_EQ(s.mode, "alpha");
    EXPECT_EQ(s.alpha, 0.0);
    EXPECT_EQ(s.split, 0.5);

    // A pipeline saved before uiRefOn had the opacity alone: above zero = on; never on without a reference
    config["uiRefOverlay"] = 0.4;
    EXPECT_TRUE(Ws::RefSettingOf(config, true).on);
    EXPECT_FALSE(Ws::RefSettingOf(config, false).on);
    config["uiRefOn"] = false;
    config["uiRefMode"] = "diff";
    config["uiRefSplit"] = 1.7;
    s = Ws::RefSettingOf(config, true);
    EXPECT_FALSE(s.on);
    EXPECT_STR_EQ(s.mode, "diff");
    EXPECT_EQ(s.split, 1.0);
    config["uiRefMode"] = "blend";
    EXPECT_STR_EQ(Ws::RefSettingOf(config, true).mode, "alpha");

    // The stage's switch: on with an opacity one can see through, off
    Ws::UiRefSetting off;
    EXPECT_STR_EQ(PatchText(Ws::RefTogglePatch(off)), "{\"uiRefOn\":true,\"uiRefOverlay\":0.5}");
    off.alpha = 0.3;
    EXPECT_STR_EQ(PatchText(Ws::RefTogglePatch(off)), "{\"uiRefOn\":true}");
    Ws::UiRefSetting on;
    on.on = true;
    EXPECT_STR_EQ(PatchText(Ws::RefTogglePatch(on)), "{\"uiRefOn\":false}");

    // The card's menu: off is a choice, a mode switches the comparison on; the stage's segments only pick the mode
    EXPECT_STR_EQ(PatchText(Ws::RefModePatch(off, "off", true)), "{\"uiRefOn\":false}");
    EXPECT_STR_EQ(PatchText(Ws::RefModePatch(Ws::UiRefSetting(), "split", true)), "{\"uiRefMode\":\"split\",\"uiRefOn\":true,\"uiRefOverlay\":0.5}");
    EXPECT_STR_EQ(PatchText(Ws::RefModePatch(on, "diff", false)), "{\"uiRefMode\":\"diff\"}");

    // The amount: the divider's place in split, else the opacity in steps of 0.05 (and on)
    Ws::UiRefSetting split;
    split.mode = "split";
    EXPECT_STR_EQ(PatchText(Ws::RefAmountPatch(split, 0.333)), "{\"uiRefSplit\":0.33}");
    EXPECT_STR_EQ(PatchText(Ws::RefAmountPatch(on, 0.62)), "{\"uiRefOverlay\":0.6,\"uiRefOn\":true}");
    EXPECT_STR_EQ(PatchText(Ws::RefAmountPatch(on, 1.4)), "{\"uiRefOverlay\":1,\"uiRefOn\":true}");
}

TEST(UiWsScreen, OlderUnitsAndTheFirstScreenOp)
{
    UiTarget target = DefaultTarget();
    UiDoc doc = Parse(older);
    EXPECT_TRUE(Ws::InOlderUnits(doc, target));
    EXPECT_STR_EQ(Ws::OlderShort(doc), "Older units \xC2\xB7 1080\xC3\x97" "1920 at 480 dpi");
    EXPECT_STR_EQ(Ws::BringLabel(target), "Units \xE2\x86\x92 iPhone 16");

    // One op brings it over: the design taken to be the device's screen, every pixel value grown by the fit
    double scale = 0;
    auto op = Ws::BringOp(doc, target, &scale);
    ASSERT_TRUE(op.has_value());
    EXPECT_STR_EQ(op->Dump(), "{\"op\":\"screen\",\"width\":1179,\"height\":2096,\"dpi\":460,\"scale\":1.09167,\"fromDpi\":480}");
    EXPECT_NEAR(scale, 1.09167, 1e-9);
    UiDoc brought = ApplyOps(doc, { *op }).doc;
    EXPECT_FALSE(Ws::InOlderUnits(brought, target));
    EXPECT_FALSE(Ws::BringOp(brought, target).has_value());
    EXPECT_EQ(brought.width, 1179.0);
    EXPECT_NEAR(*brought.root.children[0].w, 400*1.09167, 0.5);

    // An empty layout is no concern: its first hand edit puts it into the device's pixels first
    UiDoc empty = EmptyDoc("Menu", 1080, 1920);
    EXPECT_FALSE(Ws::InOlderUnits(empty, target));
    auto first = Ws::FirstScreenOp(empty, target, nullptr);
    ASSERT_TRUE(first.has_value());
    EXPECT_STR_EQ(first->Dump(), "{\"op\":\"screen\",\"width\":1179,\"height\":2556,\"dpi\":460}");
    UiScreen reference = Ws::ReferenceScreenOf(empty, target, 768, 1376, std::nullopt);
    EXPECT_EQ(reference.w, 1179.0);
    EXPECT_EQ(reference.h, 2112.0);
    EXPECT_EQ(reference.dpi, 460.0);
    EXPECT_STR_EQ(Ws::FirstScreenOp(empty, target, &reference)->Dump(), "{\"op\":\"screen\",\"width\":1179,\"height\":2112,\"dpi\":460}");
    EXPECT_FALSE(Ws::FirstScreenOp(doc, target, nullptr).has_value()) << "a layout with blocks";
    EXPECT_FALSE(Ws::FirstScreenOp(EmptyDoc("Menu", 1179, 2556, 460.0), target, nullptr).has_value()) << "in device pixels already";

    // A layout still in older units keeps the picture's own pixels and density
    UiScreen own = Ws::ReferenceScreenOf(doc, target, 768, 1376, std::nullopt);
    EXPECT_EQ(own.w, 768.0);
    EXPECT_EQ(own.dpi, UI_BASE_DPI);
}

TEST(UiWsScreen, ThePickerNamesScreensByAspectAndCanvas)
{
    UiTarget target = DefaultTarget();
    UiDoc doc = ApplyOps(Parse(older), { *Ws::BringOp(Parse(older), target) }).doc;

    // The field: the kit's device, the canvas it gives and what it adds
    auto p = Ws::PreviewOf(doc, Ws::PreviewSettingOf(Json::Object()), nullptr, target);
    EXPECT_STR_EQ(Ws::PreviewName(p) + " \xC2\xB7 " + Ws::SizeText(p.frame.w, p.frame.h) + " \xC2\xB7 " + Ws::AddedText(doc, p),
                  "iPhone 16 \xC2\xB7 1179\xC3\x97" "2556 \xC2\xB7 +460 px height");
    EXPECT_STR_EQ(Ws::PreviewIconKind(p, target), "phone");

    // A row: the device, its aspect ratio and the canvas in the layout's pixels
    const UiDevice* ipad = DeviceById("ipad-pro-13");
    ASSERT_TRUE(ipad);
    EXPECT_STR_EQ(ipad->name + " " + AspectName(ipad->w, ipad->h) + " \xC2\xB7 " + Ws::CanvasText(doc, ipad->w, ipad->h),
                  "iPad Pro 13\xE2\x80\xB3 3:4 \xC2\xB7 1572\xC3\x97" "2096");
    Json config = Json::Object();
    config["uiDevice"] = "ipad-pro-13";
    p = Ws::PreviewOf(doc, Ws::PreviewSettingOf(config), nullptr, target);
    EXPECT_STR_EQ(Ws::AddedText(doc, p), "+393 px width");
    config["uiDevice"] = "design";
    EXPECT_STR_EQ(Ws::AddedText(doc, Ws::PreviewOf(doc, Ws::PreviewSettingOf(config), nullptr, target)), "");

    UiTarget wide{ "", "Monitor", 2560, 1440, 109 };
    EXPECT_STR_EQ(Ws::TargetKind(wide), "desktop");
}

TEST(UiWsScreen, StylePreviewsSayTheBlocksWords)
{
    UiDoc doc = Parse("screen S 1080x1920\n"
                      "  button play size=400x120 style=fancy \"  Play   now \"\n"
                      "  text long size=900x100 \"A text far longer than a row can read\"\n");
    const UiNode& play = doc.root.children[0];
    const UiNode& longText = doc.root.children[1];
    TextStyle button = TextStyleDefault();
    button.name = "btn";
    button.role = "button";
    EXPECT_STR_EQ(Ws::StyleSampleText(play, button), "Play now");
    EXPECT_STR_EQ(Ws::StyleSampleText(longText, button), "Play");
    TextStyle title = TextStyleDefault();
    title.role = "title";
    EXPECT_STR_EQ(Ws::DefaultStyleSample(title), "Forest Quest");

    // "auto": the kit's style for the block's role, its own style= left out
    UiKit kit;
    TextStyle body = TextStyleDefault();
    body.name = "plain";
    TextStyle fancy = TextStyleDefault();
    fancy.name = "fancy";
    fancy.role = "title";
    kit.textStyles = { body, button, fancy };
    EXPECT_STR_EQ(StyleOf(play, &kit, &play).name, "fancy");
    EXPECT_STR_EQ(Ws::AutoStyleOf(play, &kit).name, "btn");
    EXPECT_STR_EQ(Ws::AutoStyleOf(longText, &kit).name, "plain");

    // The preview's screen is just large enough for the words in the style, its effects included
    UiDoc tight = Ws::TightStyleDoc(button, "Play");
    EXPECT_EQ(tight.height, 64.0);
    EXPECT_STR_EQ(*tight.root.color, "#20232B");
    ASSERT_EQ(tight.root.children.size(), 1u);
    EXPECT_STR_EQ(NodeToJson(tight.root.children[0]).Dump(),
                  "{\"id\":\"sample\",\"type\":\"text\",\"anchor\":\"stretch\",\"margin\":[3,6,3,6],\"style\":\"btn\",\"text\":\"Play\",\"textAlign\":\"center\"}");
    TextStyle shadowed = button;
    shadowed.size = 80;
    Json fx = Json::Array();
    Json shadow = Json::Object();
    shadow["type"] = "shadow";
    shadow["x"] = 0;
    shadow["y"] = 8;
    shadow["blur"] = 10;
    fx.Add(shadow);
    shadowed = MergeTextStyle(shadowed, [&] { Json o = Json::Object(); o["effects"] = fx; return o; }());
    UiDoc big = Ws::TightStyleDoc(shadowed, "Play");
    TextReach reach = EffectsReach(shadowed);
    EXPECT_GT(reach.bottom, 0.0);
    EXPECT_EQ(big.height, std::ceil(80*1.2 + reach.top + reach.bottom + 16));
}

TEST(UiWsScreen, AnActWithoutWordsIsCalledByItsTitle)
{
    Json acts;
    Json::Parse(R"([{"id":"cut-1","kind":"cut","title":"Cut 3 graphics","status":"ok"},{"id":"x","kind":"draft","title":"Draft"}])", acts);
    EXPECT_STR_EQ(Editor::UiWsRunActs::ActTitle(*acts.At(0)), "Cut 3 graphics");
    EXPECT_STR_EQ(Editor::UiWsRunActs::ActTitle(*acts.At(1)), "First layout");
}
