#pragma once

#include "o2/Utils/Types/String.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace Editor
{
    // ----------------------------------------------------------
    // Creates a single asset of the given type at the given path.
    // First Redo creates a fresh sample of the type and saves it.
    // Undo stashes the file. Re-Redo restores from trash.
    // ----------------------------------------------------------
    class CreateAssetAction: public IAction
    {
    public:
        String assetTypeName;
        String parentFolderPath;
        String assetName;
        String createdPath;
        String trashPath;

    public:
        CreateAssetAction();
        CreateAssetAction(const Type& assetType, const String& parentFolderPath, const String& assetName);

        String GetName() const override;
        void Redo() override;
        void Undo() override;

        SERIALIZABLE(CreateAssetAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::CreateAssetAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::CreateAssetAction)
{
    FIELD().PUBLIC().NAME(assetTypeName);
    FIELD().PUBLIC().NAME(parentFolderPath);
    FIELD().PUBLIC().NAME(assetName);
    FIELD().PUBLIC().NAME(createdPath);
    FIELD().PUBLIC().NAME(trashPath);
}
END_META;
CLASS_METHODS_META(Editor::CreateAssetAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Type&, const String&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
