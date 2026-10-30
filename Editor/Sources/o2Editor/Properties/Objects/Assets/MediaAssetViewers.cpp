#include "o2Editor/stdafx.h"
#include "MediaAssetViewers.h"

#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Assets/Types/VideoAsset.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/Preview/MediaPlayerView.h"
#include "o2Editor/UI/Preview/MediaPreview.h"
#include "o2Editor/Windows/AssetPreviewWindow/AssetPreviewWindow.h"

namespace Editor
{

    const Type* SoundAssetViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* SoundAssetViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(SoundAsset);
    }

    const Ref<SoundPlayerView>& SoundAssetViewer::GetPlayer() const
    {
        return mPlayer;
    }

    void SoundAssetViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        PushEditorScopeOnStack scope;

        mInfoLabel = o2UI.CreateLabel("");
        mInfoLabel->name = "sound info";
        mInfoLabel->layout->minHeight = 20;
        mInfoLabel->horAlign = HorAlign::Left;
        mSpoiler->AddChild(mInfoLabel);

        mPlayer = mmake<SoundPlayerView>();
        mPlayer->name = "sound player";
        mPlayer->layout->minHeight = 28;
        mSpoiler->AddChild(mPlayer);

        mSpoiler->AddChild(AssetPreviewWindow::CreateOpenButton([this]() {
            auto asset = mAsset.Lock();
            return asset ? AssetRef<Asset>(asset) : AssetRef<Asset>();
        }));
    }

    void SoundAssetViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        if (targetObjets.IsEmpty() || !mPlayer)
            return;

        auto sound = dynamic_cast<SoundAsset*>(targetObjets.Last().first);
        if (!sound)
            return;

        // The properties refresh their viewers all the time: the playback goes on until the sound changes
        if (mAsset.Lock().Get() != sound)
            mPlayer->SetSound(AssetRef<SoundAsset>(Ref<SoundAsset>(sound)));

        mAsset = WeakRef<Asset>(sound);
        mInfoLabel->SetText(MediaPreview::GetSoundInfo(*sound));
    }

    void SoundAssetViewer::OnFree()
    {
        if (mPlayer)
            mPlayer->Stop();
    }

    const Type* VideoAssetViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* VideoAssetViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(VideoAsset);
    }

    const Ref<VideoView>& VideoAssetViewer::GetPlayer() const
    {
        return mPlayer;
    }

    void VideoAssetViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        PushEditorScopeOnStack scope;

        mInfoLabel = o2UI.CreateLabel("");
        mInfoLabel->name = "video info";
        mInfoLabel->layout->minHeight = 20;
        mInfoLabel->horAlign = HorAlign::Left;
        mSpoiler->AddChild(mInfoLabel);

        mPlayer = mmake<VideoView>();
        mPlayer->name = "video player";
        mPlayer->layout->minHeight = 200;
        mSpoiler->AddChild(mPlayer);

        mSpoiler->AddChild(AssetPreviewWindow::CreateOpenButton([this]() {
            auto asset = mAsset.Lock();
            return asset ? AssetRef<Asset>(asset) : AssetRef<Asset>();
        }));
    }

    void VideoAssetViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        if (targetObjets.IsEmpty() || !mPlayer)
            return;

        auto video = dynamic_cast<VideoAsset*>(targetObjets.Last().first);
        if (!video)
            return;

        if (mAsset.Lock().Get() != video)
            mPlayer->SetVideo(AssetRef<VideoAsset>(Ref<VideoAsset>(video)));

        mAsset = WeakRef<Asset>(video);

        mInfoLabel->SetText(MediaPreview::GetVideoInfo(*video));
    }

    void VideoAssetViewer::OnFree()
    {
        if (mPlayer)
            mPlayer->Pause();
    }
}
// --- META ---

DECLARE_CLASS(Editor::SoundAssetViewer, Editor__SoundAssetViewer);

DECLARE_CLASS(Editor::VideoAssetViewer, Editor__VideoAssetViewer);
// --- END META ---
