#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/Containers/Vector.h"

using namespace o2;

// copying a DataValue must keep the container type even when empty (empty array/object once copied as Null)

TEST(DataValueCopy, EmptyArrayKeepsArrayTypeThroughCopyCtor)
{
    DataDocument src;
    src.SetArray();
    ASSERT_TRUE(src.IsArray());

    DataDocument copy(src);
    EXPECT_TRUE(copy.IsArray()) << "empty array must stay an array after copy, not become Null";
    EXPECT_FALSE(copy.IsNull());
}

TEST(DataValueCopy, EmptyArrayKeepsArrayTypeThroughAssignment)
{
    DataDocument src;
    src.SetArray();

    DataDocument dst;
    dst = src;
    EXPECT_TRUE(dst.IsArray()) << "empty array must stay an array after assignment, not become Null";
}

TEST(DataValueCopy, EmptyObjectKeepsObjectTypeThroughCopyCtor)
{
    DataDocument src;
    src.SetObject();
    ASSERT_TRUE(src.IsObject());

    DataDocument copy(src);
    EXPECT_TRUE(copy.IsObject()) << "empty object must stay an object after copy, not become Null";
}

TEST(DataValueCopy, EmptyVectorSerializationRoundTripsThroughCopy)
{
    // The concrete undo case: serialize an empty vector, copy the doc, deserialize into a non-empty
    // vector -> it must clear (size 0).
    Vector<int> v = { 1, 2, 3 };

    DataDocument empty;
    DataDocument tmp; tmp = Vector<int>{};   // serialize empty vector via converter
    empty = tmp;                             // a copy, like PropertyChangeAction stores

    empty.Get(v);                            // deserialize back into v
    EXPECT_EQ(v.Count(), 0) << "deserializing a copied empty-vector doc must clear the vector";
}
