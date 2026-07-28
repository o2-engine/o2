#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Debug/Profiling/TimeSeries.h"

using namespace o2;

TEST(TimeSeries, NewSeriesIsZeroFilled)
{
    TimeSeries<double> series(8);

    EXPECT_EQ(series.GetSamplesCount(), 8);
    EXPECT_EQ(series.GetHead(), 0);
    EXPECT_DOUBLE_EQ(series.Min(), 0.0);
    EXPECT_DOUBLE_EQ(series.Max(), 0.0);
    EXPECT_DOUBLE_EQ(series.GetLastSample(), 0.0);
}

TEST(TimeSeries, EmptySeriesIsClampedToOneSample)
{
    TimeSeries<double> series(0);

    EXPECT_EQ(series.GetSamplesCount(), 1);

    series.Push(5.0);
    EXPECT_DOUBLE_EQ(series.GetLastSample(), 5.0);
}

TEST(TimeSeries, PushMovesHeadAndReturnsOverwrittenSample)
{
    TimeSeries<double> series(3);

    EXPECT_DOUBLE_EQ(series.Push(1.0), 0.0);
    EXPECT_EQ(series.GetHead(), 1);

    series.Push(2.0);
    series.Push(3.0);
    EXPECT_EQ(series.GetHead(), 0);

    // the ring wrapped, so the oldest sample comes back
    EXPECT_DOUBLE_EQ(series.Push(4.0), 1.0);
    EXPECT_DOUBLE_EQ(series.GetLastSample(), 4.0);
}

TEST(TimeSeries, SamplesAreWalkedChronologicallyFromTheHead)
{
    TimeSeries<int> series(4);
    for (int i = 1; i <= 6; i++)
        series.Push(i);

    const int* samples = series.GetSamples();
    const int head = series.GetHead();

    // the last four pushed values, oldest first
    EXPECT_EQ(samples[(0 + head)%4], 3);
    EXPECT_EQ(samples[(1 + head)%4], 4);
    EXPECT_EQ(samples[(2 + head)%4], 5);
    EXPECT_EQ(samples[(3 + head)%4], 6);
}

TEST(TimeSeries, PercentilesFollowTheSortedSamples)
{
    TimeSeries<double> series(5);
    for (double value : { 30.0, 10.0, 50.0, 20.0, 40.0 })
        series.Push(value);

    EXPECT_DOUBLE_EQ(series.Min(), 10.0);
    EXPECT_DOUBLE_EQ(series.Max(), 50.0);
    EXPECT_DOUBLE_EQ(series.Median(), 30.0);
    EXPECT_DOUBLE_EQ(series.Percentile(25), 20.0);
    EXPECT_DOUBLE_EQ(series.Percentile(75), 40.0);
}

TEST(TimeSeries, PercentileArgumentIsClamped)
{
    TimeSeries<double> series(4);
    for (double value : { 1.0, 2.0, 3.0, 4.0 })
        series.Push(value);

    EXPECT_DOUBLE_EQ(series.Percentile(-50), 1.0);
    EXPECT_DOUBLE_EQ(series.Percentile(500), 4.0);
}

TEST(TimeSeries, PercentilesSeeSamplesPushedAfterTheLastQuery)
{
    TimeSeries<double> series(3);
    series.Fill(5.0);
    EXPECT_DOUBLE_EQ(series.Max(), 5.0);

    series.Push(100.0);
    EXPECT_DOUBLE_EQ(series.Max(), 100.0);
    EXPECT_DOUBLE_EQ(series.Min(), 5.0);
}

TEST(TimeSeries, FillReplacesEverySample)
{
    TimeSeries<double> series(4);
    series.Push(1.0);
    series.Push(2.0);

    series.Fill(7.0);

    EXPECT_DOUBLE_EQ(series.Min(), 7.0);
    EXPECT_DOUBLE_EQ(series.Max(), 7.0);
    EXPECT_DOUBLE_EQ(series.GetLastSample(), 7.0);
}
