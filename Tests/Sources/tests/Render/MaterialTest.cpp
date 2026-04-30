#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Material.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Vector2.h"

using namespace o2;

TEST(IShaderParam, BaseHashDependsOnNameOnly)
{
    auto a = mmake<ShaderParamFloat>("uColor", 1.0f);
    auto b = mmake<ShaderParamFloat>("uColor", 1.0f);
    auto c = mmake<ShaderParamFloat>("uOther", 1.0f);

    auto baseA = static_cast<IShaderParam*>(a.Get());
    auto baseB = static_cast<IShaderParam*>(b.Get());
    auto baseC = static_cast<IShaderParam*>(c.Get());

    EXPECT_EQ(baseA->IShaderParam::ComputeHash(), baseB->IShaderParam::ComputeHash());
    EXPECT_NE(baseA->IShaderParam::ComputeHash(), baseC->IShaderParam::ComputeHash());
}

TEST(ShaderParamFloat, HashDependsOnNameAndValue)
{
    ShaderParamFloat sameA("u", 0.25f);
    ShaderParamFloat sameB("u", 0.25f);
    ShaderParamFloat diffValue("u", 0.50f);
    ShaderParamFloat diffName("v", 0.25f);

    EXPECT_EQ(sameA.ComputeHash(), sameB.ComputeHash());
    EXPECT_NE(sameA.ComputeHash(), diffValue.ComputeHash());
    EXPECT_NE(sameA.ComputeHash(), diffName.ComputeHash());
}

TEST(ShaderParamFloat, ValueRoundTrip)
{
    ShaderParamFloat p("u", 0.0f);
    p.SetValue(3.5f);
    EXPECT_FLOAT_EQ(p.GetValue(), 3.5f);
    EXPECT_EQ(p.GetName(), "u");
}

TEST(ShaderParamVec2, HashDistinguishesXAndY)
{
    ShaderParamVec2 xOnly("u", Vec2F(1.0f, 0.0f));
    ShaderParamVec2 yOnly("u", Vec2F(0.0f, 1.0f));
    ShaderParamVec2 same("u", Vec2F(1.0f, 0.0f));

    EXPECT_NE(xOnly.ComputeHash(), yOnly.ComputeHash());
    EXPECT_EQ(xOnly.ComputeHash(), same.ComputeHash());
}

TEST(ShaderParamColor, HashDependsOnAllChannels)
{
    ShaderParamColor opaque("u", Color4(255, 0, 0, 255));
    ShaderParamColor sameOpaque("u", Color4(255, 0, 0, 255));
    ShaderParamColor halfAlpha("u", Color4(255, 0, 0, 128));
    ShaderParamColor diffRgb("u", Color4(0, 255, 0, 255));

    EXPECT_EQ(opaque.ComputeHash(), sameOpaque.ComputeHash());
    EXPECT_NE(opaque.ComputeHash(), halfAlpha.ComputeHash());
    EXPECT_NE(opaque.ComputeHash(), diffRgb.ComputeHash());
}

TEST(ShaderParamInt, HashAndValueRoundTrip)
{
    ShaderParamInt p("u", 0);
    p.SetValue(42);
    EXPECT_EQ(p.GetValue(), 42);

    ShaderParamInt other("u", 42);
    EXPECT_EQ(p.ComputeHash(), other.ComputeHash());

    other.SetValue(43);
    EXPECT_NE(p.ComputeHash(), other.ComputeHash());
}

TEST(Material, AddParamAppendsNewName)
{
    Material m;
    EXPECT_EQ(m.GetParams().Count(), 0);

    auto a = mmake<ShaderParamFloat>("alpha", 0.5f);
    m.AddParam(a);

    auto b = mmake<ShaderParamFloat>("beta", 0.25f);
    m.AddParam(b);

    EXPECT_EQ(m.GetParams().Count(), 2);
    EXPECT_EQ(m.GetShaderParam("alpha").Get(), a.Get());
    EXPECT_EQ(m.GetShaderParam("beta").Get(), b.Get());
}

TEST(Material, AddParamReplacesByName)
{
    Material m;
    auto first = mmake<ShaderParamFloat>("alpha", 0.5f);
    m.AddParam(first);
    EXPECT_EQ(m.GetParams().Count(), 1);

    auto replacement = mmake<ShaderParamFloat>("alpha", 0.9f);
    m.AddParam(replacement);

    EXPECT_EQ(m.GetParams().Count(), 1);
    EXPECT_EQ(m.GetShaderParam("alpha").Get(), replacement.Get());
    EXPECT_NE(m.GetShaderParam("alpha").Get(), first.Get());
}

TEST(Material, RemoveParamByName)
{
    Material m;
    m.AddParam(mmake<ShaderParamFloat>("alpha", 0.5f));
    m.AddParam(mmake<ShaderParamFloat>("beta", 0.25f));
    EXPECT_EQ(m.GetParams().Count(), 2);

    m.RemoveParam("alpha");
    EXPECT_EQ(m.GetParams().Count(), 1);
    EXPECT_EQ(m.GetShaderParam("alpha"), nullptr);
    EXPECT_NE(m.GetShaderParam("beta"), nullptr);

    m.RemoveParam("missing");
    EXPECT_EQ(m.GetParams().Count(), 1);
}

TEST(Material, GetAllShaderParamsMapReflectsCurrentParams)
{
    Material m;
    auto a = mmake<ShaderParamFloat>("a", 1.0f);
    auto b = mmake<ShaderParamInt>("b", 7);
    m.AddParam(a);
    m.AddParam(b);

    auto map = m.GetAllShaderParamsMap();
    EXPECT_EQ(map.Count(), 2);
    EXPECT_EQ(map["a"].Get(), a.Get());
    EXPECT_EQ(map["b"].Get(), b.Get());
}

TEST(Material, AddTextureSamplerAppendsAndReplacesByUniformName)
{
    Material m;
    EXPECT_EQ(m.GetTextureSamplers().Count(), 0);

    TextureSampler s1;
    s1.samplerUniformName = "u_tex2";
    s1.texCoordsAttrName = "a_uv2";
    m.AddTextureSampler(s1);

    TextureSampler s2;
    s2.samplerUniformName = "u_tex3";
    s2.texCoordsAttrName = "a_uv3";
    m.AddTextureSampler(s2);

    EXPECT_EQ(m.GetTextureSamplers().Count(), 2);

    TextureSampler s1Replacement;
    s1Replacement.samplerUniformName = "u_tex2";
    s1Replacement.texCoordsAttrName = "a_uv2_alt";
    m.AddTextureSampler(s1Replacement);

    EXPECT_EQ(m.GetTextureSamplers().Count(), 2);
    auto& samplers = m.GetTextureSamplers();
    int idx = samplers.IndexOf([](const TextureSampler& s) { return s.samplerUniformName == "u_tex2"; });
    ASSERT_GE(idx, 0);
    EXPECT_EQ(samplers[idx].texCoordsAttrName, "a_uv2_alt");
}

TEST(Material, RemoveTextureSamplerByName)
{
    Material m;
    TextureSampler s1; s1.samplerUniformName = "u_tex2";
    TextureSampler s2; s2.samplerUniformName = "u_tex3";
    m.AddTextureSampler(s1);
    m.AddTextureSampler(s2);
    EXPECT_EQ(m.GetTextureSamplers().Count(), 2);

    m.RemoveTextureSampler("u_tex2");
    EXPECT_EQ(m.GetTextureSamplers().Count(), 1);
    EXPECT_EQ(m.GetTextureSamplers()[0].samplerUniformName, "u_tex3");

    m.RemoveTextureSampler("missing");
    EXPECT_EQ(m.GetTextureSamplers().Count(), 1);
}

TEST(Material, TotalTextureChannelsCountIsOnePlusSamplers)
{
    Material m;
    EXPECT_EQ(m.GetTotalTextureChannelsCount(), 1);

    TextureSampler s1; s1.samplerUniformName = "u_tex2";
    TextureSampler s2; s2.samplerUniformName = "u_tex3";
    m.AddTextureSampler(s1);
    m.AddTextureSampler(s2);
    EXPECT_EQ(m.GetTotalTextureChannelsCount(), 3);

    m.RemoveTextureSampler("u_tex2");
    EXPECT_EQ(m.GetTotalTextureChannelsCount(), 2);
}

TEST(Material, GetHashStableUntilMutation)
{
    Material m;
    size_t h1 = m.GetHash();
    size_t h2 = m.GetHash();
    EXPECT_EQ(h1, h2);

    m.AddParam(mmake<ShaderParamFloat>("u", 1.0f));
    size_t h3 = m.GetHash();
    EXPECT_NE(h1, h3);

    size_t h4 = m.GetHash();
    EXPECT_EQ(h3, h4);
}

TEST(Material, InvalidateHashForcesRecompute)
{
    Material m;
    auto p = mmake<ShaderParamFloat>("u", 1.0f);
    m.AddParam(p);

    size_t before = m.GetHash();

    p->SetValue(99.0f);
    size_t cached = m.GetHash();
    EXPECT_EQ(before, cached);

    m.InvalidateHash();
    size_t recomputed = m.GetHash();
    EXPECT_NE(cached, recomputed);
}

TEST(Material, SetBlendModeRoundTripAndAffectsHash)
{
    Material a;
    a.SetBlendMode(BlendMode::Normal);
    EXPECT_EQ(a.GetBlendMode(), BlendMode::Normal);

    size_t hashNormal = a.GetHash();

    a.SetBlendMode(BlendMode::Add);
    EXPECT_EQ(a.GetBlendMode(), BlendMode::Add);
    size_t hashAdd = a.GetHash();
    EXPECT_NE(hashNormal, hashAdd);
}

TEST(Material, CopyConstructorDeepClonesParams)
{
    Material src;
    auto p1 = mmake<ShaderParamFloat>("alpha", 0.7f);
    auto p2 = mmake<ShaderParamColor>("tint", Color4(10, 20, 30, 200));
    src.AddParam(p1);
    src.AddParam(p2);
    src.SetBlendMode(BlendMode::Add);

    Material copy(src);
    ASSERT_EQ(copy.GetParams().Count(), 2);

    auto copyAlpha = DynamicCast<ShaderParamFloat>(copy.GetShaderParam("alpha"));
    auto copyTint = DynamicCast<ShaderParamColor>(copy.GetShaderParam("tint"));
    ASSERT_NE(copyAlpha, nullptr);
    ASSERT_NE(copyTint, nullptr);

    EXPECT_NE(copyAlpha.Get(), p1.Get());
    EXPECT_NE(copyTint.Get(), p2.Get());

    EXPECT_FLOAT_EQ(copyAlpha->GetValue(), 0.7f);
    EXPECT_EQ(copyTint->GetValue(), Color4(10, 20, 30, 200));
    EXPECT_EQ(copy.GetBlendMode(), BlendMode::Add);

    EXPECT_EQ(src.GetHash(), copy.GetHash());
}

TEST(Material, SetParamsReplacesEntireListAndInvalidatesHash)
{
    Material m;
    m.AddParam(mmake<ShaderParamFloat>("old", 0.0f));
    size_t hashWithOld = m.GetHash();

    Vector<Ref<IShaderParam>> fresh;
    fresh.Add(mmake<ShaderParamInt>("n", 5));
    fresh.Add(mmake<ShaderParamInt>("m", 6));
    m.SetParams(fresh);

    EXPECT_EQ(m.GetParams().Count(), 2);
    EXPECT_EQ(m.GetShaderParam("old"), nullptr);
    EXPECT_NE(m.GetShaderParam("n"), nullptr);
    EXPECT_NE(m.GetShaderParam("m"), nullptr);

    size_t hashFresh = m.GetHash();
    EXPECT_NE(hashWithOld, hashFresh);
}
