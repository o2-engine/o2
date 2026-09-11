#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Types/String.h"

using namespace o2;

TEST(String, ReplaceAllMovesPastEveryReplacement)
{
    EXPECT_EQ(String("a/b/c").ReplacedAll("/", " / "), String("a / b / c"));
    EXPECT_EQ(String("aaa").ReplacedAll("a", "aa"), String("aaaaaa"));
    EXPECT_EQ(String("x-y-").ReplacedAll("-", ""), String("xy"));
    EXPECT_EQ(String("abc").ReplacedAll("", "-"), String("abc"));
    EXPECT_EQ(String("no match").ReplacedAll("/", " / "), String("no match"));
}

TEST(String, PopBackRemovesAndReturnsTheLastSymbol)
{
    WString text = "cd\n";
    EXPECT_EQ(text.PopBack(), L'\n');
    EXPECT_EQ(text, WString("cd"));
    EXPECT_EQ(text.Length(), 2);
}
