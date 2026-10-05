#pragma once

#include <filesystem>

#include "o2/Assets/Assets.h"
#include "o2/Assets/AssetsTree.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"

namespace o2::VectorImageTest
{
    inline String Svg(float width, float height, const String& body)
    {
        return String("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"") + (String)width + "\" height=\"" +
            (String)height + "\">" + body + "</svg>";
    }

    // Temporary folder with vector image sources and an assets tree attached to o2Assets while alive
    class TempVectorAssets
    {
    public:
        TempVectorAssets()
        {
            UID id;
            id.Randomize();
            mPrefix = "vt" + ((String)id).SubStr(0, 8) + "_";

            String root(std::filesystem::temp_directory_path().generic_string().c_str());
            if (!root.EndsWith("/"))
                root += "/";

            mRoot = root + "o2_vector_assets_" + (String)id + "/";
            mSourcePath = mRoot + "Source/";
            o2FileSystem.FolderCreate(mSourcePath, true);
        }

        ~TempVectorAssets()
        {
            Detach();

            std::error_code errorCode;
            std::filesystem::remove_all(std::filesystem::path(mRoot.Data()), errorCode);
        }

        // Returns asset path of the image, unique between tests because assets are cached by path
        String GetPath(const String& name) const
        {
            return mPrefix + name + ".svg";
        }

        const String& GetSourcePath() const { return mSourcePath; }
        const String& GetRootPath() const { return mRoot; }

        void WriteSvg(const String& name, const String& svg)
        {
            o2FileSystem.WriteFile(mSourcePath + GetPath(name), svg);
        }

        void WriteMeta(const String& name, const BorderI& sliceBorder, SpriteMode defaultMode)
        {
            auto meta = mmake<VectorImageAsset::Meta>();
            meta->sliceBorder = sliceBorder;
            meta->defaultMode = defaultMode;

            UID id;
            id.Randomize();

            DataDocument metaData;
            metaData = Ref<AssetMeta>(meta);
            metaData["Value"]["mId"] = id;
            metaData.SaveToFile(mSourcePath + GetPath(name) + ".meta");
        }

        // Attaches tree of the sources as is: vector images are built by plain copy
        void AttachSources()
        {
            auto tree = mmake<AssetsTree>();
            tree->assetsPath = mSourcePath;
            tree->Build(mSourcePath);
            tree->assetsPath = mSourcePath;
            tree->builtAssetsPath = mSourcePath;

            Attach(tree);
        }

        void Attach(const Ref<AssetsTree>& tree)
        {
            Detach();

            mTree = tree;
            const_cast<Vector<Ref<AssetsTree>>&>(o2Assets.GetAssetsTrees()).Add(mTree);
        }

        void Detach()
        {
            if (mTree)
                const_cast<Vector<Ref<AssetsTree>>&>(o2Assets.GetAssetsTrees()).Remove(mTree);

            mTree = nullptr;
        }

        const Ref<AssetsTree>& GetTree() const { return mTree; }

    private:
        String mPrefix;
        String mRoot;
        String mSourcePath;

        Ref<AssetsTree> mTree;
    };
}
