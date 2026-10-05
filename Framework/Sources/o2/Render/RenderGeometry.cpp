#include "o2/stdafx.h"
#include "RenderGeometry.h"

#include <atomic>

namespace o2
{
    namespace
    {
        UInt64 NextVersion()
        {
            static std::atomic<UInt64> lastVersion{ 0 };
            return ++lastVersion;
        }
    }

    RenderGeometry::RenderGeometry():
        mVersion(NextVersion())
    {}

    void RenderGeometry::OnChanged()
    {
        mVersion = NextVersion();
    }

    UInt64 RenderGeometry::GetVersion() const
    {
        return mVersion;
    }

    bool RenderGeometry::IsQueued() const
    {
        return mQueuedCount > 0;
    }
}
