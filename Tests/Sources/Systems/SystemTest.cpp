#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/VKCodes.h"
#include "o2/Utils/System/CommandLineOptions.h"
#include "o2/Utils/System/ShortcutKeys.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2/Utils/System/Time/TimeStamp.h"
#include "o2/Utils/System/Time/Timer.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== TimeStamp =====

TEST(TimeStamp, DefaultIsZero)
{
    TimeStamp t;
    EXPECT_EQ(t.mSecond, 0);
    EXPECT_EQ(t.mMinute, 0);
    EXPECT_EQ(t.mHour, 0);
    EXPECT_EQ(t.mDay, 0);
    EXPECT_EQ(t.mMonth, 0);
    EXPECT_EQ(t.mYear, 0);
}

TEST(TimeStamp, ConstructorAssignsFieldsInOrderSecondsFirst)
{
    TimeStamp t(15, 30, 12, 5, 6, 2026);

    EXPECT_EQ(t.mSecond, 15);
    EXPECT_EQ(t.mMinute, 30);
    EXPECT_EQ(t.mHour, 12);
    EXPECT_EQ(t.mDay, 5);
    EXPECT_EQ(t.mMonth, 6);
    EXPECT_EQ(t.mYear, 2026);
}

TEST(TimeStamp, EqualityComparesAllFields)
{
    TimeStamp a(1, 2, 3, 4, 5, 6);
    TimeStamp b(1, 2, 3, 4, 5, 6);
    TimeStamp c(0, 2, 3, 4, 5, 6);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}

// ===== Timer =====

TEST(Timer, NewTimerStartsNearZero)
{
    Timer t;
    EXPECT_LT(t.GetTime(), 1.0f);
}

TEST(Timer, ResetReturnsTimeToZero)
{
    Timer t;
    t.Reset();
    EXPECT_LT(t.GetTime(), 0.5f);
}

TEST(Timer, GetDeltaTimeIsMonotonicNonNegative)
{
    Timer t;
    float a = t.GetDeltaTime();
    float b = t.GetDeltaTime();
    EXPECT_GE(a, 0.0f);
    EXPECT_GE(b, 0.0f);
}

// ===== o2Time =====

TEST(Time, SingletonReachable)
{
    EXPECT_TRUE(Time::IsSingletonInitialzed());
}

TEST(Time, SetLocalTimeAndResetLocalTime)
{
    TimeGuard guard;
    o2Time.SetLocalTime(123.5f);
    EXPECT_FLOAT_EQ(o2Time.GetLocalTime(), 123.5f);

    o2Time.ResetLocalTime();
    EXPECT_FLOAT_EQ(o2Time.GetLocalTime(), 0.0f);
}

TEST(Time, ApplicationTimeIsNonNegative)
{
    EXPECT_GE(o2Time.GetApplicationTime(), 0.0f);
}

#if defined PLATFORM_WINDOWS
TEST(Time, CurrentTimeYearIsRecent)
{
    TimeStamp ts = o2Time.CurrentTime();
    EXPECT_GE(ts.mYear, 2024);
    EXPECT_GE(ts.mMonth, 1);
    EXPECT_LE(ts.mMonth, 12);
    EXPECT_GE(ts.mDay, 1);
    EXPECT_LE(ts.mDay, 31);
}
#endif

// ===== ShortcutKeys =====

TEST(ShortcutKeys, DefaultIsEmpty)
{
    ShortcutKeys sk;
    EXPECT_TRUE(sk.IsEmpty());
    EXPECT_TRUE(sk.AsString().IsEmpty());
}

TEST(ShortcutKeys, EmptyShortcutNotPressedNotDown)
{
    ShortcutKeys sk;
    EXPECT_FALSE(sk.IsPressed());
    EXPECT_FALSE(sk.IsDown());
}

TEST(ShortcutKeys, ConstructorStoresKeys)
{
    Vector<KeyboardKey> keys = { (KeyboardKey)VK_CONTROL, (KeyboardKey)'C' };
    ShortcutKeys sk(keys);

    EXPECT_FALSE(sk.IsEmpty());
    EXPECT_EQ(sk.keys.Count(), 2);
}

TEST(ShortcutKeys, AsStringRendersCtrlPlusLetter)
{
    ShortcutKeys sk({ (KeyboardKey)VK_CONTROL, (KeyboardKey)'C' });
    EXPECT_EQ(sk.AsString(), "Ctrl+C");
}

TEST(ShortcutKeys, AsStringRendersShiftPlusLetter)
{
    ShortcutKeys sk({ (KeyboardKey)VK_SHIFT, (KeyboardKey)'A' });
    EXPECT_EQ(sk.AsString(), "Shift+A");
}

TEST(ShortcutKeys, AsStringUppercasesLowerLetter)
{
    ShortcutKeys sk({ (KeyboardKey)VK_CONTROL, (KeyboardKey)'a' });
    EXPECT_EQ(sk.AsString(), "Ctrl+A");
}

TEST(ShortcutKeys, AsStringEmptyForEmpty)
{
    ShortcutKeys sk;
    EXPECT_EQ(sk.AsString(), "");
}

TEST(ShortcutKeys, FromCustomStringPreservesCustomString)
{
    auto sk = ShortcutKeys::FromCustomString("Ctrl+Shift+P");
    EXPECT_EQ(sk.AsString(), "Ctrl+Shift+P");
    EXPECT_TRUE(sk.IsEmpty());
}

TEST(ShortcutKeys, EqualityIgnoresKeyOrder)
{
    ShortcutKeys a({ (KeyboardKey)VK_CONTROL, (KeyboardKey)'C' });
    ShortcutKeys b({ (KeyboardKey)'C', (KeyboardKey)VK_CONTROL });

    EXPECT_TRUE(a == b);
}

TEST(ShortcutKeys, EqualityRejectsDifferentSizes)
{
    ShortcutKeys a({ (KeyboardKey)VK_CONTROL, (KeyboardKey)'C' });
    ShortcutKeys b({ (KeyboardKey)VK_CONTROL });

    EXPECT_FALSE(a == b);
}

TEST(ShortcutKeys, EqualityRejectsDifferentKeys)
{
    ShortcutKeys a({ (KeyboardKey)VK_CONTROL, (KeyboardKey)'C' });
    ShortcutKeys b({ (KeyboardKey)VK_CONTROL, (KeyboardKey)'V' });

    EXPECT_FALSE(a == b);
}

TEST(ShortcutKeys, LessThanOrdersByCountFirst)
{
    ShortcutKeys one({ (KeyboardKey)'A' });
    ShortcutKeys two({ (KeyboardKey)'A', (KeyboardKey)'B' });

    EXPECT_TRUE(one < two);
    EXPECT_FALSE(two < one);
}

// ===== CommandLineOptions =====

namespace
{
    Map<String, String> ParseArgsHelper(std::initializer_list<const char*> args)
    {
        std::vector<char*> argv;
        std::vector<std::string> store;
        for (const char* a : args)
            store.emplace_back(a);
        for (auto& s : store)
            argv.push_back(s.data());

        return CommandLineOptions::Parse((int)argv.size(), argv.data());
    }
}

TEST(CommandLineOptions, ParsePairsKeyValues)
{
    Map<String, String> result = ParseArgsHelper({ "exe", "-name", "alice", "-count", "42" });

    EXPECT_EQ(result.Count(), 2);
    EXPECT_EQ(result["-name"], "alice");
    EXPECT_EQ(result["-count"], "42");
}

TEST(CommandLineOptions, ParseEmptyArgsReturnsEmptyMap)
{
    Map<String, String> result = ParseArgsHelper({ "exe" });
    EXPECT_EQ(result.Count(), 0);
}

TEST(CommandLineOptions, ParseDuplicateKeyKeepsLastValue)
{
    Map<String, String> result = ParseArgsHelper({ "exe", "-mode", "a", "-mode", "b" });

    EXPECT_EQ(result.Count(), 1);
    EXPECT_EQ(result["-mode"], "b");
}

