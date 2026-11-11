#pragma once

#include "o2Editor/Pipeline/UiDoc/UiDocStructure.h"

namespace Editor::UiDoc
{
    // ---------------------------------------------------------------------------------------------------------------
    // A layout as Unity project files (port of AssetsLine shared/uiExport/unity.ts): the prefab the AssetsLine plugin for
    // Unity builds from a ui_structure.json (XuToWei/Image-To-UI's importer), written directly as Unity's serialized
    // YAML in float32 arithmetic, with its sprites (nine-slice borders in their metas), sprite assets for other borders,
    // baked hue shifts and the fonts, every file with a .meta. Guids and fileIDs come from the seed and the paths, so a
    // re-export keeps them. UnityPackage packs the files as a .unitypackage
    // ---------------------------------------------------------------------------------------------------------------

    // A sprite the layout names: its file, PNG, size, nine-slice borders and density (sprite pixels per layout pixel)
    using UiExportSprite = UiStructureSprite;

    // A font file (or its licence) packed with the export
    struct UiExportFont
    {
        String file; // Roboto-Regular.ttf, LICENSE-Roboto.txt
        String data;
    };

    // What an export is built from
    struct UiExportInput
    {
        String                 name;    // Screen name: the prefab's file name (cleaned to A-Z a-z 0-9 _ -)
        String                 folder;  // Folder inside Assets: Generated/UI
        String                 layout;  // ui_structure.json
        Vector<UiExportSprite> sprites; // Every sprite the layout names
        Vector<UiExportFont>   fonts;
        String                 seed;    // Makes guids and ids reproducible (the node id)
    };

    // A file of the export: its path inside the project (Assets/...) and bytes
    struct UiExportFile
    {
        String path;
        String data;
    };

    // The files of an export, or why there are none
    struct UiExportFiles
    {
        bool                 ok = false;
        String               error;
        Vector<UiExportFile> files;
        Vector<String>       warnings; // Sorted
    };

    // The layout as a Unity prefab with its sprites and fonts: project files with their metas
    UiExportFiles ExportUnity(const UiExportInput& input);

    // A .unitypackage of project files: a gzipped tar with a folder per asset named by the guid in its .meta (pathname,
    // asset, asset.meta) and the package manager manifest asking for com.unity.ugui. Empty with the error set when a meta
    // has no guid or a file is missing
    String UnityPackage(const Vector<UiExportFile>& files, String* error = nullptr);

    // A guid from a key, as the exporter derives them (32 hex digits)
    String UnityGuid(const String& key);

    // Unity's float text: the shortest form that reads back as the same float32
    String UnityFloat(double value);

    // The asset files a layout names (its asset strings, a leading ./ or / dropped), each once, in first-use order
    Vector<String> UiAssetRefs(const Json& structure);
}
