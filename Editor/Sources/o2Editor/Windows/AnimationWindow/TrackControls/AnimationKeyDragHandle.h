#pragma once

#include "o2/Utils/Editor/DragHandle.h"

using namespace o2;

namespace Editor
{
    class ITrackControl;

    // Plain drag handle, not a widget: thousands of key handles must not participate
    // in the widget layout and update passes
    class AnimationKeyDragHandle : public DragHandle
    {
    public:
        String trackPath;

        WeakRef<IAnimationTrack> track;
        WeakRef<ITrackControl>   trackControl;

        UInt64 keyUid = 0;

        bool isMapping = false;

    public:
        // Default constructor
        AnimationKeyDragHandle(RefCounter* refCounter);

        // Constructor with views
        AnimationKeyDragHandle(RefCounter* refCounter, const Ref<IRectDrawable>& regular, const Ref<IRectDrawable>& hover = nullptr, const Ref<IRectDrawable>& pressed = nullptr,
                               const Ref<IRectDrawable>& selected = nullptr, const Ref<IRectDrawable>& selectedHovered = nullptr,
                               const Ref<IRectDrawable>& selectedPressed = nullptr);

        // Copy-constructor
        AnimationKeyDragHandle(RefCounter* refCounter, const AnimationKeyDragHandle& other);

        // Copy-constructor
        AnimationKeyDragHandle(const AnimationKeyDragHandle& other);

        // Destructor
        ~AnimationKeyDragHandle();

        // Copy-operator
        AnimationKeyDragHandle& operator=(const AnimationKeyDragHandle& other);

        // Draws handle
        void Draw() override;

        SERIALIZABLE(AnimationKeyDragHandle);
    };
}
// --- META ---

CLASS_BASES_META(Editor::AnimationKeyDragHandle)
{
    BASE_CLASS(o2::DragHandle);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationKeyDragHandle)
{
    FIELD().PUBLIC().NAME(trackPath);
    FIELD().PUBLIC().NAME(track);
    FIELD().PUBLIC().NAME(trackControl);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(keyUid);
    FIELD().PUBLIC().DEFAULT_VALUE(false).NAME(isMapping);
}
END_META;
CLASS_METHODS_META(Editor::AnimationKeyDragHandle)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AnimationKeyDragHandle&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const AnimationKeyDragHandle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
}
END_META;
// --- END META ---
