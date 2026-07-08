#pragma once

#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/IActorTransformViewer.h"

namespace Editor
{
    FORWARD_CLASS_REF(Vec2FProperty);
    FORWARD_CLASS_REF(Vec3FProperty);
    FORWARD_CLASS_REF(FloatProperty);

    // -------------------------------------
    // Default editor actor transform viewer
    // -------------------------------------
    class DefaultActorTransformViewer: public IActorTransformViewer
    {
    public:
        // Default constructor. Initializes data widget
        DefaultActorTransformViewer();

        // Virtual destructor
        ~DefaultActorTransformViewer();

        // Sets target actors
        void SetTargetActors(const Vector<Actor*>& actors) override;

        // Updates properties values
        void Refresh() override;

        IOBJECT(DefaultActorTransformViewer);

    protected:
        Vector<Actor*> mTargetActors;

        Ref<Vec3FProperty> mPositionProperty;
        Ref<Vec3FProperty> mPivotProperty;
        Ref<Vec3FProperty> mScaleProperty;
        Ref<Vec3FProperty> mSizeProperty;
        Ref<Vec3FProperty> mRotationProperty;
        Ref<Vec3FProperty> mShearProperty;

        bool               mLayoutEnabled = false;
        Ref<Spoiler>       mLayoutSpoiler;
        Ref<Vec2FProperty> mAnchorRightTopProperty;
        Ref<Vec2FProperty> mAnchorLeftBottomProperty;
        Ref<Vec2FProperty> mOffsetRightTopProperty;
        Ref<Vec2FProperty> mOffsetLeftBottomProperty;
        Ref<Vec2FProperty> mMinSizeProperty;
        Ref<Vec2FProperty> mMaxSizeProperty;
        Ref<Vec2FProperty> mWeightProperty;

        Vector<Ref<IPropertyField>> mAllProperties;

    protected:
        // Enable viewer event function
        void OnPropertiesEnabled() override;

        // Disable viewer event function
        void OnPropertiesDisabled() override;
        
        // Called when some property changed
        void OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser) override;

        // Called when some property change completed
        void OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::DefaultActorTransformViewer)
{
    BASE_CLASS(Editor::IActorTransformViewer);
}
END_META;
CLASS_FIELDS_META(Editor::DefaultActorTransformViewer)
{
    FIELD().PROTECTED().NAME(mTargetActors);
    FIELD().PROTECTED().NAME(mPositionProperty);
    FIELD().PROTECTED().NAME(mPivotProperty);
    FIELD().PROTECTED().NAME(mScaleProperty);
    FIELD().PROTECTED().NAME(mSizeProperty);
    FIELD().PROTECTED().NAME(mRotationProperty);
    FIELD().PROTECTED().NAME(mShearProperty);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mLayoutEnabled);
    FIELD().PROTECTED().NAME(mLayoutSpoiler);
    FIELD().PROTECTED().NAME(mAnchorRightTopProperty);
    FIELD().PROTECTED().NAME(mAnchorLeftBottomProperty);
    FIELD().PROTECTED().NAME(mOffsetRightTopProperty);
    FIELD().PROTECTED().NAME(mOffsetLeftBottomProperty);
    FIELD().PROTECTED().NAME(mMinSizeProperty);
    FIELD().PROTECTED().NAME(mMaxSizeProperty);
    FIELD().PROTECTED().NAME(mWeightProperty);
    FIELD().PROTECTED().NAME(mAllProperties);
}
END_META;
CLASS_METHODS_META(Editor::DefaultActorTransformViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetActors, const Vector<Actor*>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const Ref<IPropertyField>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChangeCompleted, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
}
END_META;
// --- END META ---
