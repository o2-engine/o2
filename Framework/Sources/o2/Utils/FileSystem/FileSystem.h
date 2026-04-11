#pragma once

#include "o2/Utils/FileSystem/FileInfo.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"


#if defined PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif
namespace o2
{
    class LogStream;

    // File system access macros
#define o2FileSystem o2::FileSystem::Instance()

    // ---------------------------------------------------
    // File system. Using for working with files and paths
    // ---------------------------------------------------
    class FileSystem: public Singleton<FileSystem>
    {
    public:
        // Default constructor
        FileSystem(RefCounter* refCounter);

        // Destructor
        ~FileSystem();

#if defined PLATFORM_ANDROID
        // Returns android asset manager
        AAssetManager* GetAssetManager() const;
#elif defined PLATFORM_MAC || defined PLATFORM_IOS
        // Returns bundle path
    static const String& GetBundlePath();

    // Returns bundle path for relative resource path
    static String GetBundlePath(const String& relativePath);
#endif

    // Returns runtime-readable path for file access
    static String GetPathForReading(const String& path);

        // Returns info of paths and files of specified path
        FolderInfo GetFolderInfo(const String& path) const;

        // Returns file info
        FileInfo GetFileInfo(const String& path) const;

        // Sets file edited date
        bool SetFileEditDate(const String& path, const TimeStamp& time) const;

        // Copying file
        bool FileCopy(const String& source, const String& dest) const;

        // Deletion file
        bool FileDelete(const String& file) const;

        // Moving file
        bool FileMove(const String& source, const String& dest) const;

        // Creates folder
        bool FolderCreate(const String& path, bool recursive = true) const;

        // Moves folder
        bool FolderCopy(const String& from, const String& to) const;

        // Removes directory
        bool FolderRemove(const String& path, bool recursive = true) const;

        // Renames file or directory
        bool Rename(const String& old, const String& newPath) const;

        // Returns true if specified directory exist
        bool IsFolderExist(const String& path) const;

        // Returns true if specified file exist
        bool IsFileExist(const String& path) const;

        // Returns path from string
        String ExtractPathStr(const String& path) const;

        // Returns file extension
        static String GetFileExtension(const String& filePath);

        // Returns file name without extension
        static String GetFileNameWithoutExtension(const String& filePath);

        // Returns end path without parent directories
        static String GetPathWithoutDirectories(const String& path);

        // Returns parent path
        static String GetParentPath(const String& path);

        // Read file and returns result
        static String ReadFile(const String& path);

        // Returns a relative path from one path to another
        static String GetPathRelativeToPath(const String& from, const String& to);

        // Simplifies a path by removing navigation elements such as "." and ".." to produce a direct, well-formed path
        static String CanonicalizePath(const String& path);

        // Writes file data
        static void WriteFile(const String& path, const String& data);

    private:
        Ref<LogStream> mLog; // File system log stream
    };
}
