#pragma once

#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace Editor
{
    // ----------------------------------------------------------
    // Turns scene actors into ActorAsset prototypes (drag from
    // scene tree into AssetsWindow). Tracks both sides: creates
    // .proto on disk and links the source actor to it. Undo
    // stashes the asset and breaks the actor's prototype link.
    // ----------------------------------------------------------
    class CreateAssetFromSceneAction: public IAction
    {
    public:
        class Entry: public ISerializable
        {
        public:
            SceneUID actorId;
            String   createdPath;
            String   trashPath;

            bool operator==(const Entry& other) const { return actorId == other.actorId; }

            SERIALIZABLE(Entry);
        };

    public:
        Vector<Entry> entries;
        String        destFolder;

    public:
        CreateAssetFromSceneAction();
        CreateAssetFromSceneAction(const Vector<SceneUID>& actorIds, const String& destFolder);

        String GetName() const override;
        void Redo() override;
        void Undo() override;

        SERIALIZABLE(CreateAssetFromSceneAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::CreateAssetFromSceneAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::CreateAssetFromSceneAction)
{
    FIELD().PUBLIC().NAME(entries);
    FIELD().PUBLIC().NAME(destFolder);
}
END_META;
CLASS_METHODS_META(Editor::CreateAssetFromSceneAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<SceneUID>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::CreateAssetFromSceneAction::Entry)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::CreateAssetFromSceneAction::Entry)
{
    FIELD().PUBLIC().NAME(actorId);
    FIELD().PUBLIC().NAME(createdPath);
    FIELD().PUBLIC().NAME(trashPath);
}
END_META;
CLASS_METHODS_META(Editor::CreateAssetFromSceneAction::Entry)
{
}
END_META;
// --- END META ---
