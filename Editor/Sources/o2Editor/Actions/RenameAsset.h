#pragma once

#include "o2/Assets/AssetInfo.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace Editor
{
    // -------------------------------------------------------
    // Renames an asset (by UID) from originalName to newName.
    // -------------------------------------------------------
    class RenameAssetAction: public IAction
    {
    public:
        UID    assetUid;
        String originalName;
        String newName;

    public:
        RenameAssetAction();
        RenameAssetAction(const UID& assetUid, const String& originalName, const String& newName);

        String GetName() const override;
        void Redo() override;
        void Undo() override;

        SERIALIZABLE(RenameAssetAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::RenameAssetAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::RenameAssetAction)
{
    FIELD().PUBLIC().NAME(assetUid);
    FIELD().PUBLIC().NAME(originalName);
    FIELD().PUBLIC().NAME(newName);
}
END_META;
CLASS_METHODS_META(Editor::RenameAssetAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const UID&, const String&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
