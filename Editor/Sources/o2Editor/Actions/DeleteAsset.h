#pragma once

#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace Editor
{
    // ----------------------------------------------------------
    // Deletes one or more assets by stashing them to the editor
    // trash. Undo restores them from the trash.
    // ----------------------------------------------------------
    class DeleteAssetAction: public IAction
    {
    public:
        class Entry: public ISerializable
        {
        public:
            String originalPath;
            String trashPath;

            bool operator==(const Entry& other) const { return originalPath == other.originalPath; }

            SERIALIZABLE(Entry);
        };

    public:
        Vector<Entry> entries;

    public:
        DeleteAssetAction();
        DeleteAssetAction(const Vector<String>& assetsPaths);

        String GetName() const override;
        void Redo() override;
        void Undo() override;

        SERIALIZABLE(DeleteAssetAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::DeleteAssetAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::DeleteAssetAction)
{
    FIELD().PUBLIC().NAME(entries);
}
END_META;
CLASS_METHODS_META(Editor::DeleteAssetAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<String>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::DeleteAssetAction::Entry)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::DeleteAssetAction::Entry)
{
    FIELD().PUBLIC().NAME(originalPath);
    FIELD().PUBLIC().NAME(trashPath);
}
END_META;
CLASS_METHODS_META(Editor::DeleteAssetAction::Entry)
{
}
END_META;
// --- END META ---
