#pragma once

#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace Editor
{
    // ----------------------------------------------------------
    // Copies one or more assets to destFolder. First Redo copies
    // via o2Assets.CopyAsset; Undo stashes copies into trash.
    // Re-Redo restores from trash.
    // ----------------------------------------------------------
    class CopyAssetsAction: public IAction
    {
    public:
        class Entry: public ISerializable
        {
        public:
            String sourcePath;
            String createdPath;
            String trashPath;

            bool operator==(const Entry& other) const { return sourcePath == other.sourcePath; }

            SERIALIZABLE(Entry);
        };

    public:
        Vector<Entry> entries;
        String        destFolder;

    public:
        CopyAssetsAction();
        CopyAssetsAction(const Vector<String>& sourcePaths, const String& destFolder);

        String GetName() const override;
        void Redo() override;
        void Undo() override;

        SERIALIZABLE(CopyAssetsAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::CopyAssetsAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::CopyAssetsAction)
{
    FIELD().PUBLIC().NAME(entries);
    FIELD().PUBLIC().NAME(destFolder);
}
END_META;
CLASS_METHODS_META(Editor::CopyAssetsAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<String>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::CopyAssetsAction::Entry)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::CopyAssetsAction::Entry)
{
    FIELD().PUBLIC().NAME(sourcePath);
    FIELD().PUBLIC().NAME(createdPath);
    FIELD().PUBLIC().NAME(trashPath);
}
END_META;
CLASS_METHODS_META(Editor::CopyAssetsAction::Entry)
{
}
END_META;
// --- END META ---
