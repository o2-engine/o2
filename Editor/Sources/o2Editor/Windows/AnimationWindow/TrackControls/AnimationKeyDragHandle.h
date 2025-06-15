#pragma once

#include "o2/Utils/Editor/DragHandle.h"

using namespace o2;

namespace Editor
{
    class ITrackControl;

    class AnimationKeyDragHandle : public WidgetDragHandle
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
        AnimationKeyDragHandle(RefCounter* refCounter, const Ref<Sprite>& regular, const Ref<Sprite>& hover = nullptr, const Ref<Sprite>& pressed = nullptr,
                               const Ref<Sprite>& selected = nullptr, const Ref<Sprite>& selectedHovered = nullptr,
                               const Ref<Sprite>& selectedPressed = nullptr);

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

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(AnimationKeyDragHandle);
    };
}
// --- META ---

CLASS_BASES_META(Editor::AnimationKeyDragHandle)
{
    BASE_CLASS(o2::WidgetDragHandle);
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
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Ref<Sprite>&, const Ref<Sprite>&, const Ref<Sprite>&, const Ref<Sprite>&, const Ref<Sprite>&, const Ref<Sprite>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AnimationKeyDragHandle&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const AnimationKeyDragHandle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
}
END_META;
// --- END META ---
