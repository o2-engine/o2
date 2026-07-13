#include "o2Editor/stdafx.h"
#include "WindowsLayout.h"

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/DockWindowPlace.h"
#include "o2Editor/Windows/DockableWindow.h"
#include "o2Editor/Windows/IEditorWindow.h"
#include "o2Editor/Windows/WindowsManager.h"

namespace Editor
{
    void WindowsLayout::WindowDockPlaceInfo::RetrieveLayout(const Ref<Widget>& widget)
    {
        if (widget->GetType() == TypeOf(DockWindowPlace))
        {
            anchors.Set(widget->layout->GetAnchorLeft(), widget->layout->GetAnchorTop(),
                        widget->layout->GetAnchorRight(), widget->layout->GetAnchorBottom());

            for (auto& child : widget->GetChildWidgets())
            {
                if (child->GetType() == TypeOf(DockWindowPlace))
                {
                    childs.Add(WindowDockPlaceInfo());
                    childs[childs.Count() - 1].RetrieveLayout(child);
                }
                else if (child->GetType() == TypeOf(DockableWindow) && child->IsEnabled())
                {
                    windows.Add(child->name);

                    if (DynamicCast<DockableWindow>(child)->IsTabActive())
                        active = child->name;
                }
            }
        }
    }

    bool WindowsLayout::WindowDockPlaceInfo::operator==(const WindowDockPlaceInfo& other) const
    {
        return anchors == other.anchors && childs == other.childs && windows == other.windows;
    }

    bool WindowsLayout::operator==(const WindowsLayout& other) const
    {
        return mainDock == other.mainDock && windows == other.windows;
    }

    void WindowsLayout::RestoreDock(WindowDockPlaceInfo& dockDef, DockWindowPlace& dockWidget)
    {
        PushEditorScopeOnStack scope;

        Vector<Ref<DockWindowPlace>> childDockWidgets;
        for (auto& child : dockDef.childs)
        {
            dockWidget.interactable = false;

            auto newDock = mmake<DockWindowPlace>();
            newDock->name = "dock place";
            dockWidget.AddChild(newDock);

            *newDock->layout = WidgetLayout::BothStretch();
            newDock->layout->anchorMin = child.anchors.LeftBottom();
            newDock->layout->anchorMax = child.anchors.RightTop();
            childDockWidgets.Add(newDock);
        }

        int idx = 0;
        for (auto& child : dockDef.childs)
        {
            auto newDock = childDockWidgets[idx++];

            if ((child.anchors.left < 1.0f && child.anchors.left > 0.0f) ||
                (child.anchors.right < 1.0f && child.anchors.right > 0.0f))
            {
                auto neighborMin = childDockWidgets.FindOrDefault([&](auto& x) {
                    return Math::Equals(x->layout->GetAnchorRight(), newDock->layout->GetAnchorLeft()) && x != newDock;
                });

                auto neighborMax = childDockWidgets.FindOrDefault([&](auto& x) {
                    return Math::Equals(x->layout->GetAnchorLeft(), newDock->layout->GetAnchorRight()) && x != newDock;
                });

                newDock->SetResizibleDir(TwoDirection::Horizontal, 1.5f, neighborMin, neighborMax);

                if (neighborMin)
                    neighborMin->SetResizibleDir(TwoDirection::Horizontal, 1.5f, neighborMin->mNeighborMin.Lock(), newDock);

                if (neighborMax)
                    neighborMax->SetResizibleDir(TwoDirection::Horizontal, 1.5f, newDock, neighborMax->mNeighborMax.Lock());
            }

            if ((child.anchors.top < 1.0f && child.anchors.top > 0.0f) ||
                (child.anchors.bottom < 1.0f && child.anchors.bottom > 0.0f))
            {
                auto neighborMin = childDockWidgets.FindOrDefault([&](auto& x) {
                    return Math::Equals(x->layout->GetAnchorTop(), newDock->layout->GetAnchorBottom()) && x != newDock;
                });

                auto neighborMax = childDockWidgets.FindOrDefault([&](auto& x) {
                    return Math::Equals(x->layout->GetAnchorBottom(), newDock->layout->GetAnchorTop()) && x != newDock;
                });

                newDock->SetResizibleDir(TwoDirection::Vertical, 1.5f, neighborMin, neighborMax);

                if (neighborMin)
                    neighborMin->SetResizibleDir(TwoDirection::Vertical, 1.5f, neighborMin->mNeighborMin.Lock(), newDock);

                if (neighborMax)
                    neighborMax->SetResizibleDir(TwoDirection::Vertical, 1.5f, newDock, neighborMax->mNeighborMax.Lock());
            }
        }

        idx = 0;
        for (auto& child : dockDef.childs)
        {
            auto newDock = childDockWidgets[idx++];
            RestoreDock(child, *newDock);
        }

        Ref<DockableWindow> activeTabWindow;
        for (auto& wnd : dockDef.windows)
        {
            auto window = o2EditorWindows.mEditorWindows.FindOrDefault([&](auto& x) {
                return x->mWindow->GetName() == wnd; 
            });

            if (window)
            {
                auto dockWnd = DynamicCast<DockableWindow>(window->mWindow);

                dockWnd->mNonDockSize = dockWnd->layout->size2D;
                dockWidget.AddChild(dockWnd);
                *dockWnd->layout = WidgetLayout::BothStretch();
                dockWnd->SetDocked(true);
                dockWnd->Show();

                if (dockDef.active == wnd)
                    activeTabWindow = dockWnd;
            }
        }

        dockWidget.ArrangeChildWindows();

        if (activeTabWindow)
            activeTabWindow->SetTabActive();

        CleanEmptyDocks(dockWidget);
    }

    void WindowsLayout::CleanEmptyDocks(DockWindowPlace& dockPlace)
    {
        PushEditorScopeOnStack scope;

        auto childs = dockPlace.GetChildWidgets();
        for (auto& child : childs)
        {
            if (auto dockChild = DynamicCast<DockWindowPlace>(child))
            {
                CleanEmptyDocks(*dockChild);

                if (dockChild->GetChildWidgets().IsEmpty())
                    dockPlace.RemoveChild(child);
            }
        }
    }

}
// --- META ---

DECLARE_CLASS(Editor::WindowsLayout, Editor__WindowsLayout);

DECLARE_CLASS(Editor::WindowsLayout::WindowDockPlaceInfo, Editor__WindowsLayout__WindowDockPlaceInfo);
// --- END META ---
