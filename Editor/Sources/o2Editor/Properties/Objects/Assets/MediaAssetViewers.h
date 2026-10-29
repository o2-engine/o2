#pragma once

#include "o2Editor/Properties/Objects/DefaultObjectPropertiesViewer.h"

using namespace o2;

namespace o2
{
    class Label;
}

namespace Editor
{
    FORWARD_CLASS_REF(SoundPlayerView);
    FORWARD_CLASS_REF(VideoView);

    // ---------------------------------------------------------
    // Sound asset viewer: duration, channels and the playback
    // ---------------------------------------------------------
    class SoundAssetViewer: public DefaultObjectPropertiesViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        // Returns the player
        const Ref<SoundPlayerView>& GetPlayer() const;

        IOBJECT(SoundAssetViewer);

    protected:
        Ref<Label>           mInfoLabel; // Duration and channels
        Ref<SoundPlayerView> mPlayer;    // Playback

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when the viewer is freed: stops the sound
        void OnFree() override;
    };

    // ---------------------------------------------------------------------------
    // Video asset viewer: size, frame rate and duration, the player, the preview
    // ---------------------------------------------------------------------------
    class VideoAssetViewer: public DefaultObjectPropertiesViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        // Returns the player
        const Ref<VideoView>& GetPlayer() const;

        IOBJECT(VideoAssetViewer);

    protected:
        Ref<Label>     mInfoLabel; // Size, frame rate and duration
        Ref<VideoView> mPlayer;    // Playback
        WeakRef<Asset> mAsset;     // Shown asset, for the preview window

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when the viewer is freed: pauses the video
        void OnFree() override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::SoundAssetViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::SoundAssetViewer)
{
    FIELD().PROTECTED().NAME(mInfoLabel);
    FIELD().PROTECTED().NAME(mPlayer);
}
END_META;
CLASS_METHODS_META(Editor::SoundAssetViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<SoundPlayerView>&, GetPlayer);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp2);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFree);
}
END_META;

CLASS_BASES_META(Editor::VideoAssetViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::VideoAssetViewer)
{
    FIELD().PROTECTED().NAME(mInfoLabel);
    FIELD().PROTECTED().NAME(mPlayer);
    FIELD().PROTECTED().NAME(mAsset);
}
END_META;
CLASS_METHODS_META(Editor::VideoAssetViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<VideoView>&, GetPlayer);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp2);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFree);
}
END_META;
// --- END META ---
