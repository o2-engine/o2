#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace
{
    class CapturingLogStream: public LogStream
    {
    public:
        Vector<WString> messages;

        CapturingLogStream() = default;
        CapturingLogStream(const WString& id): LogStream(id) {}

    protected:
        void OutStrEx(const WString& str) override { messages.Add(str); }
    };
}

TEST(LogStream, OutStrCapturedByDerivedStream)
{
    auto s = mmake<CapturingLogStream>();
    s->OutStr("hello");

    ASSERT_EQ(s->messages.Count(), 1);
    EXPECT_EQ(s->messages[0], WString("hello"));
}

TEST(LogStream, ErrorStrPrependsErrorPrefix)
{
    auto s = mmake<CapturingLogStream>();
    s->ErrorStr("oops");

    ASSERT_EQ(s->messages.Count(), 1);
    EXPECT_EQ(s->messages[0], WString("ERROR:oops"));
}

TEST(LogStream, WarningStrPrependsWarningPrefix)
{
    auto s = mmake<CapturingLogStream>();
    s->WarningStr("careful");

    ASSERT_EQ(s->messages.Count(), 1);
    EXPECT_EQ(s->messages[0], WString("WARNING:careful"));
}

TEST(LogStream, IdStoredAndReturned)
{
    auto s = mmake<CapturingLogStream>(WString("MyStream"));
    EXPECT_EQ(s->GetId(), WString("MyStream"));
}

TEST(LogStream, ChildOutPropagatesToParent)
{
    auto parent = mmake<CapturingLogStream>();
    auto child = mmake<CapturingLogStream>();

    parent->BindStream(child);

    child->OutStr("from-child");

    ASSERT_EQ(child->messages.Count(), 1);
    ASSERT_EQ(parent->messages.Count(), 1);
    EXPECT_EQ(parent->messages[0], WString("from-child"));
}

TEST(LogStream, ChildWithIdPrefixesParentMessage)
{
    auto parent = mmake<CapturingLogStream>();
    auto child = mmake<CapturingLogStream>(WString("Child"));

    parent->BindStream(child);
    child->OutStr("msg");

    ASSERT_EQ(parent->messages.Count(), 1);
    EXPECT_EQ(parent->messages[0], WString("Child:msg"));
}

TEST(LogStream, BindUpdatesParentReference)
{
    auto parent = mmake<CapturingLogStream>();
    auto child = mmake<CapturingLogStream>();

    EXPECT_FALSE(child->GetParentStream());

    parent->BindStream(child);

    auto locked = child->GetParentStream().Lock();
    EXPECT_EQ(locked, DynamicCast<LogStream>(parent));
}

TEST(LogStream, UnbindStreamDetachesChild)
{
    auto parent = mmake<CapturingLogStream>();
    auto child = mmake<CapturingLogStream>();

    parent->BindStream(child);
    parent->UnbindStream(child);

    EXPECT_FALSE(child->GetParentStream());

    child->OutStr("after-unbind");
    EXPECT_EQ(parent->messages.Count(), 0);
}

TEST(LogStream, UnbindAllStreamsDetachesEveryChild)
{
    auto parent = mmake<CapturingLogStream>();
    auto a = mmake<CapturingLogStream>();
    auto b = mmake<CapturingLogStream>();

    parent->BindStream(a);
    parent->BindStream(b);

    parent->UnbindAllStreams();

    EXPECT_FALSE(a->GetParentStream());
    EXPECT_FALSE(b->GetParentStream());
}

TEST(LogStream, ErrorPropagatesAsErrorToParent)
{
    auto parent = mmake<CapturingLogStream>();
    auto child = mmake<CapturingLogStream>(WString("X"));

    parent->BindStream(child);
    child->ErrorStr("bad");

    ASSERT_EQ(parent->messages.Count(), 1);
    EXPECT_EQ(parent->messages[0], WString("ERROR:X:bad"));
}

TEST(Debug, InstanceAvailableAfterApplicationInit)
{
    EXPECT_TRUE(Debug::IsSingletonInitialzed());
    EXPECT_TRUE(o2Debug.GetLog());
}

namespace
{
    // Inserts `capture` between the main Debug log stream and its existing
    // parent so it observes everything propagated up. The Debug log stream
    // itself only forwards messages UP the parent chain, so binding a child
    // to it would never receive anything.
    class DebugLogIntercept
    {
    public:
        explicit DebugLogIntercept(const Ref<CapturingLogStream>& capture):
            mCapture(capture), mLog(o2Debug.GetLog())
        {
            mOriginalParent = mLog->GetParentStream().Lock();
            if (mOriginalParent)
            {
                mOriginalParent->UnbindStream(mLog);
                mOriginalParent->BindStream(mCapture);
            }
            mCapture->BindStream(mLog);
        }

        ~DebugLogIntercept()
        {
            mCapture->UnbindStream(mLog);
            if (mOriginalParent)
            {
                mOriginalParent->UnbindStream(mCapture);
                mOriginalParent->BindStream(mLog);
            }
        }

    private:
        Ref<CapturingLogStream> mCapture;
        Ref<LogStream>          mLog;
        Ref<LogStream>          mOriginalParent;
    };

    bool AnyMessageContains(const Vector<WString>& messages, const String& needle)
    {
        for (auto& m : messages)
        {
            if (((String)m).Contains(needle))
                return true;
        }
        return false;
    }
}

TEST(Debug, LogStrRoutesIntoMainLog)
{
    auto capture = mmake<CapturingLogStream>();
    {
        DebugLogIntercept intercept(capture);
        o2Debug.LogStr("debug-test-message");
    }

    EXPECT_TRUE(AnyMessageContains(capture->messages, "debug-test-message"));
}

TEST(Debug, LogWarningStrRoutesAsWarning)
{
    auto capture = mmake<CapturingLogStream>();
    {
        DebugLogIntercept intercept(capture);
        o2Debug.LogWarningStr("debug-test-warn");
    }

    EXPECT_TRUE(AnyMessageContains(capture->messages, "debug-test-warn"));
    EXPECT_TRUE(AnyMessageContains(capture->messages, "WARNING"));
}

TEST(Debug, LogErrorStrRoutesAsError)
{
    auto capture = mmake<CapturingLogStream>();
    {
        DebugLogIntercept intercept(capture);
        o2Debug.LogErrorStr("debug-test-err");
    }

    EXPECT_TRUE(AnyMessageContains(capture->messages, "debug-test-err"));
    EXPECT_TRUE(AnyMessageContains(capture->messages, "ERROR"));
}
