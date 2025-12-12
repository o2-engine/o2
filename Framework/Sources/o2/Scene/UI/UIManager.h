#pragma once

#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Events/CursorAreaEventsListenersLayer.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Ref.h"

// User interfaces manager access macros
#define o2UI UIManager::Instance()

namespace o2
{
    FORWARD_CLASS_REF(Button);
    FORWARD_CLASS_REF(CustomDropDown);
    FORWARD_CLASS_REF(CustomList);
    FORWARD_CLASS_REF(DropDown);
    FORWARD_CLASS_REF(EditBox);
    FORWARD_CLASS_REF(EditBoxDropDown);
    FORWARD_CLASS_REF(HorizontalLayout);
    FORWARD_CLASS_REF(HorizontalProgress);
    FORWARD_CLASS_REF(HorizontalScrollBar);
    FORWARD_CLASS_REF(Image);
    FORWARD_CLASS_REF(Label);
    FORWARD_CLASS_REF(List);
    FORWARD_CLASS_REF(ScrollArea);
    FORWARD_CLASS_REF(Toggle);
    FORWARD_CLASS_REF(VerticalProgress);
    FORWARD_CLASS_REF(VerticalScrollBar);
    FORWARD_CLASS_REF(Widget);
    FORWARD_CLASS_REF(Window);

#undef CreateWindow

    // ------------------------------------------------
    // UI manager, contains all root widgets and styles
    // ------------------------------------------------
    class UIManager : public Singleton<UIManager>
    {
    public:
        // Default constructor
        UIManager(RefCounter* refCounter);

        // Destructor
        ~UIManager();

        // Loads widgets style
        void LoadStyle(const String& stylesPath);

        // Saves style
        void SaveStyle(const String& stylesPath);

        // Clears style widgets
        void ClearStyle();

        // Adds widget style
        void AddWidgetStyle(const Ref<Widget>& widget, const String& style);

        // Creates widget by style
        Ref<Widget> CreateWidget(const Type& type, const String& style = "standard");

        // Returns widget style by name
        Ref<Widget> GetWidgetStyle(const Type& type, const String& style);

        // Returns widget style by name
        template<typename _type>
        Ref<_type> GetWidgetStyle(const String& style);

        // Removes widget style
        template<typename _type>
        void RemoveWidgetStyle(const String& style);

        // Creates widget by style
        template<typename _type>
        Ref<_type> CreateWidget(const String& style = "standard");

        // Creates button by style
        Ref<Button> CreateButton(const WString& caption, const Function<void()>& onClick = Function<void()>(),
                                 const String& style = "standard");

        // Creates button by style
        Ref<Window> CreateWindow(const WString& caption, const String& style = "standard");

        // Creates label by style
        Ref<Label> CreateLabel(const WString& text, const String& style = "standard");

        // Creates horizontal layout
        Ref<HorizontalLayout> CreateHorLayout();

        // Creates vertical layout
        Ref<VerticalLayout> CreateVerLayout();

        // Creates horizontal progress bar by style
        Ref<HorizontalProgress> CreateHorProgress(const String& style = "standard");

        // CreatesCreates vertical progress bar by style
        Ref<VerticalProgress> CreateVerProgress(const String& style = "standard");

        // Creates horizontal scroll bar by style
        Ref<HorizontalScrollBar> CreateHorScrollBar(const String& style = "standard");

        // Creates vertical scroll bar by style
        Ref<VerticalScrollBar> CreateVerScrollBar(const String& style = "standard");

        // Creates scroll area
        Ref<ScrollArea> CreateScrollArea(const String& style = "standard");

        // Creates edit box
        Ref<EditBox> CreateEditBox(const String& style = "standard");

        // Creates list view
        Ref<CustomList> CreateCustomList(const String& style = "standard");

        // Creates text list
        Ref<List> CreateList(const String& style = "standard");

        // Creates custom dropdown
        Ref<CustomDropDown> CreateCustomDropdown(const String& style = "standard");

        // Creates text dropdown
        Ref<DropDown> CreateDropdown(const String& style = "standard");

        // Creates editbox dropdown
        Ref<EditBoxDropDown> CreateEditBoxDropDown(const String& style = "standard");

        // Creates toggle
        Ref<Toggle> CreateToggle(const WString& caption, const String& style = "standard");

        // Creates image
        Ref<Image> CreateImage(const String& name);

        // Sets widget as focused
        void FocusWidget(const Ref<Widget>& widget);

        // Returns current focused widget
        Ref<Widget> GetFocusedWidget() const;

        // Sets next widget focused
        void FocusNextWidget();

        // Draws context menus and top drawing widgets
        void DrawCurrentLayerTopWidgets();

        // Checks last focused and unfocused widget
        void Update();

        // Registering widget for drawing at top of all regular widgets
        void DrawWidgetAtTop(const Ref<Widget>& widget);

        // Returns all styles widgets
        const Vector<AssetRef<ActorAsset>>& GetWidgetStyles() const;

    protected:
        Ref<LogStream> mLog; // UI Log stream

        Ref<Widget>         mFocusedWidget;        // Current selected widget
        Vector<Ref<Widget>> mLastUnfocusedWidgets; // Widget that was unfocused on last frame
        Vector<Ref<Widget>> mLastFocusedWidgets;   // Widget that was focused on last frame
        Vector<Ref<Widget>> mFocusableWidgets;     // List of selectable widgets

        Map<WeakRef<CursorAreaEventListenersLayer>, Vector<Ref<Widget>>> mTopWidgets; // Top widgets, drawing after mScreenWidget 

        Vector<AssetRef<ActorAsset>> mStyleSamples; // Style widgets

    protected:
        // Registers focusable widget
        static void RegisterFocusableWidget(Widget* widget);

        // Unregisters focusable widget
        static void UnregisterFocusableWidget(Widget* widget);

    protected:
        // Tries to load style "ui_style.json"
        void TryLoadStyle();

        friend class BaseApplication;
        friend class CustomDropDown;
        friend class Integration;
        friend class Tree;
        friend class Widget;
		friend class Application;
    };

    template<typename _type>
    Ref<_type> UIManager::GetWidgetStyle(const String& style /*= "standard"*/)
    {
        for (auto& styleWidget : mStyleSamples)
        {
            if (TypeOf(_type) == styleWidget->GetActor()->GetType())
            {
                if (style == styleWidget->GetActor()->GetName())
                    return DynamicCast<_type>(styleWidget->GetActor());
            }
        }

        return nullptr;
    }

    template<typename _type>
    void UIManager::RemoveWidgetStyle(const String& style)
    {
        int removeIdx = -1;
        int i = 0;
        for (auto& styleWidget : mStyleSamples)
        {
            if (TypeOf(_type) == styleWidget->GetActor()->GetType())
            {
                if (style == styleWidget->GetActor()->GetName())
                {
                    removeIdx = i;
                    break;
                }
            }

            i++;
        }

        if (removeIdx != -1)
            mStyleSamples.RemoveAt(removeIdx);
    }

    template<typename _type>
    Ref<_type> UIManager::CreateWidget(const String& style /*= "standard"*/)
    {
        auto sample = GetWidgetStyle<_type>(style);
        if (!sample)
            sample = GetWidgetStyle<_type>("standard");

        Ref<_type> res;

        if (sample)
            res = sample->template CloneAsRef<_type>();
        else
            res = mmake<_type>();

        return res;
    }
}
