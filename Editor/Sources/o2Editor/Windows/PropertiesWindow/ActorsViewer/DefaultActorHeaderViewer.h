#pragma once

#include "o2/Assets/Types/ActorAsset.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/IActorHeaderViewer.h"

namespace o2
{
    class Toggle;
    class EditBox;
    class Button;
}

namespace Editor
{
    using namespace o2;

    FORWARD_CLASS_REF(TagsProperty);
    FORWARD_CLASS_REF(SceneLayerRefProperty);
    FORWARD_CLASS_REF(BooleanProperty);
    FORWARD_CLASS_REF(StringProperty);
    FORWARD_CLASS_REF(AssetProperty);
    FORWARD_CLASS_REF(FloatProperty);

    // ---------------------------
    // Default actor header viewer
    // ---------------------------
    class DefaultActorHeaderViewer: public IActorHeaderViewer
    {
    public:
        // Default constructor
        DefaultActorHeaderViewer();

        // Virtual destructor
        ~DefaultActorHeaderViewer();

        // Sets target actors
        void SetTargetActors(const Vector<Actor*>& actors) override;

        // Returns data widget
        Ref<Widget> GetWidget() const override;

        // Updates properties values
        void Refresh() override;

        IOBJECT(DefaultActorHeaderViewer);

    public: 
        Vector<Actor*> mActors;

        Ref<Widget> mDataView;

        Ref<BooleanProperty>       mEnableProperty;
        Ref<StringProperty>        mNameProperty;
        Ref<BooleanProperty>       mLockProperty;
        Ref<AssetProperty>         mPrototypeProperty;
        Ref<TagsProperty>          mTagsProperty;
        Ref<SceneLayerRefProperty> mLayerProperty;
        Ref<FloatProperty>         mDepthProperty;

        Ref<Button> mPrototypeApplyBtn;
        Ref<Button> mPrototypeRevertBtn;
        Ref<Button> mPrototypeBreakBtn;

    protected:
        // Enable viewer event function
        void OnPropertiesEnabled() override;

        // Disable viewer event function
        void OnPropertiesDisabled() override;

        // Refreshes layer property
        void RefreshLayer();

        // Called when apply button pressed, and applies changes to prototype
        void OnApplyPrototypePressed();

        // Called when revert button pressed and reverts actor to prototype
        void OnRevertPrototypePressed();

        // Called when break prototype buttons pressed and break prototype link
        void OnBreakPrototypePressed();

        // Called when selected inherit from parent layer
        void OnSelectedInheritedLayer();

        // Returns actor applying roots
        Vector<Actor*> GetRootApplyActors();

        // Called when some property changed, stores action for undo
        void OnPropertyChanged(const String& path, const Vector<DataDocument>& prevValue, 
                               const Vector<DataDocument>& newValue);
    };
}
// --- META ---

CLASS_BASES_META(Editor::DefaultActorHeaderViewer)
{
    BASE_CLASS(Editor::IActorHeaderViewer);
}
END_META;
CLASS_FIELDS_META(Editor::DefaultActorHeaderViewer)
{
    FIELD().PUBLIC().NAME(mActors);
    FIELD().PUBLIC().NAME(mDataView);
    FIELD().PUBLIC().NAME(mEnableProperty);
    FIELD().PUBLIC().NAME(mNameProperty);
    FIELD().PUBLIC().NAME(mLockProperty);
    FIELD().PUBLIC().NAME(mPrototypeProperty);
    FIELD().PUBLIC().NAME(mTagsProperty);
    FIELD().PUBLIC().NAME(mLayerProperty);
    FIELD().PUBLIC().NAME(mDepthProperty);
    FIELD().PUBLIC().NAME(mPrototypeApplyBtn);
    FIELD().PUBLIC().NAME(mPrototypeRevertBtn);
    FIELD().PUBLIC().NAME(mPrototypeBreakBtn);
}
END_META;
CLASS_METHODS_META(Editor::DefaultActorHeaderViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetActors, const Vector<Actor*>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, GetWidget);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, RefreshLayer);
    FUNCTION().PROTECTED().SIGNATURE(void, OnApplyPrototypePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRevertPrototypePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnBreakPrototypePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectedInheritedLayer);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Actor*>, GetRootApplyActors);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
}
END_META;
// --- END META ---
