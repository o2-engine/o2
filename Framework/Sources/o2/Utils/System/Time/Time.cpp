#include "o2/stdafx.h"
#include "Time.h"

#include "o2/Utils/Reflection/Reflection.h"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace o2
{
	//TimeStuff& Time = TimeStuff::Instance();

	DECLARE_SINGLETON(Time);

	Time::Time():
		mApplicationTime(0), mLocalTime(0), mCurrentFrame(1), mDeltaTime(0), mFPS(0), mFPSSum(0),
		mFramesSum(0), mLastFPSCheckingTime(0)
	{}

	Time::~Time()
	{}

	void Time::Update(float dt)
	{
		mCurrentFrame++;
		mFramesSum += 1.0f;

		if (dt < FLT_EPSILON)
			return;

		mDeltaTime = dt;
		mApplicationTime += dt;
		mLocalTime += dt;

		mFPSSum += 1.0f / dt;
		if (mApplicationTime - mLastFPSCheckingTime > 0.3f)
		{
			mFPS = mFramesSum / (mApplicationTime - mLastFPSCheckingTime);
			mLastFPSCheckingTime = mApplicationTime;
			mFPSSum = 0;
			mFramesSum = 0;
		}
	}

	float Time::GetApplicationTime() const
	{
		return mInstance->mApplicationTime;
	}

	float Time::GetLocalTime() const
	{
		return mInstance->mLocalTime;
	}

	void Time::ResetLocalTime() const
	{
		mInstance->mLocalTime = 0;
	}

	void Time::SetLocalTime(float time) const
	{
		mInstance->mLocalTime = time;
	}

	int Time::GetCurrentFrame() const
	{
		return (int)mInstance->mCurrentFrame;
	}

	float Time::GetDeltaTime() const
	{
		return mInstance->mDeltaTime;
	}

	float Time::GetFPS() const
	{
		return mInstance->mFPS;
	}

	TimeStamp Time::CurrentTime() const
	{
#ifdef PLATFORM_WINDOWS
		SYSTEMTIME tm;
		GetSystemTime(&tm);

		return TimeStamp(tm.wSecond, tm.wMinute, tm.wHour, tm.wDay, tm.wMonth, tm.wYear);
#elif defined PLATFORM_ANDROID
        return TimeStamp();
#elif defined PLATFORM_MAC
		return TimeStamp();
#elif defined PLATFORM_IOS
		return TimeStamp();
#endif
	}

	bool TimeStamp::operator!=(const TimeStamp& wt) const
	{
		return !(*this == wt);
	}

	bool TimeStamp::operator==(const TimeStamp& wt) const
	{
		return mSecond == wt.mSecond && mMinute == wt.mMinute && mHour == wt.mHour && mDay == wt.mDay && mMonth == wt.mMonth &&
			mYear == wt.mYear;
	}

	TimeStamp::TimeStamp(int seconds /*= 0*/, int minutes /*= 0*/, int hours /*= 0*/, int days /*= 0*/, int months /*= 0*/, int years /*= 0*/):
		mYear(years), mMonth(months), mDay(days), mHour(hours), mMinute(minutes), mSecond(seconds)
	{}

}
