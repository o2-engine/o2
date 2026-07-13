#include "o2Editor/stdafx.h"

#include "AnimationKeyDragHandle.h"
#include "ITrackControl.h"

namespace Editor
{

    AnimationKeyDragHandle::AnimationKeyDragHandle(RefCounter* refCounter):
        DragHandle(refCounter)
    {}

    AnimationKeyDragHandle::AnimationKeyDragHandle(RefCounter* refCounter,
                                                   const Ref<Sprite>& regular,
                                                   const Ref<Sprite>& hover /*= nullptr*/,
                                                   const Ref<Sprite>& pressed /*= nullptr*/,
                                                   const Ref<Sprite>& selected /*= nullptr*/,
                                                   const Ref<Sprite>& selectedHovered /*= nullptr*/,
                                                   const Ref<Sprite>& selectedPressed /*= nullptr*/):
        DragHandle(refCounter, regular, hover, pressed, selected, selectedHovered, selectedPressed)
    {}

    AnimationKeyDragHandle::AnimationKeyDragHandle(RefCounter* refCounter, const AnimationKeyDragHandle& other):
        DragHandle(refCounter, other)
    {}

    AnimationKeyDragHandle::AnimationKeyDragHandle(const AnimationKeyDragHandle& other) :
        AnimationKeyDragHandle(nullptr, other)
    {}

    AnimationKeyDragHandle::~AnimationKeyDragHandle()
    {}

    void AnimationKeyDragHandle::Draw()
    {
        DragHandle::Draw();
    }

    Editor::AnimationKeyDragHandle& AnimationKeyDragHandle::operator=(const AnimationKeyDragHandle& other)
    {
        DragHandle::operator=(other);
        return *this;
    }
}
// --- META ---

DECLARE_CLASS(Editor::AnimationKeyDragHandle, Editor__AnimationKeyDragHandle);
// --- END META ---
