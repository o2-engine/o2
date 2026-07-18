#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Types/FontStyleAsset.h"
#include "o2/Render/FontStyle.h"
#include "o2/Render/VectorFontEffects.h"
#include "o2/Utils/Serialization/DataValue.h"

using namespace o2;

TEST(FontStyle, EffectsAddRemove)
{
    auto style = mmake<FontStyle>();
    EXPECT_TRUE(style->GetEffects().IsEmpty());

    auto stroke = style->AddEffect<FontStrokeEffect>(2.0f, Color4(255, 0, 0, 255), 120);
    ASSERT_TRUE(stroke);
    EXPECT_EQ(style->GetEffects().Count(), 1);

    style->RemoveEffect(stroke);
    EXPECT_TRUE(style->GetEffects().IsEmpty());

    style->AddEffect<FontShadowEffect>();
    style->AddEffect<FontGradientEffect>();
    EXPECT_EQ(style->GetEffects().Count(), 2);

    style->RemoveAllEffects();
    EXPECT_TRUE(style->GetEffects().IsEmpty());
}

TEST(FontStyle, OnChangedIsInvokedByEffectsMutation)
{
    auto style = mmake<FontStyle>();

    int changes = 0;
    style->onChanged += [&]() { changes++; };

    auto stroke = style->AddEffect<FontStrokeEffect>();
    EXPECT_EQ(changes, 1);

    style->SetEffects({ mmake<FontShadowEffect>() });
    EXPECT_EQ(changes, 2);

    style->RemoveAllEffects();
    EXPECT_EQ(changes, 3);
}

TEST(FontStyle, CacheKeySharedForEqualContent)
{
    auto a = mmake<FontStyle>();
    auto b = mmake<FontStyle>();

    a->AddEffect<FontStrokeEffect>(2.0f, Color4(255, 0, 0, 255), 120);
    b->AddEffect<FontStrokeEffect>(2.0f, Color4(255, 0, 0, 255), 120);

    EXPECT_EQ(a->GetCacheKey(), b->GetCacheKey());
}

TEST(FontStyle, CacheKeyDiffersForDifferentContentAndUpdatesOnChange)
{
    auto a = mmake<FontStyle>();
    auto b = mmake<FontStyle>();

    a->AddEffect<FontStrokeEffect>(2.0f, Color4(255, 0, 0, 255), 120);
    b->AddEffect<FontStrokeEffect>(5.0f, Color4(0, 255, 0, 255), 120);

    EXPECT_NE(a->GetCacheKey(), b->GetCacheKey());

    UInt64 oldKey = a->GetCacheKey();
    a->SetEffects({ mmake<FontShadowEffect>() });
    EXPECT_NE(a->GetCacheKey(), oldKey);
}

TEST(FontStyle, CloneCopiesEffects)
{
    auto style = mmake<FontStyle>();
    auto stroke = style->AddEffect<FontStrokeEffect>(3.0f, Color4(0, 0, 255, 255), 90);

    auto clone = style->CloneAsRef<FontStyle>();
    ASSERT_EQ(clone->GetEffects().Count(), 1);

    auto clonedStroke = DynamicCast<FontStrokeEffect>(clone->GetEffects()[0]);
    ASSERT_TRUE(clonedStroke);
    EXPECT_NE(clonedStroke.Get(), stroke.Get());
    EXPECT_FLOAT_EQ(clonedStroke->radius, 3.0f);
    EXPECT_EQ(clone->GetCacheKey(), style->GetCacheKey());
}

TEST(FontStyleAsset, SerializationRoundTrip)
{
    auto asset = mmake<FontStyleAsset>();
    asset->AddEffect<FontStrokeEffect>(4.0f, Color4(10, 20, 30, 255), 77);
    asset->AddEffect<FontShadowEffect>(3.0f, Vec2I(5, 6), Color4(0, 0, 0, 128));

    DataDocument doc;
    asset->Serialize(doc);

    auto loaded = mmake<FontStyleAsset>();
    loaded->Deserialize(doc);

    ASSERT_EQ(loaded->GetEffects().Count(), 2);

    auto stroke = DynamicCast<FontStrokeEffect>(loaded->GetEffects()[0]);
    ASSERT_TRUE(stroke);
    EXPECT_FLOAT_EQ(stroke->radius, 4.0f);
    EXPECT_EQ(stroke->alphaThreshold, 77);

    auto shadow = DynamicCast<FontShadowEffect>(loaded->GetEffects()[1]);
    ASSERT_TRUE(shadow);
    EXPECT_EQ(shadow->offset, Vec2I(5, 6));

    EXPECT_EQ(loaded->GetCacheKey(), asset->GetCacheKey());
}

TEST(FontStyleAsset, ReferenceCanOwnInstance)
{
    EXPECT_TRUE(FontStyleAsset::IsReferenceCanOwnInstance());

    AssetRef<FontStyleAsset> ref;
    EXPECT_FALSE(ref.IsInstance());

    ref.CreateInstance();
    ASSERT_TRUE(ref.IsValid());
    EXPECT_TRUE(ref.IsInstance());

    ref->AddEffect<FontStrokeEffect>(1.5f, Color4(255, 255, 0, 255), 100);
    EXPECT_EQ(ref->GetEffects().Count(), 1);

    ref.RemoveInstance();
    EXPECT_FALSE(ref.IsValid());
    EXPECT_FALSE(ref.IsInstance());
}

TEST(FontStyleAsset, InstanceSerializesInlineAndRestores)
{
    AssetRef<FontStyleAsset> ref;
    ref.CreateInstance();
    ref->AddEffect<FontStrokeEffect>(2.5f, Color4(1, 2, 3, 255), 42);

    DataDocument doc;
    ref.Serialize(doc);

    ASSERT_TRUE(doc.FindMember("instance"));

    AssetRef<FontStyleAsset> loaded;
    loaded.Deserialize(doc);

    ASSERT_TRUE(loaded.IsValid());
    EXPECT_TRUE(loaded.IsInstance());
    EXPECT_NE(loaded.Get(), ref.Get());

    ASSERT_EQ(loaded->GetEffects().Count(), 1);
    auto stroke = DynamicCast<FontStrokeEffect>(loaded->GetEffects()[0]);
    ASSERT_TRUE(stroke);
    EXPECT_FLOAT_EQ(stroke->radius, 2.5f);
    EXPECT_EQ(stroke->alphaThreshold, 42);

    EXPECT_EQ(loaded->GetCacheKey(), ref->GetCacheKey());
}

TEST(FontStyleAsset, RegisteredByExtension)
{
    auto type = Assets::GetAssetTypeByExtension("fntstyle");
    ASSERT_TRUE(type);
    EXPECT_EQ(type, &TypeOf(FontStyleAsset));
}
