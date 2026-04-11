#include "o2/stdafx.h"
#include "File.h"

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Reflection/Reflection.h"

namespace o2
{
    InFile::InFile() :
        mOpened(false)
    {}

    InFile::InFile(const String& filename) :
        mOpened(false)
    {
        Open(filename);
    }

    InFile::~InFile()
    {
        Close();
    }

    const String& InFile::GetFilename() const
    {
        return mFilename;
    }

    bool InFile::IsOpened() const
    {
        return mOpened;
    }


    bool InFile::Open(const String& filename)
    {
        Close();

        String actualFilename = FileSystem::GetPathForReading(filename);
        mIfstream.open(actualFilename, std::ios::binary);

        if (!mIfstream.is_open())
            return false;

        mOpened = true;
        mFilename = actualFilename;

        return true;
    }

    bool InFile::Close()
    {
        if (mOpened)
            mIfstream.close();

        return true;
    }

    UInt InFile::ReadFullData(void* dataPtr)
    {
        mIfstream.seekg(0, std::ios::beg);
        mIfstream.seekg(0, std::ios::end);
        UInt length = (UInt)mIfstream.tellg();
        mIfstream.seekg(0, std::ios::beg);

        mIfstream.read((char*)dataPtr, length);

        return length;
    }

    String InFile::ReadFullData()
    {
        UInt len = GetDataSize();
        char* buffer = mnew char[len + 1];

        ReadData(buffer, len);
        buffer[len] = '\0';

        return String(buffer);
    }

    void InFile::ReadData(void* dataPtr, UInt bytes)
    {
        auto& r = mIfstream.read((char*)dataPtr, bytes);
    }

    void InFile::SetCaretPos(UInt pos)
    {
        mIfstream.seekg(pos, std::ios::beg);
    }

    UInt InFile::GetCaretPos()
    {
        return (UInt)mIfstream.tellg();
    }

    UInt InFile::GetDataSize()
    {
        mIfstream.seekg(0, std::ios::beg);
        mIfstream.seekg(0, std::ios::end);
        UInt res = (long unsigned int)mIfstream.tellg();
        mIfstream.seekg(0, std::ios::beg);

        return res;
    }

    OutFile::OutFile() :
        mOpened(false)
    {}

    OutFile::OutFile(const String& filename) :
        mOpened(false)
    {
        Open(filename);
    }

    OutFile::~OutFile()
    {
        Close();
    }

    const String& OutFile::GetFilename() const
    {
        return mFilename;
    }

    bool OutFile::IsOpened() const
    {
        return mOpened;
    }

    bool OutFile::Open(const String& filename)
    {
        Close();

        mOfstream.open(filename, std::ios::binary);

        if (!mOfstream.is_open())
            return false;

        mOpened = true;
        mFilename = filename;

        return true;
    }

    bool OutFile::Close()
    {
        if (mOpened)
            mOfstream.close();

        return true;
    }

    void OutFile::WriteData(const void* dataPtr, UInt bytes)
    {
        mOfstream.write((const char*)dataPtr, bytes);
    }
}
