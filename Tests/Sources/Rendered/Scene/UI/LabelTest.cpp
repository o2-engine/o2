#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/Label.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(Label, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto l = mmake<Label>();
    ASSERT_TRUE(l);
    EXPECT_TRUE(l->GetText().IsEmpty());
}

TEST(Label, CopyPreservesText)
{
    SceneCleanGuard guard;
    auto src = mmake<Label>();
    src->SetText("hello");
    auto copy = src->CloneAsRef<Label>();
    EXPECT_EQ(copy->GetText(), WString("hello"));
}

// ===== Text =====

TEST(Label, SetTextRoundTrip)
{
    SceneCleanGuard guard;
    auto l = mmake<Label>();
    l->SetText("hello world");
    EXPECT_EQ(l->GetText(), WString("hello world"));
}

// ===== Color =====

TEST(Label, SetColorRoundTrip)
{
    SceneCleanGuard guard;
    auto l = mmake<Label>();
    l->SetColor(Color4(10, 20, 30, 40));
    EXPECT_EQ(l->GetColor(), Color4(10, 20, 30, 40));
}

// ===== Alignment =====

TEST(Label, SetHorAlignRoundTrip)
{
    SceneCleanGuard guard;
    auto l = mmake<Label>();
    l->SetHorAlign(HorAlign::Right);
    EXPECT_EQ(l->GetHorAlign(), HorAlign::Right);
    l->SetHorAlign(HorAlign::Middle);
    EXPECT_EQ(l->GetHorAlign(), HorAlign::Middle);
}

TEST(Label, SetVerAlignRoundTrip)
{
    SceneCleanGuard guard;
    auto l = mmake<Label>();
    l->SetVerAlign(VerAlign::Top);
    EXPECT_EQ(l->GetVerAlign(), VerAlign::Top);
    l->SetVerAlign(VerAlign::Middle);
    EXPECT_EQ(l->GetVerAlign(), VerAlign::Middle);
}

// ===== Overflow =====

TEST(Label, SetHorOverflowRoundTrip)
{
    SceneCleanGuard guard;
    auto l = mmake<Label>();
    l->SetHorOverflow(Label::HorOverflow::Wrap);
    EXPECT_EQ(l->GetHorOverflow(), Label::HorOverflow::Wrap);
    l->SetHorOverflow(Label::HorOverflow::Cut);
    EXPECT_EQ(l->GetHorOverflow(), Label::HorOverflow::Cut);
}

TEST(Label, SetVerOverflowRoundTrip)
{
    SceneCleanGuard guard;
    auto l = mmake<Label>();
    l->SetVerOverflow(Label::VerOverflow::Cut);
    EXPECT_EQ(l->GetVerOverflow(), Label::VerOverflow::Cut);
    l->SetVerOverflow(Label::VerOverflow::Expand);
    EXPECT_EQ(l->GetVerOverflow(), Label::VerOverflow::Expand);
}

// ===== Distance coefs =====

TEST(Label, SetSymbolsDistanceCoefRoundTrip)
{
    SceneCleanGuard guard;
    auto l = mmake<Label>();
    l->SetSymbolsDistanceCoef(2.0f);
    EXPECT_FLOAT_EQ(l->GetSymbolsDistanceCoef(), 2.0f);
}

TEST(Label, SetLinesDistanceCoefRoundTrip)
{
    SceneCleanGuard guard;
    auto l = mmake<Label>();
    l->SetLinesDistanceCoef(1.5f);
    EXPECT_FLOAT_EQ(l->GetLinesDistanceCoef(), 1.5f);
}

// ===== Expand border =====

TEST(Label, SetExpandBorderRoundTrip)
{
    SceneCleanGuard guard;
    auto l = mmake<Label>();
    l->SetExpandBorder(Vec2F(5, 7));
    EXPECT_EQ(l->GetExpandBorder(), Vec2F(5, 7));
}

// ===== Height =====

TEST(Label, SetHeightRoundTrip)
{
    SceneCleanGuard guard;
    auto l = mmake<Label>();
    l->SetHeight(24);
    EXPECT_EQ(l->GetHeight(), 24);
}
