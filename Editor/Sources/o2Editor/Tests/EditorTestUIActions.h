#pragma once

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    class Widget;
    class Actor;
}

namespace Editor::Tests
{
    // ---------------- UI search ----------------

    // Searches in editor UI starting from EditorUIRoot's root widget.
    // Path uses "/" as separator. Searches both childWidgets and internalWidgets.
    o2::Ref<o2::Widget> FindWidgetByPath(const o2::String& path);

    // Searches whole editor UI tree (children+internal recursively) by name.
    o2::Ref<o2::Widget> FindWidgetByName(const o2::String& name);

    // Searches whole editor UI tree by type name (matches o2::Widget subclasses).
    o2::Vector<o2::Ref<o2::Widget>> FindWidgetsByType(const o2::String& typeName);

    // Returns a multi-line string representation of UI tree starting from EditorUIRoot.
    o2::String DumpUITree();

    // ---------------- Scene search ----------------

    // Returns a multi-line string of scene actors hierarchy.
    o2::String DumpSceneTree();

    // ---------------- Input emulation ----------------

    // Returns true on success. Computes click point as the center of widget's world rect.
    // Emits move + press in current frame; release should be emitted on next frame
    // (the runner inserts a 1-frame gap between steps automatically).
    bool ClickWidget(const o2::Ref<o2::Widget>& widget);

    // Two-stage click for sequencing across frames.
    bool BeginClickWidget(const o2::Ref<o2::Widget>& widget); // move + press
    void EndClickWidget();                                    // release

    void MouseMoveTo(const o2::Vec2F& pos);
    void MouseDown(const o2::Vec2F& pos);
    void MouseUp();

    // Types text by emulating sequential key presses for ASCII characters.
    void TypeText(const o2::String& text);

    // Single key press+release.
    void KeyPress(int keyCode);
}

#endif
