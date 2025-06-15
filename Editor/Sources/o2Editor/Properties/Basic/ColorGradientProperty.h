#pragma once

#include "o2/Events/CursorEventsArea.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Math/ColorGradient.h"
#include "o2Editor/Properties/IPropertyField.h"

using namespace o2;

namespace o2
{
    class Widget;
}

namespace Editor
{
    FORWARD_CLASS_REF(CurvePreview);

    // ------------------------------
    // Editor curve property edit box
    // ------------------------------
    class ColorGradientProperty: public TPropertyField<Ref<ColorGradient>>
    {
    public:
        // Default constructor
        ColorGradientProperty(RefCounter* refCounter);

        // Copy constructor
        ColorGradientProperty(RefCounter* refCounter, const ColorGradientProperty& other);

        // Copy operator
        ColorGradientProperty& operator=(const ColorGradientProperty& other);

        SERIALIZABLE(ColorGradientProperty);
        CLONEABLE_REF(ColorGradientProperty);

    public:
        // ---------------------------------------------------------------
        // Gradient preview drawable, used as layer of property box widget
        // ---------------------------------------------------------------
        class GradientPreviewDrawable : public IRectDrawable
        {
        public:
            // Sets gradient and updates mesh
            void SetGradient(const Ref<ColorGradient>& gradient);

            // Draws gradient
            void Draw() override;

            SERIALIZABLE(GradientPreviewDrawable);
            CLONEABLE_REF(GradientPreviewDrawable);

        protected:
            Ref<ColorGradient> mGradient;
            Mesh               mMesh;

        protected:
            // Called when rectangle changed, updates gradient mesh
            void BasisChanged() override;

            // Updates gradient mesh
            void UpdateMesh();
        };

    public:
        enum class HandleType { Left, Middle, Right };

    protected:
        Ref<Widget>                  mBox;     // Gradient edit box 
        Ref<GradientPreviewDrawable> mPreview; // Gradient preview drawable

        Vector<Pair<UInt64, Ref<WidgetDragHandle>>> mHandles; // List of handles, each for keys

        Ref<CursorEventsArea> mBoxArea; // Cursor events area for box

        Ref<ContextMenu> mBoxContextMenu;    // Context menu for box
        Ref<ContextMenu> mHandleContextMenu; // Context menu for handles

        UInt64 mSelectedHandleUID = 0; // Selected handle UID
        Input::Cursor mBoxPressedCursor; // Cursor when box was pressed

    protected:
        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Called when common curve changed in curve edit dialog, tells to drawing content
        void OnValueChanged();

        // Updates value view
        void UpdateValueView() override;

        // Creates new handle
        Ref<WidgetDragHandle> CreateHandle(HandleType type);

        // Finds selected handle UID
        UInt64 FindSelectedHandle();

        // Called when handle changed position and updates key position
        void OnHandleChangedPos(const Vec2F& pos, UInt64 id);

        // Called when handle double clicked and opens color dialog
        void OpenKeyColorPick(UInt64 id);

        // Called when box double clicked and adds new key
        void AddNewKey(const Input::Cursor& cursor);

        // Opens box context menu
        void OpenBoxContextMenu(const Input::Cursor& cursor);

        // Opens handle context menu
        void OpenKeyContextMenu(UInt64 id, const Input::Cursor& cursor);

        // Deletes key by UID
        void DeleteKey(UInt64 keyUid);

        // Creates handles for all keys
        void InitializeHandles();
    };
}
// --- META ---

PRE_ENUM_META(Editor::ColorGradientProperty::HandleType);

CLASS_BASES_META(Editor::ColorGradientProperty)
{
    BASE_CLASS(Editor::TPropertyField<Ref<ColorGradient>>);
}
END_META;
CLASS_FIELDS_META(Editor::ColorGradientProperty)
{
    FIELD().PROTECTED().NAME(mBox);
    FIELD().PROTECTED().NAME(mPreview);
    FIELD().PROTECTED().NAME(mHandles);
    FIELD().PROTECTED().NAME(mBoxArea);
    FIELD().PROTECTED().NAME(mBoxContextMenu);
    FIELD().PROTECTED().NAME(mHandleContextMenu);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mSelectedHandleUID);
    FIELD().PROTECTED().NAME(mBoxPressedCursor);
}
END_META;
CLASS_METHODS_META(Editor::ColorGradientProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ColorGradientProperty&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, OnValueChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(Ref<WidgetDragHandle>, CreateHandle, HandleType);
    FUNCTION().PROTECTED().SIGNATURE(UInt64, FindSelectedHandle);
    FUNCTION().PROTECTED().SIGNATURE(void, OnHandleChangedPos, const Vec2F&, UInt64);
    FUNCTION().PROTECTED().SIGNATURE(void, OpenKeyColorPick, UInt64);
    FUNCTION().PROTECTED().SIGNATURE(void, AddNewKey, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OpenBoxContextMenu, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OpenKeyContextMenu, UInt64, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, DeleteKey, UInt64);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeHandles);
}
END_META;

CLASS_BASES_META(Editor::ColorGradientProperty::GradientPreviewDrawable)
{
    BASE_CLASS(o2::IRectDrawable);
}
END_META;
CLASS_FIELDS_META(Editor::ColorGradientProperty::GradientPreviewDrawable)
{
    FIELD().PROTECTED().NAME(mGradient);
    FIELD().PROTECTED().NAME(mMesh);
}
END_META;
CLASS_METHODS_META(Editor::ColorGradientProperty::GradientPreviewDrawable)
{

    FUNCTION().PUBLIC().SIGNATURE(void, SetGradient, const Ref<ColorGradient>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PROTECTED().SIGNATURE(void, BasisChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateMesh);
}
END_META;
// --- END META ---
