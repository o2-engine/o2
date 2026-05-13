#include "o2Editor/stdafx.h"
#include "CreateAssetFromScene.h"

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/AssetsTrash.h"

namespace Editor
{
    CreateAssetFromSceneAction::CreateAssetFromSceneAction()
    {}

    CreateAssetFromSceneAction::CreateAssetFromSceneAction(const Vector<SceneUID>& actorIds, const String& destFolder):
        destFolder(destFolder)
    {
        for (auto& id : actorIds)
        {
            Entry e;
            e.actorId = id;
            entries.Add(e);
        }
    }

    String CreateAssetFromSceneAction::GetName() const
    {
        return "Create asset from scene";
    }

    void CreateAssetFromSceneAction::Redo()
    {
        for (auto& e : entries)
        {
            if (!e.trashPath.IsEmpty())
            {
                AssetsTrash::RestoreAsset(e.trashPath, e.createdPath);
                e.trashPath = "";

                auto obj = o2Scene.GetEditableObjectByID(e.actorId);
                auto actor = DynamicCast<Actor>(obj);
                if (actor)
                {
                    AssetRef<ActorAsset> asset(e.createdPath);
                    actor->SetPrototype(asset);
                }
                continue;
            }

            auto obj = o2Scene.GetEditableObjectByID(e.actorId);
            auto actor = DynamicCast<Actor>(obj);
            if (!actor)
                continue;

            AssetRef<ActorAsset> newAsset = actor->MakePrototype();
            String basePath = destFolder.IsEmpty() ? newAsset->GetActor()->name + String(".proto")
                                                   : destFolder + "/" + newAsset->GetActor()->name + String(".proto");
            String uniquePath = o2Assets.MakeUniqueAssetName(basePath);
            newAsset->Save(uniquePath);
            e.createdPath = uniquePath;
        }
        AssetsTrash::NotifyAssetsChanged();
    }

    void CreateAssetFromSceneAction::Undo()
    {
        for (auto& e : entries)
        {
            if (e.createdPath.IsEmpty())
                continue;

            auto obj = o2Scene.GetEditableObjectByID(e.actorId);
            auto actor = DynamicCast<Actor>(obj);
            if (actor)
                actor->BreakPrototypeLink();

            e.trashPath = AssetsTrash::StashAsset(e.createdPath);
        }
        AssetsTrash::NotifyAssetsChanged();
    }
}
// --- META ---

DECLARE_CLASS(Editor::CreateAssetFromSceneAction, Editor__CreateAssetFromSceneAction);

DECLARE_CLASS(Editor::CreateAssetFromSceneAction::Entry, Editor__CreateAssetFromSceneAction__Entry);
// --- END META ---
