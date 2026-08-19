#include "o2/stdafx.h"
#include "FileSystem.h"

#include "o2/Application/Application.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"

#include <chrono>

#ifdef PLATFORM_ANDROID
#include "o2/Application/Android/AndroidPlatform.h"
#endif

#ifdef PLATFORM_WASM
#include "o2/Utils/FileSystem/WebAssembly/WebServerFS.h"
#define O2_WEBFS_MIRROR(call) WebFS::call
#else
#define O2_WEBFS_MIRROR(call)
#endif

#if defined(O2_FILESYSTEM_EXPERIMENTAL)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

namespace o2
{

    FileSystem::FileSystem(RefCounter* refCounter):
        Singleton<FileSystem>(refCounter)
    {
        mLog = mmake<LogStream>("File System");
        o2Debug.GetLog()->BindStream(mLog);
    }

    FileSystem::~FileSystem()
    {}

    String FileSystem::ExtractPathStr(const String& path) const
    {
        auto fnd = path.FindLast("/");
        if (fnd < 0)
            return "";

        return path.SubStr(0, fnd);
    }

    String FileSystem::GetFileExtension(const String& filePath)
    {
        int dotIdx = filePath.FindLast(".");

        if (dotIdx != -1)
            return filePath.SubStr(dotIdx + 1);

        return String();
    }

    String FileSystem::GetFileNameWithoutExtension(const String& filePath)
    {
        return filePath.SubStr(0, filePath.FindLast("."));
    }

    String FileSystem::GetPathWithoutDirectories(const String& path)
    {
        return path.SubStr(Math::Max(path.FindLast("/"), path.FindLast("\\")) + 1);
    }

    String FileSystem::GetParentPath(const String& path)
    {
        int idx = Math::Max(path.FindLast("/"), path.FindLast("\\"));

        if (idx == -1)
            return String();

        return path.SubStr(0, idx);
    }

    String FileSystem::ReadFile(const String& path)
    {
        InFile file(path);
        if (!file.IsOpened())
            return String();

        return file.ReadFullData();
    }

    void FileSystem::WriteFile(const String& path, const String& data)
    {
        OutFile file(path);
        file.WriteData(data.Data(), data.Length());
    }

    FolderInfo FileSystem::GetFolderInfo(const String& path) const
    {
        FolderInfo res;
        res.path = path;

        fs::path fullPath(path.Data());
        if (!fs::exists(fullPath))
        {
            mInstance->mLog->Error("Failed GetPathInfo: Error opening directory " + path);
            return res;
        }

        for (auto& subPath : fs::directory_iterator(fullPath))
        {
            if (fs::is_directory(subPath))
                res.folders.Add(GetFolderInfo(subPath.path().string() + "/"));
            else
                res.files.Add(GetFileInfo(subPath.path().string()));
        }

        return res;
    }

    bool FileSystem::FileCopy(const String& source, const String& dest) const
    {
        FileDelete(dest);
        FolderCreate(ExtractPathStr(dest));

        fs::copy(source.Data(), dest.Data());

        O2_WEBFS_MIRROR(NotifyFileCopied(source, dest));

        return fs::exists(dest.Data());
    }

    bool FileSystem::FileDelete(const String& file) const
    {
        if (!fs::exists(file.Data()))
            return false;

        fs::remove(file.Data());

        O2_WEBFS_MIRROR(NotifyFileDeleted(file));

        return true;
    }

    bool FileSystem::FileMove(const String& source, const String& dest) const
    {
        String destFolder = GetParentPath(dest);

        if (!IsFolderExist(destFolder))
            FolderCreate(destFolder);

        fs::rename(source.Data(), dest.Data());

        O2_WEBFS_MIRROR(NotifyMoved(source, dest));

        return fs::exists(dest.Data());
    }

    FileInfo FileSystem::GetFileInfo(const String& path) const
    {
        using namespace std::chrono;

        FileInfo res;
        res.path = "invalid_file";

        fs::path fullPath(path.Data());

        if (!fs::exists(fullPath))
            return res;

        auto writeTime = fs::last_write_time(fullPath);
        auto timePoint = time_point_cast<system_clock::time_point::duration>(writeTime - fs::file_time_type::clock::now() + system_clock::now());
        std::time_t time = system_clock::to_time_t(timePoint);

        if (std::tm* timeInfo = std::localtime(&time))
        {
            res.editDate = TimeStamp(timeInfo->tm_sec, timeInfo->tm_min, timeInfo->tm_hour,
                                     timeInfo->tm_mday, timeInfo->tm_mon + 1, timeInfo->tm_year + 1900);
        }

        res.path = path;
        res.size = fs::file_size(fullPath);

        return res;
    }

    bool FileSystem::SetFileEditDate(const String& path, const TimeStamp& time) const
    {
        using namespace std::chrono;

        std::tm file_tm = {};
        file_tm.tm_year = time.mYear - 1900;
        file_tm.tm_mon = time.mMonth - 1;
        file_tm.tm_mday = time.mDay;
        file_tm.tm_hour = time.mHour;
        file_tm.tm_min = time.mMinute;
        file_tm.tm_sec = time.mSecond;

        std::time_t writeTime = std::mktime(&file_tm);
        auto timePoint = system_clock::from_time_t(writeTime);

        auto fileTimePoint = time_point_cast<fs::file_time_type::clock::time_point::duration>(timePoint - system_clock::now() + fs::file_time_type::clock::now());

        fs::last_write_time(path.Data(), fileTimePoint);

        O2_WEBFS_MIRROR(NotifyEditDateSet(path, (long long)writeTime));

        return true;
    }

    bool FileSystem::FolderCreate(const String& path, bool recursive /*= true*/) const
    {
        if (IsFolderExist(path))
            return true;

        if (!recursive)
            return fs::create_directory(path.Data());

        String extrPath = ExtractPathStr(path);
        if (extrPath == path)
            return false;

        // A single-segment relative path has no parent to create
        if (!extrPath.IsEmpty() && !FolderCreate(extrPath, true))
            return false;

        bool created = fs::create_directory(path.Data());

        if (created)
            O2_WEBFS_MIRROR(NotifyFolderCreated(path));

        return created;
    }

    bool FileSystem::FolderCopy(const String& from, const String& to) const
    {
        if (!IsFolderExist(from) || !IsFolderExist(to))
            return false;

        fs::copy(from.Data(), to.Data(), fs::copy_options::recursive);

        O2_WEBFS_MIRROR(NotifyFolderCopied(from, to));

        return true;
    }

    bool FileSystem::FolderRemove(const String& path, bool recursive /*= true*/) const
    {
        if (!IsFolderExist(path))
            return false;

        fs::remove_all(path.Data());

        O2_WEBFS_MIRROR(NotifyFolderRemoved(path));

        return true;
    }

    bool FileSystem::Rename(const String& old, const String& newPath) const
    {
        fs::rename(old.Data(), newPath.Data());

        O2_WEBFS_MIRROR(NotifyMoved(old, newPath));

        return true;
    }

    bool FileSystem::IsFolderExist(const String& path) const
    {
        return fs::exists(path.Data());
    }

    bool FileSystem::IsFileExist(const String& path) const
    {
#ifdef PLATFORM_ANDROID
        // Relative paths live in APK assets; only AAssetManager can see them.
        if (!path.IsEmpty() && path[0] != '/')
        {
            if (AAssetManager* am = AndroidPlatform::GetAssetManager())
            {
                AAsset* a = AAssetManager_open(am, path.Data(), AASSET_MODE_UNKNOWN);
                if (a) { AAsset_close(a); return true; }
                return false;
            }
        }
#endif
        return fs::exists(path.Data());
    }

    String FileSystem::GetPathRelativeToPath(const String& from, const String& to)
    {
#if defined(O2_FILESYSTEM_EXPERIMENTAL)
        return to;
#else
        return fs::relative(from.Data(), to.Data()).string();
#endif
    }

    String FileSystem::CanonicalizePath(const String& path)
    {
        return fs::canonical(path.Data()).string();
    }
}
