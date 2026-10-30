#include "o2/stdafx.h"

#ifdef PLATFORM_MAC

#include <gtest/gtest.h>
#include <ApplicationServices/ApplicationServices.h>

#include "o2/Utils/System/Clipboard.h"

using namespace o2;

// Text comes to the clipboard in the flavour its app writes: plain text in UTF-8 or UTF-16 only, or a link
namespace
{
    // Puts one item with one flavour onto the system clipboard
    void PutFlavor(CFStringRef flavor, const void* bytes, size_t length)
    {
        PasteboardRef pasteboard = nullptr;
        ASSERT_EQ(PasteboardCreate(kPasteboardClipboard, &pasteboard), noErr);
        PasteboardClear(pasteboard);
        CFDataRef data = CFDataCreate(kCFAllocatorDefault, (const UInt8*)bytes, (CFIndex)length);
        PasteboardPutItemFlavor(pasteboard, (PasteboardItemID)1, flavor, data, 0);
        CFRelease(data);
        CFRelease(pasteboard);
    }

    // Keeps the text the user had on the clipboard
    struct ClipboardMac : ::testing::Test
    {
        WString saved;

        void SetUp() override { saved = Clipboard::GetText(); }
        void TearDown() override { Clipboard::SetText(saved); }
    };
}

TEST_F(ClipboardMac, ReadsUtf8Text)
{
    Clipboard::SetText("https://utf8.example/path");
    EXPECT_EQ((String)Clipboard::GetText(), "https://utf8.example/path");
}

TEST_F(ClipboardMac, ReadsUtf16OnlyText)
{
    const char16_t text[] = u"https://utf16.example/путь";
    PutFlavor(CFSTR("public.utf16-plain-text"), text, sizeof(text) - sizeof(char16_t));
    EXPECT_EQ((String)Clipboard::GetText(), "https://utf16.example/путь");
}

TEST_F(ClipboardMac, ReadsALink)
{
    const char url[] = "https://link.example/page?x=1";
    PutFlavor(CFSTR("public.url"), url, sizeof(url) - 1);
    EXPECT_EQ((String)Clipboard::GetText(), "https://link.example/page?x=1");
}

#endif // PLATFORM_MAC
