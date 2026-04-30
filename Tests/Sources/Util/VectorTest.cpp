#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Types/Containers/Vector.h"

using namespace o2;

TEST(Vector, InitializerListAndCount) {
    Vector<int> v = { 1, 2, 3, 4, 5 };
    EXPECT_EQ(v.Count(), 5);
    EXPECT_FALSE(v.IsEmpty());

    Vector<int> empty;
    EXPECT_EQ(empty.Count(), 0);
    EXPECT_TRUE(empty.IsEmpty());
}

TEST(Vector, AddAppendsAndReturnsReference) {
    Vector<int> v;
    int& ref = v.Add(42);
    EXPECT_EQ(v.Count(), 1);
    EXPECT_EQ(ref, 42);

    ref = 100;
    EXPECT_EQ(v[0], 100);
}

TEST(Vector, AddOtherVectorAppendsAll) {
    Vector<int> a = { 1, 2 };
    Vector<int> b = { 3, 4, 5 };
    a.Add(b);
    EXPECT_EQ(a.Count(), 5);
    EXPECT_EQ(a[2], 3);
    EXPECT_EQ(a[4], 5);
    // b should be unchanged.
    EXPECT_EQ(b.Count(), 3);
}

TEST(Vector, PlusOperatorReturnsNewVector) {
    Vector<int> a = { 1, 2 };
    Vector<int> b = { 3, 4 };
    Vector<int> sum = a + b;

    EXPECT_EQ(sum.Count(), 4);
    EXPECT_EQ(a.Count(), 2);
    EXPECT_EQ(b.Count(), 2);

    Vector<int> sumWithValue = a + 99;
    EXPECT_EQ(sumWithValue.Count(), 3);
    EXPECT_EQ(sumWithValue.Last(), 99);
    EXPECT_EQ(a.Count(), 2);
}

TEST(Vector, PlusEqualsAppendsInPlace) {
    Vector<int> a = { 1, 2 };
    a += 3;
    EXPECT_EQ(a.Count(), 3);
    EXPECT_EQ(a.Last(), 3);

    Vector<int> b = { 4, 5 };
    a += b;
    EXPECT_EQ(a.Count(), 5);
    EXPECT_EQ(a.Last(), 5);
}

TEST(Vector, MinusOperatorRemovesFirstOccurrence) {
    // operator-(value) calls Remove(value) which strips only the first match.
    Vector<int> a = { 1, 2, 3, 2, 4 };
    Vector<int> diff = a - 2;
    EXPECT_EQ(diff.Count(), 4);
    EXPECT_EQ(diff.IndexOf(2), 2);   // second 2 still present
    EXPECT_TRUE(diff.Contains(1));
    EXPECT_TRUE(diff.Contains(3));
    EXPECT_TRUE(diff.Contains(4));
    // Original is preserved.
    EXPECT_EQ(a.Count(), 5);
}

TEST(Vector, FindAndContains) {
    Vector<int> v = { 10, 20, 30, 40 };
    EXPECT_TRUE(v.Contains(20));
    EXPECT_FALSE(v.Contains(99));

    EXPECT_EQ(v.IndexOf(30), 2);
    EXPECT_EQ(v.IndexOf(99), -1);

    int* found = v.Find([](const int& x) { return x > 25; });
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(*found, 30);

    int* missing = v.Find([](const int& x) { return x > 100; });
    EXPECT_EQ(missing, nullptr);
}

TEST(Vector, InsertAtPosition) {
    Vector<int> v = { 1, 2, 4, 5 };
    v.Insert(3, 2);
    ASSERT_EQ(v.Count(), 5);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v[3], 4);
    EXPECT_EQ(v[4], 5);

    v.Insert(0, 0); // at beginning
    EXPECT_EQ(v.First(), 0);
}

TEST(Vector, RemoveByValueRemovesFirst) {
    Vector<int> v = { 1, 2, 3, 2, 4 };
    v.Remove(2);
    EXPECT_EQ(v.Count(), 4);
    // First 2 was removed, the second remains.
    EXPECT_EQ(v.IndexOf(2), 2);
}

TEST(Vector, RemoveAtIndex) {
    Vector<int> v = { 1, 2, 3, 4 };
    v.RemoveAt(1);
    EXPECT_EQ(v.Count(), 3);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 4);
}

TEST(Vector, PopBackReturnsAndRemovesLast) {
    Vector<int> v = { 1, 2, 3 };
    int last = v.PopBack();
    EXPECT_EQ(last, 3);
    EXPECT_EQ(v.Count(), 2);
    EXPECT_EQ(v.Last(), 2);
}

TEST(Vector, ResizeShrinksAndExpands) {
    Vector<int> v = { 1, 2, 3, 4, 5 };
    v.Resize(3);
    EXPECT_EQ(v.Count(), 3);
    EXPECT_EQ(v.Last(), 3);

    v.Resize(5);
    EXPECT_EQ(v.Count(), 5);
    EXPECT_EQ(v[3], 0);
    EXPECT_EQ(v[4], 0);
}

TEST(Vector, ReserveDoesNotChangeCount) {
    Vector<int> v;
    v.Reserve(100);
    EXPECT_EQ(v.Count(), 0);
    EXPECT_GE(v.Capacity(), 100);
}

TEST(Vector, EqualityComparesElementwise) {
    Vector<int> a = { 1, 2, 3 };
    Vector<int> b = { 1, 2, 3 };
    Vector<int> c = { 1, 2, 4 };
    Vector<int> d = { 1, 2 };

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a == d);
    EXPECT_TRUE(a != c);
}

TEST(Vector, ForEachVisitsAllElements) {
    Vector<int> v = { 1, 2, 3, 4 };
    int sum = 0;
    v.ForEach([&](const int& x) { sum += x; });
    EXPECT_EQ(sum, 10);
}

TEST(Vector, ForEachMutates) {
    Vector<int> v = { 1, 2, 3 };
    v.ForEach([](int& x) { x *= 2; });
    EXPECT_EQ(v[0], 2);
    EXPECT_EQ(v[1], 4);
    EXPECT_EQ(v[2], 6);
}

TEST(Vector, ConvertProducesNewTypeVector) {
    Vector<int> v = { 1, 2, 3 };
    Vector<float> f = v.Convert<float>([](const int& x) { return (float)x * 0.5f; });
    ASSERT_EQ(f.Count(), 3);
    EXPECT_FLOAT_EQ(f[0], 0.5f);
    EXPECT_FLOAT_EQ(f[2], 1.5f);
}

TEST(Vector, FindAllFiltersElements) {
    Vector<int> v = { 1, 2, 3, 4, 5, 6 };
    Vector<int> evens = v.FindAll([](const int& x) { return x % 2 == 0; });
    EXPECT_EQ(evens.Count(), 3);
    EXPECT_TRUE(evens.Contains(2));
    EXPECT_TRUE(evens.Contains(4));
    EXPECT_TRUE(evens.Contains(6));
}

TEST(Vector, AnyAllPredicates) {
    Vector<int> v = { 2, 4, 6 };
    EXPECT_TRUE(v.All([](const int& x) { return x % 2 == 0; }));
    EXPECT_FALSE(v.All([](const int& x) { return x > 3; }));
    EXPECT_TRUE(v.Any([](const int& x) { return x > 5; }));
    EXPECT_FALSE(v.Any([](const int& x) { return x > 100; }));
}

TEST(Vector, SortAscending) {
    Vector<int> v = { 3, 1, 4, 1, 5, 9, 2, 6 };
    v.Sort([](const int& a, const int& b) { return a < b; });
    for (int i = 1; i < v.Count(); ++i)
        EXPECT_LE(v[i - 1], v[i]);
}

TEST(Vector, SortWithCustomPredicate) {
    Vector<int> v = { 3, 1, 4, 1, 5, 9, 2, 6 };
    v.Sort([](const int& a, const int& b) { return a > b; });
    for (int i = 1; i < v.Count(); ++i)
        EXPECT_GE(v[i - 1], v[i]);
}

TEST(Vector, ReverseOrder) {
    Vector<int> v = { 1, 2, 3, 4 };
    v.Reverse();
    EXPECT_EQ(v[0], 4);
    EXPECT_EQ(v[3], 1);
}

TEST(Vector, FirstAndLast) {
    Vector<int> v = { 10, 20, 30 };
    EXPECT_EQ(v.First(), 10);
    EXPECT_EQ(v.Last(), 30);
}

TEST(Vector, RemoveAllByPredicate) {
    Vector<int> v = { 1, 2, 3, 4, 5 };
    v.RemoveAll([](const int& x) { return x % 2 == 0; });
    EXPECT_EQ(v.Count(), 3);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 5);
}

TEST(Vector, ClearEmptiesContainer) {
    Vector<int> v = { 1, 2, 3 };
    v.Clear();
    EXPECT_EQ(v.Count(), 0);
    EXPECT_TRUE(v.IsEmpty());
}

TEST(Vector, MoveConstructorTransfersData) {
    Vector<int> a = { 1, 2, 3, 4, 5 };
    Vector<int> b(std::move(a));

    EXPECT_EQ(b.Count(), 5);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[4], 5);
}

TEST(Vector, TakeReturnsPrefix) {
    Vector<int> v = { 1, 2, 3, 4, 5 };
    Vector<int> first3 = v.Take(3);
    EXPECT_EQ(first3.Count(), 3);
    EXPECT_EQ(first3[0], 1);
    EXPECT_EQ(first3[2], 3);

    Vector<int> middle = v.Take(1, 4);
    EXPECT_EQ(middle.Count(), 3);
    EXPECT_EQ(middle[0], 2);
    EXPECT_EQ(middle[2], 4);
}
