#pragma once
#include "o2/Scene/UI/Widgets/PopupWidget.h"
#include "o2/Utils/Editor/DragHandle.h"

using namespace o2;

namespace o2
{
    class Button;
    class EditBox;
    class HorizontalLayout;
    class Toggle;
}

namespace Editor
{
    FORWARD_CLASS_REF(LayerPopupItem);

    // -----------------------
    // Scene layers list popup
    // -----------------------
    class LayersPopup: public PopupWidget
    {
    public:
        // Default constructor
        LayersPopup(RefCounter* refCounter);

        // Destructor
        ~LayersPopup();

        // Shows layers popup at position
        void Show(const Vec2F& position);

        // Updates widget and dragging animation
        void Update(float dt) override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(LayersPopup);
        CLONEABLE_REF(LayersPopup);

    private:
        Ref<LayerPopupItem> mItemSample; // Layer item sample @SERIALIZABLE

        Vector<Ref<LayerPopupItem>> mItemsCache; // Cached items widgets

        Ref<LayerPopupItem> mDraggingItem;                      // Current dragging item
        Vec2F               mDragOffset;                        // Offset from left bottom corner of dragging item to cursor
        float               mDragAnimTime = 0.4f;               // Node expanding time
        Curve               mDragAnimFunc = Curve::EaseInOut(); // Expanding easing node curve

        Ref<HorizontalLayout> mAddButtonLayout; // Add new layer button container layout
        Ref<Button>           mAddButton;       // Add new layer button

        private:
        // Special drawing for contexts
        void SpecialDraw() override;

        // Returns content size for fitting by children
        Vec2F GetContentSize() const override;

        // Initializes item sample, add button
        void InitializeControls();

        // Updates item layout by index and instert coef
        void UpdateItemLayout(const Ref<LayerPopupItem>& item, int idx);

        // Updates layers list
        void UpdateLayersList();

        // Updates layers list and fits size
        void UpdateLayersListAndFit();

        // Called when item drag began
        void BeginDragging(const Ref<LayerPopupItem>& item);

        // Called until dragging
        void UpdateDragging();

        // Updates drag animation of items
        void UpdateDragAnimation(float dt);

        // it is colled when item has dropped
        void EndDragging();

        friend class LayerPopupItem;
    };

    // ----------------------
    // Layers view popup item
    // ----------------------
    class LayerPopupItem: public Widget, public SelectableDragableObject
    {
    public:
        // Default constructor
        LayerPopupItem(RefCounter* refCounter);

        // Copy-constructor
        LayerPopupItem(RefCounter* refCounter, const LayerPopupItem& other);

        // Copy-operator
        LayerPopupItem& operator=(const LayerPopupItem& other);

        // Sets layer and updates view
        void SetLayer(const Ref<SceneLayer>& layer);

        // Sets name edit box active
        void BeginEditName();

        // Breaks name editing
        void BreakEditName();

        // Draws widget
        void Draw() override;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(LayerPopupItem);
        CLONEABLE_REF(LayerPopupItem);

    private:        
        Ref<SceneLayer> mLayer;

        Ref<LayersPopup> mPopup;

        DragHandle mDragHandle;

        Ref<Toggle>  mVisibleToggle;
        Ref<Label>   mNameCaption;
        Ref<Button>  mRemoveBtn;
        Ref<EditBox> mEditBox;

        float mDragInsertCoef = 0.0f;
        float mDragTargetInsertCoef = 0.0f;

    private:
        // Called when cursor double clicked
        void OnCursorDblClicked(const Input::Cursor& cursor) override;

        // Called when started dragging
        void OnDragStart(const Input::Cursor& cursor) override;

        // Called when dragged
        void OnDragged(const Input::Cursor& cursor, const Ref<DragDropArea>& area) override;

        // Called when dragging completed
        void OnDragEnd(const Input::Cursor& cursor) override;

        // Called when name edit box has changed
        void OnNameEditChanged(const WString& str);

        // Called when visible toggle changed
        void OnVisibleChanged(bool visible);

        // Called when remove button has pressed
        void OnRemovePressed();

        REF_COUNTERABLE_IMPL(Widget);

        friend class LayersPopup;
    };
}
// --- META ---

CLASS_BASES_META(Editor::LayersPopup)
{
    BASE_CLASS(o2::PopupWidget);
}
END_META;
CLASS_FIELDS_META(Editor::LayersPopup)
{
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().NAME(mItemSample);
    FIELD().PRIVATE().NAME(mItemsCache);
    FIELD().PRIVATE().NAME(mDraggingItem);
    FIELD().PRIVATE().NAME(mDragOffset);
    FIELD().PRIVATE().DEFAULT_VALUE(0.4f).NAME(mDragAnimTime);
    FIELD().PRIVATE().DEFAULT_VALUE(Curve::EaseInOut()).NAME(mDragAnimFunc);
    FIELD().PRIVATE().NAME(mAddButtonLayout);
    FIELD().PRIVATE().NAME(mAddButton);
}
END_META;
CLASS_METHODS_META(Editor::LayersPopup)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Show, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PRIVATE().SIGNATURE(void, SpecialDraw);
    FUNCTION().PRIVATE().SIGNATURE(Vec2F, GetContentSize);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeControls);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateItemLayout, const Ref<LayerPopupItem>&, int);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateLayersList);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateLayersListAndFit);
    FUNCTION().PRIVATE().SIGNATURE(void, BeginDragging, const Ref<LayerPopupItem>&);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateDragging);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateDragAnimation, float);
    FUNCTION().PRIVATE().SIGNATURE(void, EndDragging);
}
END_META;

CLASS_BASES_META(Editor::LayerPopupItem)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::SelectableDragableObject);
}
END_META;
CLASS_FIELDS_META(Editor::LayerPopupItem)
{
    FIELD().PRIVATE().NAME(mLayer);
    FIELD().PRIVATE().NAME(mPopup);
    FIELD().PRIVATE().NAME(mDragHandle);
    FIELD().PRIVATE().NAME(mVisibleToggle);
    FIELD().PRIVATE().NAME(mNameCaption);
    FIELD().PRIVATE().NAME(mRemoveBtn);
    FIELD().PRIVATE().NAME(mEditBox);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mDragInsertCoef);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mDragTargetInsertCoef);
}
END_META;
CLASS_METHODS_META(Editor::LayerPopupItem)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const LayerPopupItem&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLayer, const Ref<SceneLayer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginEditName);
    FUNCTION().PUBLIC().SIGNATURE(void, BreakEditName);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorDblClicked, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDragStart, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDragged, const Input::Cursor&, const Ref<DragDropArea>&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDragEnd, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnNameEditChanged, const WString&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnVisibleChanged, bool);
    FUNCTION().PRIVATE().SIGNATURE(void, OnRemovePressed);
}
END_META;
// --- END META ---
