#include "o2/stdafx.h"
#include "AssetInfo.h"

#include "o2/Assets/Types/BinaryAsset.h"

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetsTree.h"
#include "o2/Assets/Assets.h"

namespace o2
{
    FORWARD_REF_IMPL(AssetsTree);

    AssetInfo::AssetInfo()
    {
        if (Assets::IsSingletonInitialzed())
            tree = Ref(const_cast<AssetsTree*>(&o2Assets.GetAssetsTree()));
    }

    AssetInfo::AssetInfo(const AssetInfo& other):
        path(other.path), editTime(other.editTime), tree(other.tree), meta(other.meta),
        mChildren(other.mChildren)
    {}

    AssetInfo::AssetInfo(const Ref<AssetMeta>& meta):
        meta(meta)
    {
        if (Assets::IsSingletonInitialzed())
            tree = Ref(const_cast<AssetsTree*>(&o2Assets.GetAssetsTree()));
    }

    AssetInfo::~AssetInfo()
    {
    }

    AssetInfo& AssetInfo::operator=(const AssetInfo& other)
    {
        meta = other.meta;
        path = other.path;
        editTime = other.editTime;
        tree = other.tree;
        mChildren = other.mChildren;

        return *this;
    }

    AssetInfo::operator bool() const
    {
        return IsValid();
    }

    Ref<AssetInfo> AssetInfo::AddChild(const Ref<AssetInfo>& node)
    {
        if (node->parent)
            node->parent.Lock()->RemoveChild(node);

        node->parent = WeakRef(this);

        mChildren.Add(node);

        return node;
    }

    void AssetInfo::RemoveChild(const Ref<AssetInfo>& node)
    {
        node->parent = nullptr;

        mChildren.Remove(node);
    }

    void AssetInfo::RemoveAllChildren()
    {
        mChildren.Clear();
    }

    void AssetInfo::SetParent(const Ref<AssetInfo>& parent)
    {
        if (parent)
            parent->AddChild(Ref(this));
    }

    void AssetInfo::OnDeserialized(const DataValue& node)
    {
        if (!meta)
        {
            // A meta of a type this build does not know is read as the standard binary asset with the same id
            if (auto metaNode = const_cast<DataValue&>(node).FindMember("meta"))
            {
                if (metaNode->IsObject())
                {
                    (*metaNode)["Type"] = TypeOf(DefaultAssetMeta<BinaryAsset>).GetName();
                    metaNode->Get(meta);
                }
            }
        }

        auto thisRef = Ref(this);
        for (auto& child : mChildren)
            child->parent = thisRef;
    }

    void AssetInfo::SetTree(const Ref<AssetsTree>& tree)
    {
        this->tree = tree;
        tree->allAssets.Add(WeakRef(this));
        tree->allAssetsByPath[path] = WeakRef(this);

        if (meta)
            tree->allAssetsByUID[meta->ID()] = WeakRef(this);

        for (auto& child : mChildren)
            child->SetTree(tree);
    }

    const Vector<Ref<AssetInfo>>& AssetInfo::GetChildren() const
    {
        return mChildren;
    }

    bool AssetInfo::IsValid() const
    {
        return meta && meta->ID() != UID::empty;
    }

    AssetInfo AssetInfo::empty = AssetInfo(mmake<AssetMeta>());

    bool AssetInfo::operator==(const AssetInfo& other) const
    {
        if (meta && other.meta)
            return meta->ID() == other.meta->ID();

        return path == other.path;
    }
}
// --- META ---

DECLARE_CLASS(o2::AssetInfo, o2__AssetInfo);
// --- END META ---
