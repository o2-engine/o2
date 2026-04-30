#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Serialization/DataValueConverters.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

TEST(DataValue, IntRoundtrip) {
    DataDocument doc;
    doc.Set(42);
    EXPECT_TRUE(doc.IsNumber());
    EXPECT_FALSE(doc.IsString());
    EXPECT_FALSE(doc.IsBoolean());
    EXPECT_FALSE(doc.IsNull());

    int value = 0;
    doc.Get(value);
    EXPECT_EQ(value, 42);
}

TEST(DataValue, FloatRoundtrip) {
    DataDocument doc;
    doc.Set(3.14f);
    EXPECT_TRUE(doc.IsNumber());

    float v = 0.0f;
    doc.Get(v);
    EXPECT_FLOAT_EQ(v, 3.14f);
}

TEST(DataValue, DoubleRoundtrip) {
    DataDocument doc;
    doc.Set(2.71828);

    double v = 0.0;
    doc.Get(v);
    EXPECT_DOUBLE_EQ(v, 2.71828);
}

TEST(DataValue, BoolRoundtrip) {
    DataDocument doc;
    doc.Set(true);
    EXPECT_TRUE(doc.IsBoolean());

    bool v = false;
    doc.Get(v);
    EXPECT_TRUE(v);

    doc.Set(false);
    doc.Get(v);
    EXPECT_FALSE(v);
}

TEST(DataValue, StringRoundtrip) {
    DataDocument doc;
    String original = "hello world";
    doc.Set(original);
    EXPECT_TRUE(doc.IsString());

    String v;
    doc.Get(v);
    EXPECT_EQ(v, original);
}

TEST(DataValue, StringWithEscapeCharsSurvivesJsonRoundtrip) {
    DataDocument doc;
    String original = "line1\nline2\twith \"quotes\" and \\backslash";
    doc.Set(original);

    String json = doc.SaveAsString(DataDocument::Format::JSON);

    DataDocument loaded;
    ASSERT_TRUE(loaded.LoadFromData(json));

    String v;
    loaded.Get(v);
    EXPECT_EQ(v, original);
}

TEST(DataValue, SetNullMakesIsNullTrue) {
    DataDocument doc;
    doc.Set(123);
    EXPECT_FALSE(doc.IsNull());

    doc.SetNull();
    EXPECT_TRUE(doc.IsNull());
}

TEST(DataValue, ArrayAddAndAccessByIndex) {
    DataDocument doc;
    doc.SetArray();
    EXPECT_TRUE(doc.IsArray());
    EXPECT_EQ(doc.GetElementsCount(), 0);

    doc.AddElement().Set(10);
    doc.AddElement().Set(20);
    doc.AddElement().Set(30);

    EXPECT_EQ(doc.GetElementsCount(), 3);

    int v = 0;
    doc[0].Get(v); EXPECT_EQ(v, 10);
    doc[1].Get(v); EXPECT_EQ(v, 20);
    doc[2].Get(v); EXPECT_EQ(v, 30);
}

TEST(DataValue, ArrayIterationViaRangeBasedFor) {
    DataDocument doc;
    doc.SetArray();
    for (int i = 1; i <= 4; ++i)
        doc.AddElement().Set(i * 10);

    int sum = 0;
    for (auto& el : doc)
    {
        int v = 0;
        el.Get(v);
        sum += v;
    }
    EXPECT_EQ(sum, 100);
}

TEST(DataValue, ObjectAddMemberAndAccess) {
    DataDocument doc;
    doc.SetObject();
    EXPECT_TRUE(doc.IsObject());
    EXPECT_EQ(doc.GetMembersCount(), 0);

    doc.AddMember("x").Set(1);
    doc.AddMember("y").Set(2);
    doc.AddMember("name").Set(String("alice"));

    EXPECT_EQ(doc.GetMembersCount(), 3);

    int x = 0, y = 0;
    String name;
    doc["x"].Get(x);
    doc["y"].Get(y);
    doc["name"].Get(name);

    EXPECT_EQ(x, 1);
    EXPECT_EQ(y, 2);
    EXPECT_EQ(name, "alice");
}

TEST(DataValue, FindMemberReturnsNullForMissing) {
    DataDocument doc;
    doc.SetObject();
    doc.AddMember("present").Set(1);

    EXPECT_NE(doc.FindMember("present"), nullptr);
    EXPECT_EQ(doc.FindMember("missing"), nullptr);
}

TEST(DataValue, RemoveMember) {
    DataDocument doc;
    doc.SetObject();
    doc.AddMember("a").Set(1);
    doc.AddMember("b").Set(2);
    EXPECT_EQ(doc.GetMembersCount(), 2);

    doc.RemoveMember("a");
    EXPECT_EQ(doc.GetMembersCount(), 1);
    EXPECT_EQ(doc.FindMember("a"), nullptr);
    EXPECT_NE(doc.FindMember("b"), nullptr);
}

TEST(DataValue, JsonRoundtripPreservesObject) {
    DataDocument doc;
    doc.SetObject();
    doc.AddMember("intVal").Set(42);
    doc.AddMember("floatVal").Set(1.5f);
    doc.AddMember("boolVal").Set(true);
    doc.AddMember("strVal").Set(String("hello"));

    String json = doc.SaveAsString(DataDocument::Format::JSON);
    EXPECT_FALSE(json.IsEmpty());

    DataDocument reloaded;
    ASSERT_TRUE(reloaded.LoadFromData(json));

    int i = 0;
    float f = 0.0f;
    bool b = false;
    String s;
    reloaded["intVal"].Get(i);
    reloaded["floatVal"].Get(f);
    reloaded["boolVal"].Get(b);
    reloaded["strVal"].Get(s);

    EXPECT_EQ(i, 42);
    EXPECT_FLOAT_EQ(f, 1.5f);
    EXPECT_TRUE(b);
    EXPECT_EQ(s, "hello");
}

TEST(DataValue, VectorOfPrimitivesRoundtrip) {
    DataDocument doc;
    Vector<int> source = { 10, 20, 30, 40, 50 };
    doc.Set(source);

    Vector<int> result;
    doc.Get(result);
    ASSERT_EQ(result.Count(), source.Count());
    for (int i = 0; i < source.Count(); ++i)
        EXPECT_EQ(result[i], source[i]);
}

TEST(DataValue, Vec2FRoundtrip) {
    DataDocument doc;
    Vec2F src(3.5f, -7.25f);
    doc.Set(src);

    Vec2F dst;
    doc.Get(dst);
    EXPECT_FLOAT_EQ(dst.x, src.x);
    EXPECT_FLOAT_EQ(dst.y, src.y);
}

TEST(DataValue, RectFRoundtrip) {
    DataDocument doc;
    RectF src(1.0f, 100.0f, 50.0f, 10.0f);
    doc.Set(src);

    RectF dst;
    doc.Get(dst);
    EXPECT_TRUE(src == dst);
}

TEST(DataValue, Color4Roundtrip) {
    DataDocument doc;
    Color4 src(11, 22, 33, 44);
    doc.Set(src);

    Color4 dst;
    doc.Get(dst);
    EXPECT_EQ(src, dst);
}

TEST(DataValue, AssignmentOperatorEquivalentToSet) {
    DataDocument doc;
    doc = 999;
    EXPECT_TRUE(doc.IsNumber());
    int v = 0;
    doc.Get(v);
    EXPECT_EQ(v, 999);
}

TEST(DataValue, NestedObject) {
    DataDocument doc;
    doc.SetObject();

    auto& outer = doc.AddMember("outer");
    outer.SetObject();
    outer.AddMember("inner").Set(123);

    int v = 0;
    doc["outer"]["inner"].Get(v);
    EXPECT_EQ(v, 123);
}

TEST(DataValue, IntDeltaRestoresChangedValue) {
    DataDocument doc;
    int origin = 5;
    int curr = 17;
    doc.SetDelta(curr, origin);

    int restored = 0;
    doc.GetDelta(restored, origin);
    EXPECT_EQ(restored, curr);
}
