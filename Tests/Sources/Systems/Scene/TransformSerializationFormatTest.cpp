#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/ActorTransform.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Serialization/DataValue.h"

using namespace o2;

// Locks the on-disk "Transform" node format: flat Vec3F members named position/size/scale/pivot/
// eulerAngles/shear (+ anchors/offsets/minSize/maxSize/weight for widgets). Legacy formats must
// still load: pre-3D (2D vectors, float angle and shear) and phase-2 (positionZ/angleXY/scaleZ).

namespace
{
    Vec2F GetVec2(const DataValue& node, const char* name)
    {
        auto member = node.FindMember(name);
        EXPECT_TRUE(member != nullptr) << "missing member " << name;
        if (!member)
            return Vec2F();

        Vec2F result;
        member->Get(result);
        return result;
    }

    Vec3F GetVec3(const DataValue& node, const char* name)
    {
        auto member = node.FindMember(name);
        EXPECT_TRUE(member != nullptr) << "missing member " << name;
        if (!member)
            return Vec3F();

        Vec3F result;
        member->Get(result);
        return result;
    }
}

TEST(TransformSerializationFormat, PlainTransformWritesAllChangedFields)
{
    ActorTransform t(Vec2F(100, 50), Vec2F(10, 20), 0.5f, Vec2F(2, 3), Vec2F(0.3f, 0.7f));
    t.SetShear(Vec3F(0.1f, 0.2f, 0.3f));
    t.SetPositionZ(5.0f);
    t.SetEulerAngles(Vec3F(0.2f, 0.3f, 0.5f));
    t.SetScaleZ(2.0f);
    t.SetSizeZ(30.0f);
    t.SetPivotZ(0.25f);

    DataDocument doc;
    doc.Set(t);

    ASSERT_TRUE(doc.IsObject());
    EXPECT_EQ(doc.GetMembersCount(), 6);

    EXPECT_EQ(GetVec3(doc, "position"), Vec3F(10, 20, 5));
    EXPECT_EQ(GetVec3(doc, "size"), Vec3F(100, 50, 30));
    EXPECT_EQ(GetVec3(doc, "scale"), Vec3F(2, 3, 2));
    EXPECT_EQ(GetVec3(doc, "pivot"), Vec3F(0.3f, 0.7f, 0.25f));
    EXPECT_EQ(GetVec3(doc, "eulerAngles"), Vec3F(0.2f, 0.3f, 0.5f));
    EXPECT_EQ(GetVec3(doc, "shear"), Vec3F(0.1f, 0.2f, 0.3f));
}

TEST(TransformSerializationFormat, PlainTransformSkipsDefaultValuedFields)
{
    ActorTransform t;

    DataDocument doc;
    doc.Set(t);

    ASSERT_TRUE(doc.IsObject());
    EXPECT_EQ(doc.FindMember("position"), nullptr);
    EXPECT_EQ(doc.FindMember("size"), nullptr);
    EXPECT_EQ(doc.FindMember("scale"), nullptr);
    EXPECT_EQ(doc.FindMember("eulerAngles"), nullptr);
    EXPECT_EQ(doc.FindMember("shear"), nullptr);

    EXPECT_EQ(GetVec3(doc, "pivot"), Vec3F(0.5f, 0.5f, 0.0f));
}

TEST(TransformSerializationFormat, PlainTransformDeserializeReadsFields)
{
    ActorTransform t(Vec2F(100, 50), Vec2F(10, 20), 0.5f, Vec2F(2, 3), Vec2F(0.3f, 0.7f));
    t.SetEulerAngles(Vec3F(0.2f, 0.3f, 0.5f));
    t.SetShear(Vec3F(0.1f, 0.2f, 0.3f));
    t.SetSizeZ(30.0f);
    t.SetPivotZ(0.25f);

    DataDocument doc;
    doc.Set(t);

    ActorTransform r;
    doc.Get(r);

    EXPECT_EQ(r.GetPosition(), Vec3F(10, 20, 0));
    EXPECT_EQ(r.GetSize(), Vec3F(100, 50, 30));
    EXPECT_EQ(r.GetScale(), Vec3F(2, 3, 1));
    EXPECT_EQ(r.GetPivot(), Vec3F(0.3f, 0.7f, 0.25f));
    EXPECT_EQ(r.GetEulerAngles(), Vec3F(0.2f, 0.3f, 0.5f));
    EXPECT_EQ(r.GetShear(), Vec3F(0.1f, 0.2f, 0.3f));
}

TEST(TransformSerializationFormat, LegacyPre3DFormatLoads)
{
    DataDocument doc;
    ASSERT_TRUE(doc.LoadFromData(R"({
        "position": { "x": 10.0, "y": 20.0 },
        "size": { "x": 100.0, "y": 50.0 },
        "scale": { "x": 2.0, "y": 3.0 },
        "pivot": { "x": 0.3, "y": 0.7 },
        "angle": 0.5,
        "shear": 0.1
    })"));

    ActorTransform t;
    doc.Get(t);

    EXPECT_EQ(t.GetPosition(), Vec3F(10, 20, 0));
    EXPECT_EQ(t.GetSize(), Vec3F(100, 50, 0));
    EXPECT_EQ(t.GetScale(), Vec3F(2, 3, 1));
    EXPECT_EQ(t.GetPivot(), Vec3F(0.3f, 0.7f, 0));
    EXPECT_EQ(t.GetEulerAngles(), Vec3F(0, 0, 0.5f));
    EXPECT_EQ(t.GetShear(), Vec3F(0.1f, 0, 0));
    EXPECT_FLOAT_EQ(t.GetShear2D(), 0.1f);
    EXPECT_FALSE(t.Is3D());
}

TEST(TransformSerializationFormat, LegacyPhase2FormatLoads)
{
    DataDocument doc;
    ASSERT_TRUE(doc.LoadFromData(R"({
        "position": { "x": 10.0, "y": 20.0 },
        "size": { "x": 100.0, "y": 50.0 },
        "angle": 0.5,
        "shear": 0.1,
        "positionZ": 5.0,
        "angleXY": { "x": 0.2, "y": 0.3 },
        "scaleZ": 2.0
    })"));

    ActorTransform t;
    doc.Get(t);

    EXPECT_EQ(t.GetPosition(), Vec3F(10, 20, 5));
    EXPECT_EQ(t.GetSize(), Vec3F(100, 50, 0));
    EXPECT_EQ(t.GetScale(), Vec3F(1, 1, 2));
    EXPECT_EQ(t.GetEulerAngles(), Vec3F(0.2f, 0.3f, 0.5f));
    EXPECT_EQ(t.GetShear(), Vec3F(0.1f, 0, 0));
    EXPECT_TRUE(t.Is3D());
}

TEST(TransformSerializationFormat, WidgetLayoutWritesOwnFieldsOnly)
{
    WidgetLayout l(Vec2F(0.1f, 0.2f), Vec2F(0.7f, 0.8f), Vec2F(1, 2), Vec2F(3, 4));
    l.SetPivot2D(Vec2F(0.5f, 0.5f));
    l.SetMinimalSize(Vec2F(5, 6));
    l.SetMaximalSize(Vec2F(100, 200));
    l.SetWeight(Vec2F(2, 3));

    DataDocument doc;
    doc.Set(static_cast<const ActorTransform&>(l));

    ASSERT_TRUE(doc.IsObject());
    EXPECT_EQ(doc.GetMembersCount(), 7);

    EXPECT_EQ(doc.FindMember("position"), nullptr);
    EXPECT_EQ(doc.FindMember("size"), nullptr);
    EXPECT_EQ(doc.FindMember("scale"), nullptr);
    EXPECT_EQ(doc.FindMember("pivot"), nullptr);
    EXPECT_EQ(doc.FindMember("eulerAngles"), nullptr);
    EXPECT_EQ(doc.FindMember("shear"), nullptr);

    EXPECT_EQ(GetVec2(doc, "anchorMin"), Vec2F(0.1f, 0.2f));
    EXPECT_EQ(GetVec2(doc, "anchorMax"), Vec2F(0.7f, 0.8f));
    EXPECT_EQ(GetVec2(doc, "offsetMin"), Vec2F(1, 2));
    EXPECT_EQ(GetVec2(doc, "offsetMax"), Vec2F(3, 4));
    EXPECT_EQ(GetVec2(doc, "minSize"), Vec2F(5, 6));
    EXPECT_EQ(GetVec2(doc, "maxSize"), Vec2F(100, 200));
    EXPECT_EQ(GetVec2(doc, "weight"), Vec2F(2, 3));
}

TEST(TransformSerializationFormat, WidgetLayoutUsesDeclaredDefaultsForSkipping)
{
    WidgetLayout l(Vec2F(0, 0), Vec2F(1, 1), Vec2F(0, 0), Vec2F(0, 0));

    DataDocument doc;
    doc.Set(static_cast<const ActorTransform&>(l));

    ASSERT_TRUE(doc.IsObject());

    EXPECT_EQ(doc.FindMember("anchorMin"), nullptr);
    EXPECT_EQ(doc.FindMember("offsetMin"), nullptr);
    EXPECT_EQ(doc.FindMember("minSize"), nullptr);
    EXPECT_EQ(doc.FindMember("maxSize"), nullptr);
    EXPECT_EQ(doc.FindMember("weight"), nullptr);

    EXPECT_EQ(GetVec2(doc, "anchorMax"), Vec2F(1, 1));
    EXPECT_EQ(GetVec2(doc, "offsetMax"), Vec2F(0, 0));
}

TEST(TransformSerializationFormat, WidgetLayoutDeserializeReadsFields)
{
    WidgetLayout l(Vec2F(0.1f, 0.2f), Vec2F(0.7f, 0.8f), Vec2F(1, 2), Vec2F(3, 4));
    l.SetMinimalSize(Vec2F(5, 6));
    l.SetWeight(Vec2F(2, 3));

    DataDocument doc;
    doc.Set(static_cast<const ActorTransform&>(l));

    WidgetLayout r;
    doc.Get(static_cast<ActorTransform&>(r));

    EXPECT_EQ(r.GetAnchorMin(), Vec2F(0.1f, 0.2f));
    EXPECT_EQ(r.GetAnchorMax(), Vec2F(0.7f, 0.8f));
    EXPECT_EQ(r.GetOffsetMin(), Vec2F(1, 2));
    EXPECT_EQ(r.GetOffsetMax(), Vec2F(3, 4));
    EXPECT_EQ(r.GetMinimalSize(), Vec2F(5, 6));
    EXPECT_EQ(r.GetWeight(), Vec2F(2, 3));
}

TEST(TransformSerializationFormat, PlainTransformDeltaWritesOnlyDifferences)
{
    ActorTransform origin(Vec2F(100, 50), Vec2F(10, 20), 0.5f, Vec2F(2, 3), Vec2F(0.3f, 0.7f));
    ActorTransform changed(origin);
    changed.SetPosition2D(Vec2F(30, 40));

    DataDocument doc;
    changed.SerializeDelta(doc, origin);

    ASSERT_TRUE(doc.IsObject());
    EXPECT_EQ(doc.GetMembersCount(), 1);
    EXPECT_EQ(GetVec3(doc, "position"), Vec3F(30, 40, 0));

    ActorTransform restored;
    restored.DeserializeDelta(doc, origin);

    EXPECT_EQ(restored.GetPosition2D(), Vec2F(30, 40));
    EXPECT_EQ(restored.GetSize2D(), Vec2F(100, 50));
    EXPECT_EQ(restored.GetScale2D(), Vec2F(2, 3));
    EXPECT_EQ(restored.GetPivot2D(), Vec2F(0.3f, 0.7f));
    EXPECT_FLOAT_EQ(restored.GetAngle(), 0.5f);
}

TEST(TransformSerializationFormat, LegacyDeltaFormatLoads)
{
    ActorTransform origin(Vec2F(100, 50), Vec2F(10, 20), 0.5f, Vec2F(2, 3), Vec2F(0.3f, 0.7f));
    origin.SetPositionZ(1.0f);

    DataDocument doc;
    ASSERT_TRUE(doc.LoadFromData(R"({
        "position": { "x": 30.0, "y": 40.0 },
        "angle": 0.7
    })"));

    ActorTransform restored;
    restored.DeserializeDelta(doc, origin);

    EXPECT_EQ(restored.GetPosition2D(), Vec2F(30, 40));
    EXPECT_FLOAT_EQ(restored.GetPositionZ(), 1.0f); // z comes from origin, legacy vector has no z
    EXPECT_FLOAT_EQ(restored.GetAngle(), 0.7f);
    EXPECT_EQ(restored.GetSize2D(), Vec2F(100, 50));
    EXPECT_EQ(restored.GetScale2D(), Vec2F(2, 3));
    EXPECT_EQ(restored.GetPivot2D(), Vec2F(0.3f, 0.7f));
}

TEST(TransformSerializationFormat, WidgetLayoutDeltaSkipsBaseTransformFields)
{
    WidgetLayout origin(Vec2F(0, 0), Vec2F(1, 1), Vec2F(0, 0), Vec2F(0, 0));
    origin.SetPivot2D(Vec2F(0.5f, 0.5f));

    WidgetLayout changed(origin);
    changed.SetAnchorMin(Vec2F(0.5f, 0.5f));
    changed.SetPivot2D(Vec2F(0.1f, 0.9f)); // differs from origin, but disabled for serialization

    DataDocument doc;
    changed.SerializeDelta(doc, origin);

    ASSERT_TRUE(doc.IsObject());
    EXPECT_EQ(doc.GetMembersCount(), 1);
    EXPECT_EQ(GetVec2(doc, "anchorMin"), Vec2F(0.5f, 0.5f));
    EXPECT_EQ(doc.FindMember("pivot"), nullptr);

    WidgetLayout restored;
    restored.DeserializeDelta(doc, origin);

    EXPECT_EQ(restored.GetAnchorMin(), Vec2F(0.5f, 0.5f));
    EXPECT_EQ(restored.GetAnchorMax(), Vec2F(1, 1));
    EXPECT_EQ(restored.GetOffsetMax(), Vec2F(0, 0));
    EXPECT_EQ(restored.GetPivot2D(), Vec2F(0.5f, 0.5f)); // unchanged fields come from origin
}
