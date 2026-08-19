#pragma once

#include "o2/Render/Video.h"
#include "o2/Scene/Component.h"

namespace o2
{
    // ---------------
    // Video component
    // ---------------
    class VideoComponent: public Component, public Video
    {
    public:
        PROPERTIES(VideoComponent);

    public:
        // Default constructor @SCRIPTABLE
        VideoComponent();

        // Constructor from video asset
        explicit VideoComponent(const AssetRef<VideoAsset>& video);

        // Copy-constructor
        VideoComponent(const VideoComponent& other);

        // Destructor
        ~VideoComponent();

        // Assign operator
        VideoComponent& operator=(const VideoComponent& other);

        // Sets actor's size as video size
        void FitActorByVideo() const;

        // Returns true if point is under drawable
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Returns name of component icon
        static String GetIcon();

        // Dynamic cast to RefCounterable via Component
        static Ref<RefCounterable> CastToRefCounterable(const Ref<VideoComponent>& ref);

        SERIALIZABLE(VideoComponent);
        CLONEABLE_REF(VideoComponent);
        REF_COUNTERABLE_IMPL(Component, Video);

        using Video::onDraw;

    protected:
        // Draws video
        void OnDraw() override;

        // Starts playback if play on awake is enabled
        void OnStart() override;

        // Advances playback
        void OnUpdate(float dt) override;

        // Called when actor's transform was changed
        void OnTransformUpdated() override;

        // Calling when deserializing
        void OnDeserialized(const DataValue& node) override;

        // Calling when serializing
        void OnSerialize(DataValue& node) const override;

        // Beginning serialization delta callback
        void OnSerializeDelta(DataValue& node, const IObject& origin) const override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::VideoComponent)
{
    BASE_CLASS(o2::Component);
    BASE_CLASS(o2::Video);
}
END_META;
CLASS_FIELDS_META(o2::VideoComponent)
{
}
END_META;
CLASS_METHODS_META(o2::VideoComponent)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AssetRef<VideoAsset>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const VideoComponent&);
    FUNCTION().PUBLIC().SIGNATURE(void, FitActorByVideo);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<VideoComponent>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStart);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerializeDelta, DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
}
END_META;
// --- END META ---
