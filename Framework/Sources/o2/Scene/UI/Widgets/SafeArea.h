#pragma once

#include "o2/Scene/UI/Widget.h"

namespace o2
{
    class CameraActor;

    // ---------------------------------------------------------------------------------------------------------------
    // Safe area widget. Lives inside a camera actor and takes the part of the camera's frame the device keeps clear:
    // no rounded corners, notch or island, status bar or home indicator (Integration::GetSafeAreaInsets). Put the
    // interface people touch inside it, the graphics that fill the screen outside. It lays itself out every frame the
    // scene updates, so its anchors and offsets are its own only where there is no camera above (a prototype opened alone)
    // ---------------------------------------------------------------------------------------------------------------
    class SafeArea: public Widget
    {
    public:
        bool left = true;   // Keeps clear of the left inset @SERIALIZABLE @SCRIPTABLE
        bool right = true;  // Keeps clear of the right inset @SERIALIZABLE @SCRIPTABLE
        bool top = true;    // Keeps clear of the top inset @SERIALIZABLE @SCRIPTABLE
        bool bottom = true; // Keeps clear of the bottom inset @SERIALIZABLE @SCRIPTABLE

        // The larger inset of each axis on both sides: the area stays centred in the camera's frame, for a design placed
        // from the frame's centre (effects that count from it keep hitting their targets) @SERIALIZABLE @SCRIPTABLE
        bool symmetric = false;

    public:
        // Default constructor @SCRIPTABLE
        explicit SafeArea(RefCounter* refCounter);

        // Copy-constructor
        SafeArea(RefCounter* refCounter, const SafeArea& other);

        // Copy operator
        SafeArea& operator=(const SafeArea& other);

        // Lays the area out by its camera and the safe area of now @SCRIPTABLE
        void Fit();

        // Returns the camera actor the area is inside of, null when there is none
        Ref<CameraActor> GetCamera() const;

        // Returns the world rectangle of the area: the camera's frame without the insets @SCRIPTABLE
        RectF GetSafeWorldRect() const;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        // Returns the icon of the type in the editor
        static String GetEditorIcon();

        SERIALIZABLE(SafeArea);
        CLONEABLE_REF(SafeArea);

    protected:
        bool mWarnedNoCamera = false; // The missing camera was reported once

    protected:
        // Fits the area before the children update
        void OnUpdate(float dt) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::SafeArea)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(o2::SafeArea)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(left);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(right);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(top);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(bottom);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(symmetric);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mWarnedNoCamera);
}
END_META;
CLASS_METHODS_META(o2::SafeArea)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SafeArea&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Fit);
    FUNCTION().PUBLIC().SIGNATURE(Ref<CameraActor>, GetCamera);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(RectF, GetSafeWorldRect);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
}
END_META;
// --- END META ---
