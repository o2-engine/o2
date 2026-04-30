#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/IRectDrawable.h"
#include "o2/Render/Material.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Vector2.h"

using namespace o2;

namespace
{
    Ref<FunctionalRectDrawable> MakeRect(const Vec2F& size = Vec2F(100, 100),
                                         const Vec2F& position = Vec2F(0, 0),
                                         float angleRad = 0.0f,
                                         const Vec2F& pivot = Vec2F(0, 0))
    {
        return mmake<FunctionalRectDrawable>(
            [](const Basis&, const Color4&) {},
            size, position, angleRad, Vec2F(1.0f, 1.0f), Color4::White(), pivot);
    }
}

TEST(IRectDrawable, SetColorRoundTrip)
{
    auto d = MakeRect();
    d->SetColor(Color4(10, 20, 30, 200));
    EXPECT_EQ(d->GetColor(), Color4(10, 20, 30, 200));
}

TEST(IRectDrawable, SetOverrideColorRoundTrip)
{
    auto d = MakeRect();
    d->SetOverrideColor(Color4(50, 60, 70, 100));
    EXPECT_EQ(d->GetOverrideColor(), Color4(50, 60, 70, 100));
}

TEST(IRectDrawable, SetTransparencyAffectsAlphaWith8BitTruncation)
{
    auto d = MakeRect();
    d->SetColor(Color4(255, 255, 255, 255));

    d->SetTransparency(0.5f);
    EXPECT_EQ(d->GetColor().a, 127);
    EXPECT_NEAR(d->GetTransparency(), 0.498f, 0.005f);

    d->SetTransparency(0.0f);
    EXPECT_EQ(d->GetColor().a, 0);
    EXPECT_FLOAT_EQ(d->GetTransparency(), 0.0f);

    d->SetTransparency(1.0f);
    EXPECT_EQ(d->GetColor().a, 255);
    EXPECT_FLOAT_EQ(d->GetTransparency(), 1.0f);
}

TEST(IRectDrawable, SetEnabledRoundTrip)
{
    auto d = MakeRect();
    EXPECT_TRUE(d->IsEnabled());
    d->SetEnabled(false);
    EXPECT_FALSE(d->IsEnabled());
    d->SetEnabled(true);
    EXPECT_TRUE(d->IsEnabled());
}

TEST(IRectDrawable, EqualityRequiresColorEnabledAndTransform)
{
    auto a = MakeRect(Vec2F(100, 100), Vec2F(5, 6));
    auto b = MakeRect(Vec2F(100, 100), Vec2F(5, 6));
    EXPECT_TRUE(*a == *b);

    b->SetColor(Color4(0, 255, 0, 255));
    EXPECT_FALSE(*a == *b);

    auto c = MakeRect(Vec2F(100, 100), Vec2F(5, 6));
    c->SetEnabled(false);
    EXPECT_FALSE(*a == *c);

    auto d = MakeRect(Vec2F(100, 100), Vec2F(5, 6));
    d->SetPosition(Vec2F(99, 99));
    EXPECT_FALSE(*a == *d);
}

TEST(IRectDrawable, CopyConstructorPreservesColorEnabledAndTransform)
{
    auto src = MakeRect(Vec2F(80, 40), Vec2F(7, 8), 0.0f, Vec2F(0, 0));
    src->SetColor(Color4(11, 22, 33, 200));
    src->SetEnabled(false);

    FunctionalRectDrawable copy(*src);

    EXPECT_EQ(copy.GetColor(), Color4(11, 22, 33, 200));
    EXPECT_FALSE(copy.IsEnabled());
    EXPECT_EQ(copy.GetSize(), Vec2F(80, 40));
    EXPECT_EQ(copy.GetPosition(), Vec2F(7, 8));
}

TEST(IRectDrawable, IsUnderPointReturnsFalseBeforeFirstDraw)
{
    auto d = MakeRect(Vec2F(100, 100), Vec2F(0, 0));
    EXPECT_FALSE(d->IsUnderPoint(Vec2F(50, 50)));
}

TEST(IRectDrawable, IsUnderPointAfterDrawAxisAlignedRect)
{
    auto d = MakeRect(Vec2F(100, 100), Vec2F(0, 0));
    d->Draw();

    EXPECT_TRUE(d->IsUnderPoint(Vec2F(50, 50)));
    EXPECT_FALSE(d->IsUnderPoint(Vec2F(150, 150)));
    EXPECT_FALSE(d->IsUnderPoint(Vec2F(-10, 50)));
}

TEST(IRectDrawable, IsUnderPointAfterDrawRotatedRect)
{
    auto d = MakeRect(Vec2F(100, 100), Vec2F(0, 0), Math::Deg2rad(45.0f));
    d->Draw();

    EXPECT_TRUE(d->IsUnderPoint(Vec2F(50, 50)));
    EXPECT_FALSE(d->IsUnderPoint(Vec2F(90, 5)));
}

TEST(IRectDrawable, SetMaterialClearsMaterialAsset)
{
    auto d = MakeRect();
    auto mat = mmake<Material>();
    d->SetMaterial(mat);
    EXPECT_EQ(d->GetMaterialAsset(), nullptr);
    EXPECT_EQ(d->GetMaterial().Get(), mat.Get());
}
