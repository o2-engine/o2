#pragma once

// @CODETOOLIGNORE: the profiler classes are not reflected, and the whole feature is compiled
// out by O2_PROFILER, which generated registrations would not follow
#include "o2/Utils/Types/Containers/Vector.h"

#include <algorithm>

namespace o2
{
    // ------------------------------------------------------------------------------------------------------
    // Fixed size ring buffer of samples with percentile queries. The sorted copy used by the percentiles is
    // rebuilt lazily, so pushing a sample per frame costs one store and reading several percentiles per frame
    // costs one sort
    // ------------------------------------------------------------------------------------------------------
    template<typename _type>
    class TimeSeries
    {
    public:
        // Constructor, allocates the given samples count and fills it with zeroes
        explicit TimeSeries(int samplesCount);

        // Pushes a sample, overwriting the oldest one, and returns the overwritten value
        _type Push(_type value);

        // Sets every sample to the value
        void Fill(_type value);

        // Returns the samples count
        int GetSamplesCount() const { return mValues.Count(); }

        // Returns the samples buffer, ordered from the ring head, use GetHead() to walk it chronologically
        const _type* GetSamples() const { return mValues.data(); }

        // Returns the ring head: the index of the oldest sample
        int GetHead() const { return mHead; }

        // Returns the last pushed sample
        _type GetLastSample() const;

        // Returns the minimal sample
        _type Min() const { return Percentile(0); }

        // Returns the maximal sample
        _type Max() const { return Percentile(100); }

        // Returns the median sample
        _type Median() const { return Percentile(50); }

        // Returns the sample at the percentile, 0 is the minimum and 100 is the maximum
        _type Percentile(int percentile) const;

    protected:
        Vector<_type>         mValues;       // Ring buffer of samples
        mutable Vector<_type> mSortedValues; // Samples sorted ascending, rebuilt by Validate

        int          mHead = 0;      // Ring head: index of the oldest sample
        mutable bool mSorted = true; // Is mSortedValues up to date

    protected:
        // Rebuilds the sorted copy when it is out of date
        void Validate() const;
    };

    template<typename _type>
    TimeSeries<_type>::TimeSeries(int samplesCount)
    {
        mValues.Resize(Math::Max(samplesCount, 1));
        mSortedValues.Resize(mValues.Count());

        Fill((_type)0);
    }

    template<typename _type>
    _type TimeSeries<_type>::Push(_type value)
    {
        const _type overwritten = mValues[mHead];

        mValues[mHead] = value;
        mHead = (mHead + 1)%mValues.Count();
        mSorted = false;

        return overwritten;
    }

    template<typename _type>
    void TimeSeries<_type>::Fill(_type value)
    {
        for (auto& sample : mValues)
            sample = value;

        mSorted = false;
    }

    template<typename _type>
    _type TimeSeries<_type>::GetLastSample() const
    {
        return mValues[(mHead + mValues.Count() - 1)%mValues.Count()];
    }

    template<typename _type>
    _type TimeSeries<_type>::Percentile(int percentile) const
    {
        Validate();
        return mSortedValues[Math::Clamp(percentile*(mSortedValues.Count() - 1)/100, 0, mSortedValues.Count() - 1)];
    }

    template<typename _type>
    void TimeSeries<_type>::Validate() const
    {
        if (mSorted)
            return;

        std::copy(mValues.begin(), mValues.end(), mSortedValues.begin());
        std::sort(mSortedValues.begin(), mSortedValues.end());

        mSorted = true;
    }
}
