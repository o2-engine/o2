#pragma once

#include "o2/Assets/AssetInfo.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Editor/DragAndDrop.h"

using namespace o2;

namespace o2
{
    class Label;
}

namespace Editor
{
    FORWARD_CLASS_REF(AssetsIconsScrollArea);

    // --------------------------
    // Dragable asset icon widget
    // --------------------------
    class AssetIcon: public Widget, public SelectableDragableObject, public DragDropArea
    {
    public:
        PROPERTIES(AssetIcon);
        PROPERTY(WString, assetName, SetAssetName, GetAssetName); // Asset name label text property

    public:
        // Default constructor
        AssetIcon(RefCounter* refCounter);

        // Copy-constructor
        AssetIcon(RefCounter* refCounter, const AssetIcon& other);

        // Destructor
        ~AssetIcon();

        // Copy operator
        AssetIcon& operator=(const AssetIcon& other);

        // Sets asset info
        void SetAssetInfo(const Ref<AssetInfo>& info);

        // Returns asset info
        const Ref<AssetInfo>& GetAssetInfo() const;

        // Sets name label text
        void SetAssetName(const WString& name);

        // Returns name label text
        WString GetAssetName() const;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns true when input events can be handled by down listeners
        bool IsInputTransparent() const override;

        // Called when listener was drawn
        void OnDrawn() override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(AssetIcon);
        CLONEABLE_REF(AssetIcon);

    protected:
        Ref<Label>       mNameText;      // Asset name text
        Ref<AssetInfo>   mAssetInfo;     // Asset information
        Ref<WidgetState> mSelectedState; // Node selected state

        WeakRef<AssetsIconsScrollArea> mOwner; // Owner assets scroll area

    protected:
        using SelectableDragableObject::OnDrawn;
        using SelectableDragableObject::OnCursorPressed;
        using SelectableDragableObject::OnCursorReleased;

        // Sets is this selected
        void SetSelected(bool selected) override;

        // Called when cursor double clicked
        void OnCursorDblClicked(const Input::Cursor& cursor) override;

        // Called when right mouse button was released (only when right mouse button pressed this at previous time)
        void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

        // Called when cursor enters this object, moving hover of tree to this
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object, moving hover of tree to this
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor moved on this (or moved outside when this was pressed)
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor released outside this(only when cursor pressed this at previous time)
        void OnCursorReleasedOutside(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when started dragging
        void OnDragStart(const Input::Cursor& cursor) override;

        // Called when dragged
        void OnDragged(const Input::Cursor& cursor, const Ref<DragDropArea>& area) override;

        // Called when dragging completed
        void OnDragEnd(const Input::Cursor& cursor) override;

        // Called when this was selected
        void OnSelected() override;

        // Called when this was unselected
        void OnDeselected() override;

        // Called when some selectable listeners was dropped to this
        void OnDropped(const Ref<ISelectableDragableObjectsGroup>& group) override;

        REF_COUNTERABLE_IMPL(Widget);

        friend class AssetsIconsScrollArea;
    };
}
// --- META ---

CLASS_BASES_META(Editor::AssetIcon)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::SelectableDragableObject);
    BASE_CLASS(o2::DragDropArea);
}
END_META;
CLASS_FIELDS_META(Editor::AssetIcon)
{
    FIELD().PUBLIC().NAME(assetName);
    FIELD().PROTECTED().NAME(mNameText);
    FIELD().PROTECTED().NAME(mAssetInfo);
    FIELD().PROTECTED().NAME(mSelectedState);
    FIELD().PROTECTED().NAME(mOwner);
}
END_META;
CLASS_METHODS_META(Editor::AssetIcon)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AssetIcon&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAssetInfo, const Ref<AssetInfo>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<AssetInfo>&, GetAssetInfo);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAssetName, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetAssetName);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInputTransparent);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDrawn);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, SetSelected, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorDblClicked, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleasedOutside, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragStart, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragged, const Input::Cursor&, const Ref<DragDropArea>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragEnd, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelected);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeselected);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDropped, const Ref<ISelectableDragableObjectsGroup>&);
}
END_META;
// --- END META ---
