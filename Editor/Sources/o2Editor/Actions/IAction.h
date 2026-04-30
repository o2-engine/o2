#pragma once

#include "o2/Utils/Serialization/Serializable.h"

using namespace o2;

namespace Editor
{
    // -----------------------------
    // Basic editor action interface
    // -----------------------------
    class IAction: public ISerializable, public RefCounterable
    {
    public:
        // VIrtual destructor
        virtual ~IAction() {}

        // Returns name of action
        virtual String GetName() const { return "Unknown"; }

        // Does action again
        virtual void Redo() {}

        // Undoing action
        virtual void Undo() {}

        // Merges incremental action into this and applies its Redo
        void Append(const Ref<IAction>& other);

        SERIALIZABLE(IAction);

    protected:
        // Merges other into this if compatible, returns true on success
        virtual bool TryMerge(const Ref<IAction>& other) { return false; }
    };
}
// --- META ---

CLASS_BASES_META(Editor::IAction)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(Editor::IAction)
{
}
END_META;
CLASS_METHODS_META(Editor::IAction)
{

    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
    FUNCTION().PUBLIC().SIGNATURE(void, Append, const Ref<IAction>&);
    FUNCTION().PROTECTED().SIGNATURE(bool, TryMerge, const Ref<IAction>&);
}
END_META;
// --- END META ---
