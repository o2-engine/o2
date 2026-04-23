#include "o2/stdafx.h"
#include "LogStream.h"

#include <cstdarg>

namespace o2
{
    LogStream::LogStream()
    {}

    LogStream::LogStream(const WString& id) :
        mId(id)
    {}

	LogStream::LogStream(RefCounter* refCounter):
		RefCounterable(refCounter)
	{}

	LogStream::~LogStream()
    {
        if (mParentStream)
            mParentStream.Lock()->UnbindStream(Ref(this));

        UnbindAllStreams();
    }

    const WString& LogStream::GetId() const
    {
        return mId;
    }

    void LogStream::BindStream(const Ref<LogStream>& stream)
    {
        stream->mParentStream = WeakRef(this);
        mChildStreams.Add(stream);
    }

    void LogStream::UnbindStream(const Ref<LogStream>& stream)
    {
        if (!stream)
			return;

		stream->mParentStream = nullptr;

        mChildStreams.Remove(stream);
    }

    void LogStream::UnbindAndReleaseStream(const Ref<LogStream>& stream)
    {
        mChildStreams.Remove(stream);
    }

    void LogStream::UnbindAllStreams()
    {
        for (auto& stream : mChildStreams)
			stream->mParentStream = nullptr;

        mChildStreams.Clear();
    }

    void LogStream::Out(WString format, ...)
    {
        va_list vlist;
        va_start(vlist, format);

        OutStr(WString::Format(format, vlist));

        va_end(vlist);
    }

    void LogStream::Error(WString format, ...)
    {
        va_list vlist;
        va_start(vlist, format);

        ErrorStr(WString::Format(format, vlist));

        va_end(vlist);
    }

    void LogStream::Warning(WString format, ...)
    {
        va_list vlist;
        va_start(vlist, format);

        WarningStr(WString::Format(format, vlist));

        va_end(vlist);
    }

    const WeakRef<LogStream>& LogStream::GetParentStream() const
    {
        return mParentStream;
    }

    void LogStream::OutStr(const WString& str)
    {
        OutStrEx(str);

        if (mParentStream)
        {
            if (mId.IsEmpty())
                mParentStream.Lock()->OutStr(str);
            else
                mParentStream.Lock()->OutStr((mId + ":" + str));
        }
    }

    void LogStream::ErrorStr(const WString& str)
    {
        OutErrorEx(str);

        if (mParentStream)
        {
            if (mId == "")
                mParentStream.Lock()->ErrorStr(str);
            else
                mParentStream.Lock()->ErrorStr((mId + ":" + str));
        }
    }

    void LogStream::WarningStr(const WString& str)
    {
        OutWarningEx(str);

        if (mParentStream)
        {
            if (mId == "")
                mParentStream.Lock()->WarningStr(str);
            else
                mParentStream.Lock()->WarningStr((mId + ":" + str));
        }
    }

    void LogStream::OutErrorEx(const WString& str)
    {
        OutStrEx("ERROR:" + str);

        if (IsStoppingOnLogErrors())
            Assert(false, (const char*)((String)str));
    }

    void LogStream::OutWarningEx(const WString& str)
    {
        OutStrEx("WARNING:" + str);
    }

}
