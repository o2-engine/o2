#include "o2/stdafx.h"
#include "UIManager.h"

#include "o2/Application/Application.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/CustomDropDown.h"
#include "o2/Scene/UI/Widgets/CustomList.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/EditBoxDropDown.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalProgress.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/List.h"
#include "o2/Scene/UI/Widgets/PopupWidget.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/VerticalProgress.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/StringUtils.h"
#include "o2/Utils/System/Time/Timer.h"
#include "o2/Events/EventSystem.h"

#undef CreateWindow

namespace o2
{
    DECLARE_SINGLETON(UIManager);

    FORWARD_REF_IMPL(Button);
    FORWARD_REF_IMPL(CustomDropDown);
    FORWARD_REF_IMPL(CustomList);
    FORWARD_REF_IMPL(DropDown);
    FORWARD_REF_IMPL(EditBox);
    FORWARD_REF_IMPL(EditBoxDropDown);
    FORWARD_REF_IMPL(HorizontalLayout);
    FORWARD_REF_IMPL(HorizontalProgress);
    FORWARD_REF_IMPL(HorizontalScrollBar);
    FORWARD_REF_IMPL(Image);
    FORWARD_REF_IMPL(Label);
    FORWARD_REF_IMPL(List);
    FORWARD_REF_IMPL(ScrollArea);
    FORWARD_REF_IMPL(Toggle);
    FORWARD_REF_IMPL(VerticalProgress);
    FORWARD_REF_IMPL(VerticalScrollBar);
    FORWARD_REF_IMPL(Widget);
    FORWARD_REF_IMPL(Window);

    void UIManager::RegisterFocusableWidget(Widget* widget)
    {
        if (!IsSingletonInitialzed())
            return;

        Instance().mFocusableWidgets.Add(Ref(widget));
    }

    void UIManager::UnregisterFocusableWidget(Widget* widget)
    {
        if (!IsSingletonInitialzed())
            return;

        Instance().mFocusableWidgets.RemoveFirst([&](auto& x) { return x == widget; });
    }

    UIManager::UIManager(RefCounter* refCounter):
        Singleton<UIManager>(refCounter)
    {
        mLog = mmake<LogStream>("UI");
        o2Debug.GetLog()->BindStream(mLog);
    }

    UIManager::~UIManager()
    {
        ClearStyle();
    }

    void UIManager::FocusWidget(const Ref<Widget>& widget)
    {
        if (mFocusedWidget == widget || (widget && !widget->IsFocusable()))
            return;

        if (mFocusedWidget)
        {
            auto lastFocusedWidget = mFocusedWidget;
            mFocusedWidget = nullptr;

            lastFocusedWidget->mIsFocused = false;

            auto focusedIdx = mLastFocusedWidgets.IndexOf(lastFocusedWidget);
            if (focusedIdx >= 0)
                mLastFocusedWidgets.RemoveAt(focusedIdx);
            else
                mLastUnfocusedWidgets.Add(lastFocusedWidget);

            if (lastFocusedWidget->mFocusedState)
                lastFocusedWidget->mFocusedState->SetState(false);
        }

        mFocusedWidget = widget;

        if (mFocusedWidget)
        {
            mFocusedWidget->mIsFocused = true;

            auto unfocusedIdx = mLastUnfocusedWidgets.IndexOf(mFocusedWidget);
            if (unfocusedIdx >= 0)
                mLastUnfocusedWidgets.RemoveAt(unfocusedIdx);
            else
                mLastFocusedWidgets.Add(mFocusedWidget);

            if (mFocusedWidget->mParentWidget)
                mFocusedWidget->mParentWidget.Lock()->OnChildFocused(mFocusedWidget);

            if (mFocusedWidget->mFocusedState)
                mFocusedWidget->mFocusedState->SetState(true);
        }
    }

    Ref<Widget> UIManager::GetFocusedWidget() const
    {
        return mFocusedWidget;
    }

    void UIManager::FocusNextWidget()
    {
        bool fnd = mFocusedWidget == nullptr;
        Ref<Widget> nextFocusingWidget;
        for (auto& widget : mFocusableWidgets)
        {
            if (!fnd)
            {
                if (widget == mFocusedWidget)
                    fnd = true;
            }
            else
            {
                if (widget->IsFocusable())
                {
                    nextFocusingWidget = widget;
                    break;
                }
            }
        }

        FocusWidget(nextFocusingWidget);
    }

    void UIManager::LoadStyle(const String& stylesPath)
    {
        ClearStyle();

        AssetRef<FolderAsset> folder(stylesPath);
        for (auto& subAsset : folder->GetChildrenAssets())
        {
            if (auto actorAsset = DynamicCast<ActorAsset>(subAsset.GetRef()))
                mStyleSamples.Add(actorAsset);
        }
    }

    void UIManager::SaveStyle(const String& stylesPath)
    {
        for (auto& asset : mStyleSamples)
        {
#if IS_EDITOR
            asset->SetEditorAsset(true);
#endif

            auto path = stylesPath + "/" + GetSmartName(asset->GetActor()->GetType().GetName()).ReplacedAll(":", "") + " " +
                asset->GetActor()->GetName() + ".proto";

            asset->Save(path);
        }
    }

    void UIManager::ClearStyle()
    {
        mStyleSamples.Clear();
    }

    void UIManager::AddWidgetStyle(const Ref<Widget>& widget, const String& style)
    {
        widget->SetName(style);
        mStyleSamples.Add(mmake<ActorAsset>(widget));
    }

    Ref<Widget> UIManager::CreateWidget(const Type& type, const String& style /*= "standard"*/)
    {
        auto sample = GetWidgetStyle(type, style);
        if (!sample)
            sample = GetWidgetStyle(type, "standard");

        Ref<Widget> res;

        if (sample)
            res = sample->CloneAsRef<Widget>();
        else
            res = DynamicCast<Widget>(type.CreateSampleRef());

        if (type != TypeOf(ContextMenu))
            res->SetEnabledForcible(true);

        return res;
    }

    Ref<Widget> UIManager::GetWidgetStyle(const Type& type, const String& style)
    {
        for (auto& styleWidget : mStyleSamples)
        {
            if (type == styleWidget->GetActor()->GetType())
            {
                if (style == styleWidget->GetActor()->GetName())
                    return DynamicCast<Widget>(styleWidget->GetActor());
            }
        }

        return nullptr;
    }

    Ref<Button> UIManager::CreateButton(const WString& caption, const Function<void()>& onClick /*= Function<void()>()*/,
                                        const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<Button>(style);
        res->caption = caption;
        res->name = caption + " button";
        res->onClick = onClick;
        return res;
    }

    Ref<Window> UIManager::CreateWindow(const WString& caption, const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<Window>(style);
        res->caption = caption;
        res->name = caption + " window";
        return res;
    }

    Ref<Label> UIManager::CreateLabel(const WString& text, const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<Label>(style);
        res->text = text;
        res->name = text + " label";
        res->horOverflow = Label::HorOverflow::Dots;
        return res;
    }

    Ref<HorizontalLayout> UIManager::CreateHorLayout()
    {
        auto res = mmake<HorizontalLayout>();
        res->name = "horizontal layout";
        *res->layout = WidgetLayout::BothStretch();
        return res;
    }

    Ref<VerticalLayout> UIManager::CreateVerLayout()
    {
        auto res = mmake<VerticalLayout>();
        res->name = "vertical layout";
        *res->layout = WidgetLayout::BothStretch();
        return res;
    }

    Ref<HorizontalProgress> UIManager::CreateHorProgress(const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<HorizontalProgress>(style);
        res->name = "horizontal progress";
        return res;
    }

    Ref<VerticalProgress> UIManager::CreateVerProgress(const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<VerticalProgress>(style);
        res->name = "vertical progress";
        return res;
    }

    Ref<HorizontalScrollBar> UIManager::CreateHorScrollBar(const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<HorizontalScrollBar>(style);
        res->name = "horizontal scroll bar";
        return res;
    }

    Ref<VerticalScrollBar> UIManager::CreateVerScrollBar(const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<VerticalScrollBar>(style);
        res->name = "vertical scroll bar";
        return res;
    }

    Ref<ScrollArea> UIManager::CreateScrollArea(const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<ScrollArea>(style);
        res->name = "scroll area";
        return res;
    }

    Ref<EditBox> UIManager::CreateEditBox(const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<EditBox>(style);
        res->name = "edit box";
        return res;
    }

    Ref<CustomList> UIManager::CreateCustomList(const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<CustomList>(style);
        res->name = "custom list";
        return res;
    }

    Ref<List> UIManager::CreateList(const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<List>(style);
        res->name = "list";
        return res;
    }

    Ref<CustomDropDown> UIManager::CreateCustomDropdown(const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<CustomDropDown>(style);
        res->name = "custom dropdown";
        return res;
    }

    Ref<DropDown> UIManager::CreateDropdown(const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<DropDown>(style);
        res->name = "dropdown";
        return res;
    }

    Ref<EditBoxDropDown> UIManager::CreateEditBoxDropDown(const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<EditBoxDropDown>(style);
        res->name = "editbox dropdown";
        return res;
    }

    Ref<Toggle> UIManager::CreateToggle(const WString& caption, const String& style /*= "standard"*/)
    {
        auto res = CreateWidget<Toggle>(style);
        res->caption = caption;
        res->name = caption + " toggle";
        return res;
    }

    Ref<Image> UIManager::CreateImage(const String& name)
    {
        auto res = mmake<Image>();
        res->name = name;
        res->SetImageName(name);
        return res;
    }

    void UIManager::DrawCurrentLayerTopWidgets()
    {
		auto currentLayer = o2Events.GetCurrentCursorAreaEventsLayer();
        
		auto& topWidgetsAtLayer = mTopWidgets[currentLayer];
        for (auto& widget : topWidgetsAtLayer)
            widget->Draw();

        topWidgetsAtLayer.Clear();

        if (PopupWidget::mVisiblePopup && PopupWidget::mVisiblePopup.Lock()->GetCurrentDrawingLayer() == currentLayer)
            PopupWidget::mVisiblePopup.Lock()->SpecialDraw();

        if (o2Input.IsKeyPressed(VK_TAB))
            FocusNextWidget();
    }

    void UIManager::Update()
    {
        PROFILE_SAMPLE_FUNC();

        for (auto& widget : mLastFocusedWidgets)
            widget->OnFocused();

        for (auto& widget : mLastUnfocusedWidgets)
            widget->OnUnfocused();

        mLastFocusedWidgets.Clear();
        mLastUnfocusedWidgets.Clear();
    }

    void UIManager::DrawWidgetAtTop(const Ref<Widget>& widget)
    {
        mTopWidgets[o2Events.GetCurrentCursorAreaEventsLayer()].Add(widget);
    }

    const Vector<AssetRef<ActorAsset>>& UIManager::GetWidgetStyles() const
    {
        return mStyleSamples;
    }

    void UIManager::TryLoadStyle()
    {
        if (o2Assets.IsAssetExist("ui_style.json"))
            LoadStyle("ui_style.json");
    }
}
