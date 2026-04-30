#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/FileSystem/FileInfo.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace
{
    int g_dirCounter = 0;

    String TempDirName(const char* tag)
    {
        g_dirCounter++;
        return String("o2_fs_test_") + tag + "_" + (String)g_dirCounter;
    }

    class TempDir
    {
    public:
        explicit TempDir(const char* tag): mPath(TempDirName(tag))
        {
            o2FileSystem.FolderRemove(mPath);
            o2FileSystem.FolderCreate(mPath, false);
        }

        ~TempDir()
        {
            o2FileSystem.FolderRemove(mPath);
        }

        const String& Path() const { return mPath; }

        String File(const String& name) const { return mPath + "/" + name; }

    private:
        String mPath;
    };

    bool WriteText(const String& path, const String& contents)
    {
        OutFile f(path);
        if (!f.IsOpened())
            return false;
        f.WriteData(contents.Data(), contents.Length());
        return true;
    }
}

// ===== Static path utilities =====

TEST(FileSystem, GetFileExtensionReturnsTextAfterLastDot)
{
    EXPECT_EQ(FileSystem::GetFileExtension("foo.txt"), "txt");
    EXPECT_EQ(FileSystem::GetFileExtension("a/b/c.PNG"), "PNG");
    EXPECT_EQ(FileSystem::GetFileExtension("archive.tar.gz"), "gz");
}

TEST(FileSystem, GetFileExtensionEmptyWhenNoDot)
{
    EXPECT_TRUE(FileSystem::GetFileExtension("nodot").IsEmpty());
}

TEST(FileSystem, GetFileNameWithoutExtensionStripsLastDot)
{
    EXPECT_EQ(FileSystem::GetFileNameWithoutExtension("foo.txt"), "foo");
    EXPECT_EQ(FileSystem::GetFileNameWithoutExtension("path/to/file.png"), "path/to/file");
    EXPECT_EQ(FileSystem::GetFileNameWithoutExtension("archive.tar.gz"), "archive.tar");
}

TEST(FileSystem, GetPathWithoutDirectoriesReturnsLeaf)
{
    EXPECT_EQ(FileSystem::GetPathWithoutDirectories("a/b/c.txt"), "c.txt");
    EXPECT_EQ(FileSystem::GetPathWithoutDirectories("a\\b\\c.txt"), "c.txt");
    EXPECT_EQ(FileSystem::GetPathWithoutDirectories("just_a_file.png"), "just_a_file.png");
}

TEST(FileSystem, GetParentPathDropsLeaf)
{
    EXPECT_EQ(FileSystem::GetParentPath("a/b/c.txt"), "a/b");
    EXPECT_EQ(FileSystem::GetParentPath("a\\b\\c.txt"), "a\\b");
    EXPECT_TRUE(FileSystem::GetParentPath("nodir.txt").IsEmpty());
}

// ===== Instance helpers =====

TEST(FileSystem, ExtractPathStrReturnsParentDirectoryStyleOnly)
{
    EXPECT_EQ(o2FileSystem.ExtractPathStr("a/b/c"), "a/b");
    EXPECT_TRUE(o2FileSystem.ExtractPathStr("noslash").IsEmpty());
}

// ===== Real filesystem ops =====

TEST(FileSystem, FolderCreateAndRemove)
{
    String path = TempDirName("create_remove");

    EXPECT_FALSE(o2FileSystem.IsFolderExist(path));
    EXPECT_TRUE(o2FileSystem.FolderCreate(path, false));
    EXPECT_TRUE(o2FileSystem.IsFolderExist(path));
    EXPECT_TRUE(o2FileSystem.FolderRemove(path));
    EXPECT_FALSE(o2FileSystem.IsFolderExist(path));
}

TEST(FileSystem, FolderCreateRecursiveCreatesIntermediatePath)
{
    TempDir dir("recursive");
    String nested = dir.Path() + "/a/b/c";

    EXPECT_TRUE(o2FileSystem.FolderCreate(nested, true));
    EXPECT_TRUE(o2FileSystem.IsFolderExist(nested));
    EXPECT_TRUE(o2FileSystem.IsFolderExist(dir.File("a")));
    EXPECT_TRUE(o2FileSystem.IsFolderExist(dir.File("a/b")));
}

TEST(FileSystem, IsFolderExistReturnsFalseForUnknown)
{
    EXPECT_FALSE(o2FileSystem.IsFolderExist("definitely_missing_o2_folder_xyz"));
}

TEST(FileSystem, IsFileExistReturnsTrueForCreatedFile)
{
    TempDir dir("is_file_exist");
    String file = dir.File("hello.txt");

    EXPECT_FALSE(o2FileSystem.IsFileExist(file));
    ASSERT_TRUE(WriteText(file, "data"));
    EXPECT_TRUE(o2FileSystem.IsFileExist(file));
}

TEST(FileSystem, ReadFileReturnsWrittenContents)
{
    TempDir dir("read_write");
    String file = dir.File("rw.txt");

    String original = "hello o2 file system";
    FileSystem::WriteFile(file, original);

    String read = FileSystem::ReadFile(file);
    EXPECT_EQ(read, original);
}

TEST(FileSystem, ReadFileMissingReturnsEmpty)
{
    EXPECT_TRUE(FileSystem::ReadFile("o2_does_not_exist_test.txt").IsEmpty());
}

TEST(FileSystem, FileCopyDuplicatesContent)
{
    TempDir dir("file_copy");
    String src = dir.File("src.txt");
    String dst = dir.File("dst.txt");

    FileSystem::WriteFile(src, "payload");

    EXPECT_TRUE(o2FileSystem.FileCopy(src, dst));
    EXPECT_TRUE(o2FileSystem.IsFileExist(dst));
    EXPECT_EQ(FileSystem::ReadFile(dst), "payload");
}

TEST(FileSystem, FileMoveTransfersToDestination)
{
    TempDir dir("file_move");
    String src = dir.File("src.txt");
    String dst = dir.File("nested/dst.txt");

    FileSystem::WriteFile(src, "moveme");

    EXPECT_TRUE(o2FileSystem.FileMove(src, dst));
    EXPECT_FALSE(o2FileSystem.IsFileExist(src));
    EXPECT_TRUE(o2FileSystem.IsFileExist(dst));
    EXPECT_EQ(FileSystem::ReadFile(dst), "moveme");
}

TEST(FileSystem, FileDeleteRemovesFile)
{
    TempDir dir("file_delete");
    String file = dir.File("kill.txt");

    FileSystem::WriteFile(file, "x");
    EXPECT_TRUE(o2FileSystem.IsFileExist(file));

    EXPECT_TRUE(o2FileSystem.FileDelete(file));
    EXPECT_FALSE(o2FileSystem.IsFileExist(file));
}

TEST(FileSystem, FileDeleteOnMissingReturnsFalse)
{
    EXPECT_FALSE(o2FileSystem.FileDelete("o2_missing_file_for_delete.txt"));
}

TEST(FileSystem, GetFileInfoReportsSizeForExistingFile)
{
    TempDir dir("file_info");
    String file = dir.File("info.txt");
    String contents = "1234567890";

    FileSystem::WriteFile(file, contents);

    FileInfo info = o2FileSystem.GetFileInfo(file);
    EXPECT_NE(info.path, "invalid_file");
    EXPECT_EQ(info.size, (Int64)contents.Length());
}

TEST(FileSystem, GetFileInfoMissingMarkedInvalid)
{
    FileInfo info = o2FileSystem.GetFileInfo("o2_missing_for_info.txt");
    EXPECT_EQ(info.path, "invalid_file");
}

TEST(FileSystem, GetFolderInfoEnumeratesFiles)
{
    TempDir dir("folder_info");

    FileSystem::WriteFile(dir.File("a.txt"), "a");
    FileSystem::WriteFile(dir.File("b.txt"), "bb");
    o2FileSystem.FolderCreate(dir.File("sub"));
    FileSystem::WriteFile(dir.File("sub/c.txt"), "ccc");

    FolderInfo info = o2FileSystem.GetFolderInfo(dir.Path());

    EXPECT_GE(info.files.Count(), 2);
    EXPECT_GE(info.folders.Count(), 1);
}

TEST(FolderInfo, IsFileExistFindsFileRecursively)
{
    FolderInfo root;
    root.path = "root";

    FolderInfo sub;
    sub.path = "root/sub";
    FileInfo f;
    f.path = "root/sub/found.txt";
    f.size = 0;
    sub.files.Add(f);

    root.folders.Add(sub);

    EXPECT_TRUE(root.IsFileExist("root/sub/found.txt"));
    EXPECT_FALSE(root.IsFileExist("root/sub/missing.txt"));
}

// ===== InFile / OutFile =====

TEST(File, OutFileWritesAndInFileReadsRoundtrip)
{
    TempDir dir("file_io");
    String path = dir.File("data.bin");

    {
        OutFile out(path);
        ASSERT_TRUE(out.IsOpened());
        const char payload[] = "octofile";
        out.WriteData(payload, sizeof(payload) - 1);
    }

    InFile in(path);
    ASSERT_TRUE(in.IsOpened());
    EXPECT_EQ(in.GetDataSize(), 8u);

    char buf[16] = {};
    in.ReadData(buf, 8);
    EXPECT_STREQ(buf, "octofile");
}

TEST(File, InFileMissingNotOpened)
{
    InFile in("o2_missing_file_xyz.bin");
    EXPECT_FALSE(in.IsOpened());
}

TEST(File, InFileCaretPositioning)
{
    TempDir dir("caret");
    String path = dir.File("caret.txt");

    FileSystem::WriteFile(path, "abcdef");

    InFile in(path);
    ASSERT_TRUE(in.IsOpened());

    in.SetCaretPos(2);
    EXPECT_EQ(in.GetCaretPos(), 2u);

    char buf[4] = {};
    in.ReadData(buf, 3);
    buf[3] = '\0';
    EXPECT_STREQ(buf, "cde");
}
