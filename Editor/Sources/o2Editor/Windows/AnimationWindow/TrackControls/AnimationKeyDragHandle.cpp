#include "o2Editor/stdafx.h"

#include "AnimationKeyDragHandle.h"
#include "ITrackControl.h"

namespace Editor
{

    AnimationKeyDragHandle::AnimationKeyDragHandle(RefCounter* refCounter):
        WidgetDragHandle(refCounter)
    {}

    AnimationKeyDragHandle::AnimationKeyDragHandle(RefCounter* refCounter,
                                                   const Ref<Sprite>& regular,
                                                   const Ref<Sprite>& hover /*= nullptr*/,
                                                   const Ref<Sprite>& pressed /*= nullptr*/, 
                                                   const Ref<Sprite>& selected /*= nullptr*/, 
                                                   const Ref<Sprite>& selectedHovered /*= nullptr*/, 
                                                   const Ref<Sprite>& selectedPressed /*= nullptr*/):
        WidgetDragHandle(refCounter, regular, hover, pressed, selected, selectedHovered, selectedPressed)
    {}

    AnimationKeyDragHandle::AnimationKeyDragHandle(RefCounter* refCounter, const AnimationKeyDragHandle& other):
        WidgetDragHandle(refCounter, other)
    {}

    AnimationKeyDragHandle::AnimationKeyDragHandle(const AnimationKeyDragHandle& other) :
        AnimationKeyDragHandle(nullptr, other)
    {}

    AnimationKeyDragHandle::~AnimationKeyDragHandle()
    {}

#undef DrawText
    void AnimationKeyDragHandle::Draw()
    {
        WidgetDragHandle::Draw();

        //o2Debug.DrawText(GetScreenPosition(), String(mIsClipped));
    }

    String AnimationKeyDragHandle::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    Editor::AnimationKeyDragHandle& AnimationKeyDragHandle::operator=(const AnimationKeyDragHandle& other)
    {
        WidgetDragHandle::operator=(other);
        return *this;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::AnimationKeyDragHandle>);
// --- META ---

DECLARE_CLASS(Editor::AnimationKeyDragHandle, Editor__AnimationKeyDragHandle);
// --- END META ---
