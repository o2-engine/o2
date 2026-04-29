#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/EditBox.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(EditBox, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    ASSERT_TRUE(eb);
    EXPECT_TRUE(eb->GetText().IsEmpty());
    EXPECT_TRUE(eb->IsFocusable());
}

TEST(EditBox, CopyPreservesText)
{
    SceneCleanGuard guard;
    auto src = mmake<EditBox>();
    src->SetText("hello");
    auto copy = src->CloneAsRef<EditBox>();
    EXPECT_EQ(copy->GetText(), WString("hello"));
}

// ===== Text =====

TEST(EditBox, SetTextRoundTrip)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetText("hello world");
    EXPECT_EQ(eb->GetText(), WString("hello world"));
}

// ===== Caret =====

TEST(EditBox, SetCaretPositionRoundTrip)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetText("12345");
    eb->SetCaretPosition(3);
    EXPECT_EQ(eb->GetCaretPosition(), 3);
}

// ===== Selection =====

// Selection API requires text drawable initialized via UI style; not exercised here.

// ===== Selection color =====

TEST(EditBox, SetSelectionColorRoundTrip)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetSelectionColor(Color4(10, 20, 30, 40));
    EXPECT_EQ(eb->GetSelectionColor(), Color4(10, 20, 30, 40));
}

// ===== Filters / available symbols =====

TEST(EditBox, SetFilterIntegerSetsAvailableSymbols)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetFilterInteger();
    EXPECT_FALSE(eb->GetAvailableSymbols().IsEmpty());
}

TEST(EditBox, SetAvailableSymbolsRoundTrip)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetAvailableSymbols("abc");
    EXPECT_EQ(eb->GetAvailableSymbols(), WString("abc"));
}

// ===== Multi-line =====

TEST(EditBox, SetMultiLineRoundTrip)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetMultiLine(false);
    EXPECT_FALSE(eb->IsMultiLine());
    eb->SetMultiLine(true);
    EXPECT_TRUE(eb->IsMultiLine());
}

TEST(EditBox, SetWordWrapRoundTrip)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetWordWrap(true);
    EXPECT_TRUE(eb->IsWordWrap());
    eb->SetWordWrap(false);
    EXPECT_FALSE(eb->IsWordWrap());
}

TEST(EditBox, SetMaxLineCharactersCountRoundTrip)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetMaxLineCharactersCount(10);
    EXPECT_EQ(eb->GetMaxLineCharactersCount(), 10);
}

TEST(EditBox, SetMaxLinesCountRoundTrip)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetMaxLinesCount(3);
    EXPECT_EQ(eb->GetMaxLinesCount(), 3);
}

// ===== Caret blinking =====

TEST(EditBox, SetCaretBlinkingDelayRoundTrip)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetCaretBlinkingDelay(0.5f);
    EXPECT_FLOAT_EQ(eb->GetCaretBlinkingDelay(), 0.5f);
}

// ===== Events =====

TEST(EditBox, OnChangedFiresOnSetText)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    int count = 0;
    eb->onChanged = [&](const WString&) { count++; };
    eb->SetText("a");
    eb->SetText("ab");
    EXPECT_EQ(count, 2);
}

// SetText calls onChanged unconditionally — even when text is unchanged.
// Lock that contract since callers may rely on it for pseudo-rebroadcast.
TEST(EditBox, OnChangedFiresEvenWhenTextDoesNotActuallyChange)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetText("hello");
    int count = 0;
    eb->onChanged = [&](const WString&) { count++; };
    eb->SetText("hello");
    EXPECT_EQ(count, 1);
}

// ===== Filtering =====

TEST(EditBox, AvailableSymbolsFilterStripsForbiddenCharsOnSetText)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetAvailableSymbols("0123456789");
    eb->SetText("a1b2c3");
    EXPECT_EQ(eb->GetText(), WString("123"));
}

TEST(EditBox, SetAvailableSymbolsFiltersExistingText)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetText("a1b2c3");
    eb->SetAvailableSymbols("0123456789");
    EXPECT_EQ(eb->GetText(), WString("123"));
}

// ===== Multi-line =====

TEST(EditBox, SetMultiLineFalseAfterTextStripsNewlines)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetMultiLine(true);
    eb->SetText("a\nb\nc");
    // Existing newlines must be filtered when switching to single-line mode.
    eb->SetMultiLine(false);
    bool hasNewline = false;
    for (int i = 0; i < eb->GetText().Length(); i++)
        if (eb->GetText()[i] == L'\n') hasNewline = true;
    EXPECT_FALSE(hasNewline);
}

// ===== Max characters =====

TEST(EditBox, SetMaxLineCharactersCountShrinksLine)
{
    SceneCleanGuard guard;
    auto eb = mmake<EditBox>();
    eb->SetText("abcdef");
    int beforeLen = eb->GetText().Length();
    eb->SetMaxLineCharactersCount(3);
    EXPECT_LT(eb->GetText().Length(), beforeLen);
}
