#include "o2/stdafx.h"
#include "File.h"

#include "o2/Utils/Reflection/Reflection.h"

#ifdef PLATFORM_ANDROID
#include "o2/Application/Android/AndroidPlatform.h"
#endif

#ifdef PLATFORM_WASM
#include "o2/Utils/FileSystem/WebAssembly/WebServerFS.h"
#endif

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

#ifdef PLATFORM_ANDROID
        // APK assets live inside the zip — only AAssetManager can read them.
        // Absolute paths (starting with '/') are regular FS (DataPath, cache).
        if (!filename.IsEmpty() && filename[0] != '/')
        {
            if (AAssetManager* am = AndroidPlatform::GetAssetManager())
            {
                mAsset = AAssetManager_open(am, filename.Data(), AASSET_MODE_BUFFER);
                if (mAsset)
                {
                    mOpened = true;
                    mFilename = filename;
                    return true;
                }
            }
        }
#endif

        mIfstream.open(filename, std::ios::binary);

        if (!mIfstream.is_open())
            return false;

        mOpened = true;
        mFilename = filename;

        return true;
    }

    bool InFile::Close()
    {
        if (mOpened)
        {
#ifdef PLATFORM_ANDROID
            if (mAsset)
            {
                AAsset_close(mAsset);
                mAsset = nullptr;
            }
            else
#endif
            {
                mIfstream.close();
            }
        }
        mOpened = false;
        return true;
    }

    UInt InFile::ReadFullData(void* dataPtr)
    {
#ifdef PLATFORM_ANDROID
        if (mAsset)
        {
            AAsset_seek(mAsset, 0, SEEK_SET);
            off_t length = AAsset_getLength(mAsset);
            AAsset_read(mAsset, dataPtr, length);
            return (UInt)length;
        }
#endif
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
#ifdef PLATFORM_ANDROID
        if (mAsset)
        {
            AAsset_read(mAsset, dataPtr, bytes);
            return;
        }
#endif
        auto& r = mIfstream.read((char*)dataPtr, bytes);
    }

    void InFile::SetCaretPos(UInt pos)
    {
#ifdef PLATFORM_ANDROID
        if (mAsset)
        {
            AAsset_seek(mAsset, (off_t)pos, SEEK_SET);
            return;
        }
#endif
        mIfstream.seekg(pos, std::ios::beg);
    }

    UInt InFile::GetCaretPos()
    {
#ifdef PLATFORM_ANDROID
        if (mAsset)
        {
            off_t remaining = AAsset_getRemainingLength(mAsset);
            off_t total     = AAsset_getLength(mAsset);
            return (UInt)(total - remaining);
        }
#endif
        return (UInt)mIfstream.tellg();
    }

    UInt InFile::GetDataSize()
    {
#ifdef PLATFORM_ANDROID
        if (mAsset)
            return (UInt)AAsset_getLength(mAsset);
#endif
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
        {
            mOfstream.close();
            mOpened = false;

#ifdef PLATFORM_WASM
            WebFS::NotifyFileWritten(mFilename);
#endif
        }

        return true;
    }

    void OutFile::WriteData(const void* dataPtr, UInt bytes)
    {
        mOfstream.write((const char*)dataPtr, bytes);
    }
}
