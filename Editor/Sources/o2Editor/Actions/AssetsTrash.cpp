#include "o2Editor/stdafx.h"
#include "AssetsTrash.h"

#include "o2/Application/Application.h"
#include "o2/Assets/Assets.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <sstream>

#ifdef _WIN32
#include <process.h>
#define o2_trash_getpid _getpid
#else
#include <unistd.h>
#define o2_trash_getpid getpid
#endif

using namespace o2;

namespace Editor::AssetsTrash
{
    namespace
    {
        String gLastError;
        bool   gRebuildAfterMutation = true;

        void SetError(const String& msg) { gLastError = msg; }

        bool IsAbsolutePath(const String& path)
        {
            if (path.Length() >= 2 && path[1] == ':')
                return true;
            if (!path.IsEmpty() && (path[0] == '/' || path[0] == '\\'))
                return true;
            return false;
        }

        String StripTrailingSlash(const String& path)
        {
            String r = path;
            while (!r.IsEmpty() && (r[r.Length() - 1] == '/' || r[r.Length() - 1] == '\\'))
                r = r.SubStr(0, r.Length() - 1);
            return r;
        }

        String AbsoluteAssetsRoot()
        {
            String assetsPath = o2Assets.GetAssetsPath();
            if (!IsAbsolutePath(assetsPath))
                assetsPath = o2Application.GetBinPath() + "/" + assetsPath;
            return StripTrailingSlash(assetsPath);
        }

        String AbsoluteFor(const String& assetsRelPath)
        {
            return AbsoluteAssetsRoot() + "/" + assetsRelPath;
        }

        String TrashRootNoSlash()
        {
            return o2FileSystem.GetParentPath(AbsoluteAssetsRoot()) + "/.editor-trash";
        }

        bool IsDirectory(const String& path)
        {
            std::error_code ec;
            return std::filesystem::is_directory(path.Data(), ec);
        }

        bool ExistsOnDisk(const String& path)
        {
            std::error_code ec;
            return std::filesystem::exists(path.Data(), ec);
        }

        String UniqueSubfolderName()
        {
            static std::atomic<uint64_t> counter{0};
            auto pid = (uint64_t)o2_trash_getpid();
            auto now = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::high_resolution_clock::now().time_since_epoch())
                           .count();
            auto seq = counter.fetch_add(1);
            std::ostringstream oss;
            oss << std::hex << pid << "-" << now << "-" << seq;
            return String(oss.str().c_str());
        }

        bool MoveFileOrFolder(const String& src, const String& dst)
        {
            if (IsDirectory(src))
            {
                std::filesystem::create_directories(dst.Data());
                if (!o2FileSystem.FolderCopy(src, dst))
                    return false;
                return o2FileSystem.FolderRemove(src, true);
            }
            return o2FileSystem.FileMove(src, dst);
        }
    }

    const String& LastError() { return gLastError; }

    void SetRebuildAssetsAfterMutation(bool enabled) { gRebuildAfterMutation = enabled; }
    bool ShouldRebuildAssetsAfterMutation() { return gRebuildAfterMutation; }

    void NotifyAssetsChanged()
    {
        if (gRebuildAfterMutation)
            o2Assets.RebuildAssets();
    }

    String GetRoot()
    {
        return TrashRootNoSlash() + "/";
    }

    String AbsolutePathFor(const String& assetsRelPath)
    {
        return AbsoluteFor(assetsRelPath);
    }

    String StashAsset(const String& assetsRelPath)
    {
        gLastError = "";

        String src = AbsoluteFor(assetsRelPath);
        if (!ExistsOnDisk(src))
        {
            SetError("src not found: " + src);
            return String();
        }

        String trashRoot = TrashRootNoSlash();
        std::error_code ec;
        std::filesystem::create_directories(trashRoot.Data(), ec);
        if (!o2FileSystem.IsFolderExist(trashRoot))
        {
            SetError("trashRoot not created: " + trashRoot);
            return String();
        }

        String subfolderNoSlash = trashRoot + "/" + UniqueSubfolderName();
        std::filesystem::create_directories(subfolderNoSlash.Data(), ec);
        if (!o2FileSystem.IsFolderExist(subfolderNoSlash))
        {
            SetError("subfolder not created: " + subfolderNoSlash);
            return String();
        }

        String filename = o2FileSystem.GetPathWithoutDirectories(assetsRelPath);
        String dst = subfolderNoSlash + "/" + filename;

        if (!MoveFileOrFolder(src, dst))
        {
            SetError("MoveFileOrFolder failed: " + src + " -> " + dst);
            return String();
        }

        String metaSrc = src + ".meta";
        if (o2FileSystem.IsFileExist(metaSrc))
            o2FileSystem.FileMove(metaSrc, dst + ".meta");

        return subfolderNoSlash + "/";
    }

    bool RestoreAsset(const String& trashSubfolder, const String& assetsRelPath)
    {
        gLastError = "";
        if (trashSubfolder.IsEmpty())
        {
            SetError("empty trash subfolder");
            return false;
        }

        String filename = o2FileSystem.GetPathWithoutDirectories(assetsRelPath);
        String subfolderNoSlash = StripTrailingSlash(trashSubfolder);
        String src = subfolderNoSlash + "/" + filename;
        String dst = AbsoluteFor(assetsRelPath);

        String dstParent = o2FileSystem.GetParentPath(StripTrailingSlash(dst));
        if (!dstParent.IsEmpty() && !o2FileSystem.IsFolderExist(dstParent))
            o2FileSystem.FolderCreate(dstParent, true);

        if (!MoveFileOrFolder(src, dst))
        {
            SetError("restore MoveFileOrFolder failed: " + src + " -> " + dst);
            return false;
        }

        String metaSrc = src + ".meta";
        if (o2FileSystem.IsFileExist(metaSrc))
            o2FileSystem.FileMove(metaSrc, dst + ".meta");

        o2FileSystem.FolderRemove(subfolderNoSlash, true);
        return true;
    }

    void ClearAllOnStartup()
    {
        String root = TrashRootNoSlash();
        if (o2FileSystem.IsFolderExist(root))
            o2FileSystem.FolderRemove(root, true);
    }
}
