# `FileSystem` documentation

## Description
`FileSystem` is a singleton for working with the file system. It provides information about files and folders and can copy, delete, create and move them. It also contains helper methods for path handling.

## Macro
- **o2FileSystem** — global access point to the single `FileSystem` instance.

## Constructor and destructor
- **FileSystem(RefCounter\* refCounter)** — initialization.
- **~FileSystem()** — releases resources.

## Platform-specific methods
- **GetAssetManager()** *(Android)* — returns `AAssetManager`.
- **GetBundlePath()** *(iOS/Mac)* — returns the bundle path.

## Main methods
- **GetFolderInfo(path)** — returns information about a folder and the files inside it.
- **GetFileInfo(path)** — returns information about a file.
- **SetFileEditDate(path, time)** — sets the file modification time.
- **FileCopy(source, dest)** — copies a file.
- **FileDelete(file)** — deletes a file.
- **FileMove(source, dest)** — moves a file.
- **FolderCreate(path, recursive)** — creates a folder (recursively if needed).
- **FolderCopy(from, to)** — copies a folder.
- **FolderRemove(path, recursive)** — removes a folder (recursively if needed).
- **Rename(old, newPath)** — renames a file or folder.
- **IsFolderExist(path)** — checks whether a folder exists.
- **IsFileExist(path)** — checks whether a file exists.
- **ExtractPathStr(path)** — returns the path without the file name.
- **GetFileExtension(filePath)** — returns the file extension.
- **GetFileNameWithoutExtension(filePath)** — file name without extension.
- **GetPathWithoutDirectories(path)** — returns the folder/file name without parent directories.
- **GetParentPath(path)** — returns the path to the parent directory.
- **ReadFile(path)** — reads file contents.
- **GetPathRelativeToPath(from, to)** — builds a relative path.
- **CanonicalizePath(path)** — simplifies a path, removing `.` and `..`.
- **WriteFile(path, data)** — writes a string to a file.

## Internal fields
- **mLog** — log stream for file system operations.
