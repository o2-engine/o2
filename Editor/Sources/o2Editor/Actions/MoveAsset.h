#pragma once

#include "o2/Assets/AssetInfo.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace Editor
{
    // ----------------------------------------------------------
    // Moves one or more assets (by UID) into destFolder. Undo
    // sends each asset back to its original parent folder.
    // ----------------------------------------------------------
    class MoveAssetAction: public IAction
    {
    public:
        class Entry: public ISerializable
        {
        public:
            UID    uid;
            String filename;       // file name with extension
            String originalParent; // parent folder of the asset before move

            bool operator==(const Entry& other) const { return uid == other.uid; }

            SERIALIZABLE(Entry);
        };

    public:
        Vector<Entry> entries;
        String        destFolder;

    public:
        MoveAssetAction();
        MoveAssetAction(const Vector<Entry>& entries, const String& destFolder);

        String GetName() const override;
        void Redo() override;
        void Undo() override;

        SERIALIZABLE(MoveAssetAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::MoveAssetAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::MoveAssetAction)
{
    FIELD().PUBLIC().NAME(entries);
    FIELD().PUBLIC().NAME(destFolder);
}
END_META;
CLASS_METHODS_META(Editor::MoveAssetAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Entry>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::MoveAssetAction::Entry)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::MoveAssetAction::Entry)
{
    FIELD().PUBLIC().NAME(uid);
    FIELD().PUBLIC().NAME(filename);
    FIELD().PUBLIC().NAME(originalParent);
}
END_META;
CLASS_METHODS_META(Editor::MoveAssetAction::Entry)
{
}
END_META;
// --- END META ---
