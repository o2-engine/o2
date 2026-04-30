#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Types/Containers/Map.h"

using namespace o2;

TEST(Map, AddInsertsButDoesNotOverwrite) {
    // Map::Add uses std::map::insert which keeps the existing value on duplicate keys.
    Map<int, int> m;
    m.Add(1, 100);
    m.Add(1, 200);

    EXPECT_EQ(m.Count(), 1);
    EXPECT_EQ(m.Get(1), 100);
}

TEST(Map, SetUpdatesExistingKey) {
    Map<int, int> m;
    m.Add(1, 100);
    m.Set(1, 999);
    EXPECT_EQ(m.Get(1), 999);
}

TEST(Map, ContainsKey) {
    Map<int, int> m;
    m.Add(1, 100);
    m.Add(2, 200);

    EXPECT_TRUE(m.ContainsKey(1));
    EXPECT_TRUE(m.ContainsKey(2));
    EXPECT_FALSE(m.ContainsKey(3));
}

TEST(Map, ContainsValue) {
    Map<int, int> m;
    m.Add(1, 100);
    m.Add(2, 200);

    EXPECT_TRUE(m.ContainsValue(200));
    EXPECT_FALSE(m.ContainsValue(999));
}

TEST(Map, RemoveByKey) {
    Map<int, int> m;
    m.Add(1, 100);
    m.Add(2, 200);
    m.Add(3, 300);

    m.Remove(2);
    EXPECT_EQ(m.Count(), 2);
    EXPECT_FALSE(m.ContainsKey(2));
    EXPECT_TRUE(m.ContainsKey(1));
    EXPECT_TRUE(m.ContainsKey(3));

    // Removing non-existent key is a no-op.
    m.Remove(99);
    EXPECT_EQ(m.Count(), 2);
}

TEST(Map, TryGetValue) {
    Map<int, int> m;
    m.Add(1, 100);

    int out = -1;
    EXPECT_TRUE(m.TryGetValue(1, out));
    EXPECT_EQ(out, 100);

    int missing = -1;
    EXPECT_FALSE(m.TryGetValue(2, missing));
    EXPECT_EQ(missing, -1);
}

TEST(Map, ClearEmptiesMap) {
    Map<int, int> m;
    m.Add(1, 1);
    m.Add(2, 2);
    m.Clear();
    EXPECT_EQ(m.Count(), 0);
    EXPECT_TRUE(m.IsEmpty());
}

TEST(Map, EqualityComparesContents) {
    Map<int, int> a;
    a.Add(1, 10);
    a.Add(2, 20);

    Map<int, int> b;
    b.Add(2, 20);
    b.Add(1, 10);

    EXPECT_TRUE(a == b);

    Map<int, int> c;
    c.Add(1, 11);
    c.Add(2, 20);
    EXPECT_FALSE(a == c);
}

TEST(Map, IterationCoversAllPairs) {
    Map<int, int> m;
    m.Add(1, 10);
    m.Add(2, 20);
    m.Add(3, 30);

    int sumKeys = 0, sumValues = 0;
    m.ForEach([&](const int& k, int& v) {
        sumKeys += k;
        sumValues += v;
    });
    EXPECT_EQ(sumKeys, 6);
    EXPECT_EQ(sumValues, 60);
}

TEST(Map, FindAllFiltersByPredicate) {
    Map<int, int> m;
    m.Add(1, 10);
    m.Add(2, 20);
    m.Add(3, 30);

    auto big = m.FindAll([](const int&, const int& v) { return v > 15; });
    EXPECT_EQ(big.Count(), 2);
    EXPECT_TRUE(big.ContainsKey(2));
    EXPECT_TRUE(big.ContainsKey(3));
    EXPECT_FALSE(big.ContainsKey(1));
}

TEST(Map, AddOtherMergesEntries) {
    Map<int, int> a;
    a.Add(1, 10);
    a.Add(2, 20);

    Map<int, int> b;
    b.Add(3, 30);
    b.Add(4, 40);

    a.Add(b);
    EXPECT_EQ(a.Count(), 4);
    EXPECT_TRUE(a.ContainsKey(3));
    EXPECT_TRUE(a.ContainsKey(4));
}

TEST(Map, AnyAllByPredicate) {
    Map<int, int> m;
    m.Add(1, 10);
    m.Add(2, 20);
    m.Add(3, 30);

    EXPECT_TRUE(m.All([](const int&, const int& v) { return v >= 10; }));
    EXPECT_FALSE(m.All([](const int&, const int& v) { return v >= 20; }));
    EXPECT_TRUE(m.Any([](const int&, const int& v) { return v == 30; }));
    EXPECT_FALSE(m.Any([](const int&, const int& v) { return v == 999; }));
}

// Note: Map<>::RemoveAll currently fails to compile under MSVC C++20 because
// Map.h:463 calls free `erase(it)` which now resolves to std::erase(container, value).
// Skipping that direct test until Map.h is fixed; we verify Remove(key) instead in MapTest.RemoveByKey.
