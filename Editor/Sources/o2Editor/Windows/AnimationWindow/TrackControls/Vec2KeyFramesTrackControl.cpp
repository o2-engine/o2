#include "o2Editor/stdafx.h"
#include "Vec2KeyFramesTrackControl.h"

#include "o2Editor/ToolsPanel.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    WeakRef<Vec2KeyFramesTrackControl> Vec2KeyFramesTrackControl::mLastActive;

    Vec2KeyFramesTrackControl::Vec2KeyFramesTrackControl(RefCounter* refCounter):
        Base(refCounter), mTool(mmake<SplineTool>())
    {
        InitializeControls();
    }

    Vec2KeyFramesTrackControl::Vec2KeyFramesTrackControl(RefCounter* refCounter, const Vec2KeyFramesTrackControl& other):
        Base(refCounter, other), mTool(mmake<SplineTool>())
    {
        InitializeControls();
    }

    Vec2KeyFramesTrackControl::~Vec2KeyFramesTrackControl()
    {
        o2EditorSceneScreen.RemoveTool(mTool);
    }

    void Vec2KeyFramesTrackControl::SetActive(bool active)
    {
        if (active)
        {
            if (mLastActive)
                mLastActive.Lock()->SetActive(false);

            o2EditorSceneScreen.AddTool(mTool);
            mPrevSelectedTool = o2EditorSceneScreen.GetSelectedTool();
            o2EditorSceneScreen.SelectTool<SplineTool>();

            mLastActive = Ref(this);
        }
        else
        {
            if (o2EditorSceneScreen.GetSelectedTool() == mTool)
                o2EditorSceneScreen.SelectTool(mPrevSelectedTool.Lock());

            o2EditorSceneScreen.RemoveTool(mTool);

            if (mLastActive == this)
                mLastActive = nullptr;
        }
    }

    Vec2KeyFramesTrackControl& Vec2KeyFramesTrackControl::operator=(const Vec2KeyFramesTrackControl& other)
    {
        Base::operator=(other);
        return *this;
    }

    void Vec2KeyFramesTrackControl::InitializeControls()
    {}

    void Vec2KeyFramesTrackControl::TryFindOwnerTrack()
    {
        mTrackOwner = nullptr;

        if (!mPlayer)
            return;

        auto path = mTrack.Lock()->path;
        auto root = Ref(dynamic_cast<Actor*>(mPlayer.Lock()->GetOwnerPlayer().Lock()->GetTarget()));
        while (root && path.StartsWith("child"))
        {
            int nextSlash = path.Find("/");
            if (nextSlash < 0)
                return;

            int nextChildSlash = path.Find("/", nextSlash + 1);
            auto childName = path.SubStr(nextSlash + 1, nextChildSlash);
            root = root->GetChild(childName);
            path.Erase(0, nextChildSlash + 1);
        }

        mTrackOwner = root;
    }

    void Vec2KeyFramesTrackControl::OnSetTrack()
    {
        TryFindOwnerTrack();

        mTool->SetSpline(mTrack.Lock()->spline, [&]() { 
            if (mTrackOwner && mTrackOwner.Lock()->GetParent())
                return mTrackOwner.Lock()->GetParent().Lock()->transform->worldPosition.Get(); 

            return Vec2F();
        });
    }

    void Vec2KeyFramesTrackControl::OnKeysChanged()
    {
        mTool->splineEditor->OnSplineChanged();
    }

    void Vec2KeyFramesTrackControl::SetCurveViewEnabled(bool enabled)
    {
        mAddKeyDotButton->enabled = enabled;
        mAddKeyButton->enabled = !enabled;
    }

    void Vec2KeyFramesTrackControl::Draw()
    {
        if (!mResEnabledInHierarchy)
            return;

        if (!mHandlesSheet.Lock()->enabled)
            return;

        OnDrawn();

        auto timeline = mTimeline.Lock();
        auto track = mTrack.Lock();

        o2Render.EnableScissorTest(timeline->layout->GetWorldRect());

        for (int i = 1; i < track->timeCurve->GetKeys().Count(); i++)
        {
            auto& key = track->timeCurve->GetKeys()[i];
            auto& prevKey = track->timeCurve->GetKeys()[i - 1];

            Basis drawCoords(RectF(timeline->LocalToWorld(prevKey.position) - 1,
                                   layout->GetWorldTop() - 5,
                                   timeline->LocalToWorld(key.position) - 1,
                                   layout->GetWorldBottom() + 5));

            DrawCurveInCoords(key.GetTopApproximatedPoints(), key.GetApproximatedPointsCount(),
                              key.GetGetApproximatedPointsBounds(), drawCoords, Color4(44, 62, 80));
        }

        for (auto& handle : mHandles)
            handle->handle->Draw();

        o2Render.DisableScissorTest();

        DrawDebugFrame();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::Vec2KeyFramesTrackControl>);
// --- META ---

DECLARE_CLASS(Editor::Vec2KeyFramesTrackControl, Editor__Vec2KeyFramesTrackControl);
// --- END META ---
