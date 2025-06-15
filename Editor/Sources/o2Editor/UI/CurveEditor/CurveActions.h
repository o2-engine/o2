#pragma once

#include "o2Editor/Actions/IAction.h"
#include "o2Editor/UI/CurveEditor/CurvesEditor.h"

namespace Editor
{
    // ----------------------------
    // Add to the curve keys action
    // ----------------------------
    class CurveAddKeysAction : public IAction
    {
    public:
        // Default constructor
        CurveAddKeysAction();

        // Constructor
        CurveAddKeysAction(const Vector<CurvesEditor::CurveKeysInfo>& infos, const Ref<CurvesEditor>& editor);

        // Get the action name
        String GetName() const override;

        // Redo the action
        void Redo() override;

        // Undo the action
        void Undo() override;

        SERIALIZABLE(CurveAddKeysAction);

    protected:
        Vector<CurvesEditor::CurveKeysInfo> mInfos;  // Added keys info
        Ref<CurvesEditor>                   mEditor; // Curves editor
    };

    // ---------------------------------
    // Delete from the curve keys action
    // ---------------------------------
    class CurveDeleteKeysAction : public IAction
    {
    public:
        // Default constructor
        CurveDeleteKeysAction();

        // Constructor
        CurveDeleteKeysAction(const Vector<CurvesEditor::CurveKeysInfo>& infos, const Ref<CurvesEditor>& editor);

        // Get the action name
        String GetName() const override;

        // Redo the action
        void Redo() override;

        // Undo the action
        void Undo() override;

        SERIALIZABLE(CurveDeleteKeysAction);

    protected:
        Vector<CurvesEditor::CurveKeysInfo> mInfos;  // Deleted keys info
        Ref<CurvesEditor>                   mEditor; // Curves editor
    };

    // ----------------------------
    // Change the curve keys action
    // ----------------------------
    class CurveKeysChangeAction : public IAction
    {
    public:
        struct KeysInfo
        {
            String curveId;

            Vector<Curve::Key> beforeKeys;
            Vector<Curve::Key> afterKeys;

            Vector<CurvesEditor::SelectedHandlesInfo> selectedHandles;

            bool operator==(const KeysInfo& other) const;
        };

    public:
        // Default constructor
        CurveKeysChangeAction();

        // Constructor
        CurveKeysChangeAction(const Vector<KeysInfo>& infos, const Ref<CurvesEditor>& editor);

        // Get the action name
        String GetName() const override;

        // Redo the action
        void Redo() override;

        // Undo the action
        void Undo() override;

        SERIALIZABLE(CurveKeysChangeAction);

    protected:
        Vector<KeysInfo>  mInfos;  // Changed keys info
        Ref<CurvesEditor> mEditor; // Curves editor
    };
}
// --- META ---

CLASS_BASES_META(Editor::CurveAddKeysAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::CurveAddKeysAction)
{
    FIELD().PROTECTED().NAME(mInfos);
    FIELD().PROTECTED().NAME(mEditor);
}
END_META;
CLASS_METHODS_META(Editor::CurveAddKeysAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<CurvesEditor::CurveKeysInfo>&, const Ref<CurvesEditor>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::CurveDeleteKeysAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::CurveDeleteKeysAction)
{
    FIELD().PROTECTED().NAME(mInfos);
    FIELD().PROTECTED().NAME(mEditor);
}
END_META;
CLASS_METHODS_META(Editor::CurveDeleteKeysAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<CurvesEditor::CurveKeysInfo>&, const Ref<CurvesEditor>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::CurveKeysChangeAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::CurveKeysChangeAction)
{
    FIELD().PROTECTED().NAME(mInfos);
    FIELD().PROTECTED().NAME(mEditor);
}
END_META;
CLASS_METHODS_META(Editor::CurveKeysChangeAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<KeysInfo>&, const Ref<CurvesEditor>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
