#include "o2Editor/stdafx.h"

#include "UiDocTestKit.h"

#include "UiDocGolden.inc"

namespace UiDocTest
{
    const Json& Golden()
    {
        static const Json golden = []() {
            std::string text;
            for (const char* part : kUiDocGoldenParts)
                text += part;
            Json out;
            String error;
            if (!Json::Parse(String(text), out, &error))
                ADD_FAILURE() << "the golden data does not parse: " << error.Data();
            return out;
        }();
        return golden;
    }
}

TEST(UiDocGolden, ParsesAndHasEverySection)
{
    using namespace UiDocTest;
    for (const char* key : { "uiml", "tokens", "anchors", "cleanProp", "nodes", "docs", "edits", "rawOps", "solve", "grid", "placeAt", "slots",
                             "steps", "kit", "styles", "widths", "agent", "answers", "structure", "prep", "device", "misc" })
        EXPECT_TRUE(Golden().Has(key)) << key;
}
