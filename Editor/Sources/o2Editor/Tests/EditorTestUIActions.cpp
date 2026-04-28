#include "EditorTestUIActions.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

#include "o2Editor/UIRoot.h"

#include "o2/Application/Input.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorTransform.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Reflection/Type.h"

namespace Editor::Tests
{
    using namespace o2;

    // ---------------- helpers ----------------

    static void CollectAllWidgetsRec(const Ref<Widget>& w, Vector<Ref<Widget>>& out)
    {
        if (!w)
            return;
        out.Add(w);
        for (auto& c : w->GetChildWidgets())
            CollectAllWidgetsRec(c, out);
        // Internal widgets — searched too.
        // Widget exposes internal widgets via FindInternalWidget but no direct getter; use a name-walk.
        // Most editor widgets are reachable as children; internals are added via AddInternalWidget.
        // Skip: traversal of internals would require friend access. Walking children covers most cases.
    }

    static Ref<Widget> EditorRoot()
    {
        // Singleton in Editor namespace.
        return Editor::UIRoot::Instance().GetRootWidget();
    }

    static Ref<Widget> SearchByNameRec(const Ref<Widget>& w, const String& name)
    {
        if (!w)
            return nullptr;
        if (w->GetName() == name)
            return w;
        for (auto& c : w->GetChildWidgets())
        {
            if (auto found = SearchByNameRec(c, name))
                return found;
        }
        // Try internal widget search.
        if (auto found = w->FindInternalWidget(name))
            return found;
        return nullptr;
    }

    static void SearchByTypeRec(const Ref<Widget>& w, const String& typeName, Vector<Ref<Widget>>& out)
    {
        if (!w)
            return;
        if (w->GetType().GetName() == typeName)
            out.Add(w);
        for (auto& c : w->GetChildWidgets())
            SearchByTypeRec(c, typeName, out);
    }

    static void DumpRec(const Ref<Widget>& w, int indent, String& out)
    {
        if (!w)
            return;
        for (int i = 0; i < indent; ++i)
            out += "  ";
        out += String("- ") + w->GetType().GetName() + " '" + w->GetName() + "'";
        out += String(" enabled=") + (w->IsEnabledInHierarchy() ? "1" : "0");
        out += "\n";
        for (auto& c : w->GetChildWidgets())
            DumpRec(c, indent + 1, out);
    }

    static void DumpSceneRec(const Ref<Actor>& a, int indent, String& out)
    {
        if (!a)
            return;
        for (int i = 0; i < indent; ++i)
            out += "  ";
        out += String("- ") + a->GetType().GetName() + " '" + a->GetName() + "'";
        out += String(" enabled=") + (a->IsEnabledInHierarchy() ? "1" : "0");
        out += "\n";
        for (auto& c : a->GetChildren())
            DumpSceneRec(c, indent + 1, out);
    }

    // ---------------- API ----------------

    Ref<Widget> FindWidgetByPath(const String& path)
    {
        auto root = EditorRoot();
        if (!root)
            return nullptr;
        if (path.IsEmpty())
            return root;
        // Try children path; fall back to internal widgets path.
        if (auto w = root->GetChildWidget(path))
            return w;
        return root->GetInternalWidget(path);
    }

    Ref<Widget> FindWidgetByName(const String& name)
    {
        auto root = EditorRoot();
        if (!root)
            return nullptr;
        if (root->GetName() == name)
            return root;
        return SearchByNameRec(root, name);
    }

    Vector<Ref<Widget>> FindWidgetsByType(const String& typeName)
    {
        Vector<Ref<Widget>> result;
        auto root = EditorRoot();
        if (!root)
            return result;
        SearchByTypeRec(root, typeName, result);
        return result;
    }

    String DumpUITree()
    {
        String out;
        out += "--- Editor UI tree ---\n";
        auto root = EditorRoot();
        DumpRec(root, 0, out);
        return out;
    }

    String DumpSceneTree()
    {
        String out;
        out += "--- Scene tree ---\n";
        for (auto& a : o2Scene.GetRootActors())
            DumpSceneRec(a, 0, out);
        return out;
    }

    static Vec2F WidgetCenter(const Ref<Widget>& widget)
    {
        if (!widget)
            return Vec2F();
        RectF r = widget->transform->GetWorldRect();
        return r.Center();
    }

    bool ClickWidget(const Ref<Widget>& widget)
    {
        if (!widget)
            return false;
        Vec2F c = WidgetCenter(widget);
        o2Input.OnCursorMoved(c, 0, true);
        o2Input.OnCursorPressed(c, 0);
        o2Input.OnCursorReleased(0);
        return true;
    }

    bool BeginClickWidget(const Ref<Widget>& widget)
    {
        if (!widget)
            return false;
        Vec2F c = WidgetCenter(widget);
        o2Input.OnCursorMoved(c, 0, true);
        o2Input.OnCursorPressed(c, 0);
        return true;
    }

    void EndClickWidget()
    {
        o2Input.OnCursorReleased(0);
    }

    void MouseMoveTo(const Vec2F& pos)
    {
        o2Input.OnCursorMoved(pos, 0, true);
    }

    void MouseDown(const Vec2F& pos)
    {
        o2Input.OnCursorPressed(pos, 0);
    }

    void MouseUp()
    {
        o2Input.OnCursorReleased(0);
    }

    void TypeText(const String& text)
    {
        for (int i = 0; i < text.Length(); ++i)
        {
            char ch = text[i];
            // Emulate by raw key code; works for ASCII printable.
            o2Input.OnKeyPressed((KeyboardKey)ch);
            o2Input.OnKeyReleased((KeyboardKey)ch);
        }
    }

    void KeyPress(int keyCode)
    {
        o2Input.OnKeyPressed((KeyboardKey)keyCode);
        o2Input.OnKeyReleased((KeyboardKey)keyCode);
    }
}

#endif
