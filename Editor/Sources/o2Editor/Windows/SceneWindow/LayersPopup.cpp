#include "o2Editor/stdafx.h"
#include "LayersPopup.h"

#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/EditorScope.h"

namespace Editor
{
    LayersPopup::LayersPopup(RefCounter* refCounter):
        PopupWidget(refCounter)
    {
        InitializeControls();
    }

    LayersPopup::~LayersPopup()
    {}

    void LayersPopup::Show(const Vec2F& position)
    {
        UpdateLayersList();
        PopupWidget::Show(position);
    }

    void LayersPopup::Update(float dt)
    {
        PopupWidget::Update(dt);

        if (mDraggingItem)
            UpdateDragAnimation(dt);
    }

    String LayersPopup::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void LayersPopup::SpecialDraw()
    {
        PopupWidget::SpecialDraw();
    }

    Vec2F LayersPopup::GetContentSize() const
    {
        return Vec2F(200, mChildWidgets.Count()*mItemSample->layout->GetMinHeight());
    }

    void LayersPopup::InitializeControls()
    {
        AddLayer("back", mmake<Sprite>("ui/UI4_Context_menu.png"), Layout::BothStretch(-20, -19, -20, -19));

        mItemSample = mmake<LayerPopupItem>();

        mAddButtonLayout = mmake<HorizontalLayout>();
        *mAddButtonLayout->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, 20);
        mAddButtonLayout->layout->minHeight = 20;
        mAddButtonLayout->AddLayer("line", mmake<Sprite>("ui/UI4_Separator.png"),
                                   Layout::HorStretch(VerAlign::Top, 0, 0, 5, -2));
        mAddButtonLayout->AddChild(mmake<Widget>());

        mAddButton = o2UI.CreateWidget<Button>("add small");
        mAddButton->layout->maxWidth = 20;

        mAddButton->isPointInside = [=](const Vec2F& p) {
            return mAddButton->layout->IsPointInside(p) || mAddButtonLayout->layout->IsPointInside(p);
        };

        mAddButton->onClick = [=]() {
            String newLayerName = "New layer";

            int idx = 1;
            while (o2Scene.HasLayer(newLayerName))
                newLayerName = "New layer " + (String)idx++;

            auto newLayer = o2Scene.AddLayer(newLayerName);
            UpdateLayersListAndFit();

            auto item = mChildWidgets.FindOrDefault([=](const Ref<Widget>& x) {
                if (auto item = DynamicCast<LayerPopupItem>(x))
                    return item->mLayer == newLayer;

                return false;
            });

            DynamicCast<LayerPopupItem>(item)->BeginEditName();
        };

        mAddButtonLayout->AddChild(mAddButton);
    }

    void LayersPopup::UpdateItemLayout(const Ref<LayerPopupItem>& item, int idx)
    {
        float itemHeight = mItemSample->layout->GetMinHeight();
        float dragModeOffset = 0.0f;

        if (mDraggingItem)
            dragModeOffset = mDragAnimFunc.Evaluate(item->mDragInsertCoef)*itemHeight;

        item->layout->CopyFrom(WidgetLayout::HorStretch(VerAlign::Top, 0, 0, itemHeight, itemHeight*(float)idx + dragModeOffset));
        item->layout->minSize = Vec2F(200, 20);
    }

    void LayersPopup::UpdateLayersList()
    {
        PushEditorScopeOnStack scope;

        RemoveChild(mAddButtonLayout, false);

        mItemsCache.Add(DynamicCastVector<LayerPopupItem>(mChildWidgets));
        RemoveAllChildren();

        int idx = 0;
        for (auto& layer : o2Scene.GetLayers())
        {
            Ref<LayerPopupItem> item;
            if (!mItemsCache.IsEmpty())
            {
                auto sameLayerItem = mItemsCache.FindOrDefault([=](const Ref<LayerPopupItem>& x) { return x->mLayer == layer; });
                if (sameLayerItem)
                {
                    item = sameLayerItem;
                    mItemsCache.Remove(sameLayerItem);
                }
                else
                    item = mItemsCache.PopBack();
            }
            else 
                item = mItemSample->CloneAsRef<LayerPopupItem>();

            item->SetLayer(layer);
            item->mPopup = Ref(this);
            AddChild(item);
            UpdateItemLayout(item, idx);

            idx++;
        }

        AddChild(mAddButtonLayout);
    }

    void LayersPopup::UpdateLayersListAndFit()
    {
        UpdateLayersList();
        UpdateTransform();
        FitSizeAndPosition(layout->GetWorldLeftTop());
    }

    void LayersPopup::BeginDragging(const Ref<LayerPopupItem>& item)
    {
        mDraggingItem = item;
        mDragOffset = o2Input.GetCursorPos() - mDraggingItem->layout->GetWorldLeftBottom();

        float itemPos = GetChildrenWorldRect().top - item->layout->worldTop;
        float threshold = itemPos - 10.0f;
        float itemHeight = mItemSample->layout->minHeight;
        int idx = 0;
        for (auto& child : mChildWidgets)
        {
            if (auto item = DynamicCast<LayerPopupItem>(child))
            {
                if (item != mDraggingItem)
                {
                    float itemOriginPos = itemHeight*(float)idx;
                    item->mDragInsertCoef = itemOriginPos < threshold ? 0.0f : 1.0f;
                    item->mDragTargetInsertCoef = item->mDragInsertCoef;
                    item->BreakEditName();
                    UpdateItemLayout(item, idx);
                    idx++;
                }
            }
        }
    }

    void LayersPopup::UpdateDragging()
    {
        float itemPos = layout->worldTop - o2Input.GetCursorPos().y - mDragOffset.y;
        *mDraggingItem->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, mDraggingItem->layout->GetMinHeight(),
                                                          itemPos);
        mDraggingItem->layout->minHeight = 20;

        float threshold = itemPos - 10.0f;
        float itemHeight = mItemSample->layout->minHeight;
        int idx = 0;
        for (auto& child : mChildWidgets)
        {
            if (auto item = DynamicCast<LayerPopupItem>(child))
            {
                if (item != mDraggingItem)
                {
                    float itemOriginPos = itemHeight*(float)idx;
                    item->mDragTargetInsertCoef = itemOriginPos < threshold ? 0.0f : 1.0f;
                    idx++;
                }
            }
        }
    }

    void LayersPopup::UpdateDragAnimation(float dt)
    {
        float animCoef = 10.0f;
        int idx = 0;
        for (auto& child : mChildWidgets)
        {
            if (auto item = DynamicCast<LayerPopupItem>(child))
            {
                if (item != mDraggingItem)
                {
                    item->mDragInsertCoef = Math::Lerp(item->mDragInsertCoef, item->mDragTargetInsertCoef, animCoef*dt);
                    UpdateItemLayout(item, idx);
                    idx++;
                }
            }
        }
    }

    void LayersPopup::EndDragging()
    {
        float itemPos = GetChildrenWorldRect().top - mDraggingItem->layout->worldTop + 10.0f;
        int idx = (int)(itemPos/mDraggingItem->layout->minHeight);
        o2Scene.SetLayerOrder(mDraggingItem->mLayer, idx);

        mDraggingItem = nullptr;
        UpdateLayersList();
    }

    LayerPopupItem::LayerPopupItem(RefCounter* refCounter):
        Widget(refCounter)
    {
        layout->minSize = Vec2F(200, 20);
        AddLayer("drag handle", mmake<Sprite>("ui/UI4_drag_handle.png"), Layout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(0, 0)));

        mVisibleToggle = o2UI.CreateWidget<Toggle>();
        *mVisibleToggle->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(20, 0));
        mVisibleToggle->name = "visibility";
        mVisibleToggle->onToggleByUser = THIS_FUNC(OnVisibleChanged);
        AddChild(mVisibleToggle);

        mNameCaption = o2UI.CreateLabel("");
        mNameCaption->name = "name";
        mNameCaption->horAlign = HorAlign::Left;
        *mNameCaption->layout = WidgetLayout::BothStretch(40, 0, 20, 0);
        AddChild(mNameCaption);

        mRemoveBtn = o2UI.CreateWidget<Button>("remove small");
        *mRemoveBtn->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F());
        mRemoveBtn->name = "remove";
        mRemoveBtn->onClick = THIS_FUNC(OnRemovePressed);
        AddChild(mRemoveBtn);

        mEditBox = o2UI.CreateWidget<EditBox>("backless");
        *mEditBox->layout = WidgetLayout::BothStretch(40, 0, 20, 0);
        mEditBox->enabled = false;
        mEditBox->name = "edit name";
        mEditBox->onChangeCompleted = THIS_FUNC(OnNameEditChanged);
        AddChild(mEditBox);
    }

    LayerPopupItem::LayerPopupItem(RefCounter* refCounter, const LayerPopupItem& other):
        Widget(refCounter, other)
    {
        mVisibleToggle = GetChildByType<Toggle>("visibility");
        mRemoveBtn = GetChildByType<Button>("remove");
        mEditBox = GetChildByType<EditBox>("edit name");
        mNameCaption = GetChildByType<Label>("name");

        mEditBox->onChangeCompleted = THIS_FUNC(OnNameEditChanged);
        mVisibleToggle->onToggleByUser = THIS_FUNC(OnVisibleChanged);
        mRemoveBtn->onClick = THIS_FUNC(OnRemovePressed);
    }

    LayerPopupItem& LayerPopupItem::operator=(const LayerPopupItem& other)
    {
        Widget::operator=(other);

        mVisibleToggle = GetChildByType<Toggle>("visibility");
        mRemoveBtn = GetChildByType<Button>("remove");
        mEditBox = GetChildByType<EditBox>("edit name");
        mNameCaption = GetChildByType<Label>("name");

        mEditBox->onChangeCompleted = THIS_FUNC(OnNameEditChanged);
        mVisibleToggle->onToggleByUser = THIS_FUNC(OnVisibleChanged);
        mRemoveBtn->onClick = THIS_FUNC(OnRemovePressed);

        return *this;
    }

    void LayerPopupItem::SetLayer(const Ref<SceneLayer>& layer)
    {
        if (mLayer == layer)
            return;

        mLayer = layer;

        mVisibleToggle->value = layer->visible;
        mNameCaption->text = layer->GetName();
    }

    void LayerPopupItem::BeginEditName()
    {
        mNameCaption->enabled = false;
        mEditBox->enabled = true;
        mEditBox->text = mLayer->GetName();
        mEditBox->Focus();
        mEditBox->SelectAll();
    }

    void LayerPopupItem::BreakEditName()
    {
        mEditBox->Unfocus();
        mEditBox->enabled = false;
        mNameCaption->enabled = true;
    }

    void LayerPopupItem::Draw()
    {
        SelectableDragableObject::OnDrawn();
        Widget::Draw();
    }

    bool LayerPopupItem::IsUnderPoint(const Vec2F& point)
    {
        return layout->IsPointInside(point);
    }

    String LayerPopupItem::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void LayerPopupItem::OnCursorDblClicked(const Input::Cursor& cursor)
    {
        BeginEditName();
    }

    void LayerPopupItem::OnDragStart(const Input::Cursor& cursor)
    {
        mPopup->BeginDragging(Ref(this));
    }

    void LayerPopupItem::OnDragged(const Input::Cursor& cursor, const Ref<DragDropArea>& area)
    {
        mPopup->UpdateDragging();
    }

    void LayerPopupItem::OnDragEnd(const Input::Cursor& cursor)
    {
        mPopup->EndDragging();
    }

    void LayerPopupItem::OnNameEditChanged(const WString& str)
    {
        mEditBox->enabled = false;
        mNameCaption->enabled = true;

        mNameCaption->text = str;
        mLayer->SetName(str);
        mPopup->UpdateLayersList();

        o2Scene.OnObjectChanged(nullptr);
    }

    void LayerPopupItem::OnVisibleChanged(bool visible)
    {
        mLayer->visible = visible;
    }

    void LayerPopupItem::OnRemovePressed()
    {
        o2Scene.RemoveLayer(mLayer);
        mPopup->UpdateLayersListAndFit();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::LayersPopup>);
// --- META ---

DECLARE_CLASS(Editor::LayersPopup, Editor__LayersPopup);

DECLARE_CLASS(Editor::LayerPopupItem, Editor__LayerPopupItem);
// --- END META ---
