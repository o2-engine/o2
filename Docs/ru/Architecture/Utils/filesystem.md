# Документация `FileSystem`

## Описание
`FileSystem` — синглтон для работы с файловой системой. Позволяет получать информацию о файлах и папках, копировать, удалять, создавать и перемещать их. Также содержит вспомогательные методы для работы с путями.

## Макрос
- **o2FileSystem** — глобальная точка доступа к единственному экземпляру `FileSystem`.

## Конструктор и деструктор
- **FileSystem(RefCounter\* refCounter)** — инициализация.
- **~FileSystem()** — освобождает ресурсы.

## Платформозависимые методы
- **GetAssetManager()** *(Android)* — возвращает `AAssetManager`.
- **GetBundlePath()** *(iOS/Mac)* — возвращает путь к бандлу.

## Основные методы
- **GetFolderInfo(path)** — возвращает информацию о папке и файлах в ней.
- **GetFileInfo(path)** — возвращает информацию о файле.
- **SetFileEditDate(path, time)** — задаёт время изменения файла.
- **FileCopy(source, dest)** — копирует файл.
- **FileDelete(file)** — удаляет файл.
- **FileMove(source, dest)** — перемещает файл.
- **FolderCreate(path, recursive)** — создаёт папку (рекурсивно при необходимости).
- **FolderCopy(from, to)** — копирует папку.
- **FolderRemove(path, recursive)** — удаляет папку (рекурсивно при необходимости).
- **Rename(old, newPath)** — переименовывает файл или папку.
- **IsFolderExist(path)** — проверяет наличие папки.
- **IsFileExist(path)** — проверяет наличие файла.
- **ExtractPathStr(path)** — возвращает путь без имени файла.
- **GetFileExtension(filePath)** — возвращает расширение файла.
- **GetFileNameWithoutExtension(filePath)** — имя файла без расширения.
- **GetPathWithoutDirectories(path)** — возвращает имя папки/файла без родительских директорий.
- **GetParentPath(path)** — возвращает путь к родительской директории.
- **ReadFile(path)** — читает содержимое файла.
- **GetPathRelativeToPath(from, to)** — формирует относительный путь.
- **CanonicalizePath(path)** — упрощает путь, убирает `.` и `..`.
- **WriteFile(path, data)** — записывает строку в файл.

## Внутренние поля
- **mLog** — поток логирования работы с файловой системой.