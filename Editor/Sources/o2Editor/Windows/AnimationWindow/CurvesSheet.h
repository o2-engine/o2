#pragma once

#include "o2Editor/UI/CurveEditor/CurvesEditor.h"

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(AnimationWindow);

    // ---------------------------------
    // Curves sheet. Shows curves editor
    // ---------------------------------
    class CurvesSheet : public Widget
    {
    public:
        // Default constructor
        CurvesSheet(RefCounter* refCounter);

        // Copy-constructor
        CurvesSheet(RefCounter* refCounter, const CurvesSheet& other);

        // Destructor
        ~CurvesSheet();

        // Copy-operator
        CurvesSheet& operator=(const CurvesSheet& other);

        // Sets animation and updates tree structure
        void SetAnimation(const Ref<AnimationClip>& animation);

        // Generates new color for curves and transfers to the tree
        void UpdateCurvesColors();

        // Called when animation changed, checks count of animation tracks, updates curves
        void OnAnimationChanged();

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(CurvesSheet);
        CLONEABLE_REF(CurvesSheet);

    private:
        Ref<CurvesEditor> mCurvesEditor; // Curves editor

        WeakRef<AnimationWindow> mAnimationWindow; // Animation window

        bool mEditorViewLock = false; // It is used to prevent handling editor's camera, when timeline view changing 

    private:
        // Initializes curves edit widget
        void InitializeControls();

        // Called when timeline view has changed, updates horizontal view position and scale
        void SetCameraAsTimelineView();

        // Called when curve editor view has changed, updates timeline scroll position
        void OnEditorViewChanged();

        friend class AnimationWindow;
    };
}
// --- META ---

CLASS_BASES_META(Editor::CurvesSheet)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::CurvesSheet)
{
    FIELD().PRIVATE().NAME(mCurvesEditor);
    FIELD().PRIVATE().NAME(mAnimationWindow);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mEditorViewLock);
}
END_META;
CLASS_METHODS_META(Editor::CurvesSheet)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const CurvesSheet&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAnimation, const Ref<AnimationClip>&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateCurvesColors);
    FUNCTION().PUBLIC().SIGNATURE(void, OnAnimationChanged);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeControls);
    FUNCTION().PRIVATE().SIGNATURE(void, SetCameraAsTimelineView);
    FUNCTION().PRIVATE().SIGNATURE(void, OnEditorViewChanged);
}
END_META;
// --- END META ---
