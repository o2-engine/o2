#include "o2/stdafx.h"
#include "FileLogStream.h"

#include <fstream>

namespace o2
{
    FileLogStream::FileLogStream(const String& fileName):
        LogStream(), mFileName(fileName)
    {}

    FileLogStream::FileLogStream(const WString& id, const String& fileName):
        LogStream(id), mFileName(fileName)
    {}

    FileLogStream::~FileLogStream()
    {
        if (mStream)
            mStream.close();
    }

    void FileLogStream::SetFileName(const String& fileName)
    {
        if (mStream.is_open())
            mStream.close();

        mFileName = fileName;
    }

    void FileLogStream::EnsureOpen()
    {
        if (mStream.is_open() || mFileName.IsEmpty())
            return;

        mStream.open(mFileName, std::ios::out);
        Assert(mStream, "Can't open file for logging");
    }

    void FileLogStream::OutStrEx(const WString& str)
    {
        EnsureOpen();
        if (mStream)
            mStream << (String)str << std::endl;
    }
}
