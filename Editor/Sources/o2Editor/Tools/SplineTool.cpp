#include "o2Editor/stdafx.h"
#include "SplineTool.h"

#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    SplineTool::SplineTool(RefCounter* refCounter) :
        IEditTool(refCounter), splineEditor(mmake<SplineEditor>()), sceneLayer(mmake<SplineSceneLayer>())
    {
        sceneLayer->tool = Ref(this);
    }

    void SplineTool::Reset()
    {
        splineEditor->Reset();
        onChanged.Clear();
    }

    Vec2F SplineTool::SplineWrapper::GetOrigin() const
    {
        return getOrigin();
    }

    Vec2F SplineTool::SplineWrapper::WorldToLocal(const Vec2F& point) const
    {
        return o2EditorSceneScreen.ScreenToLocalPoint(point) - GetOrigin();
    }

    Vec2F SplineTool::SplineWrapper::LocalToWorld(const Vec2F& point) const
    {
        return o2EditorSceneScreen.LocalToScreenPoint(point + GetOrigin());
    }

    int SplineTool::SplineWrapper::GetPointsCount() const
    {
        return spline->GetKeys().Count();
    }

    bool SplineTool::SplineWrapper::IsClosed() const
    {
        return spline->IsClosed();
    }

    void SplineTool::SplineWrapper::AddPoint(int idx, const Vec2F& position,
                                             const Vec2F& prevSupport, const Vec2F& nextSupport)
    {
        spline->InsertKey(idx, position, 0.0f, prevSupport, nextSupport);
    }

    void SplineTool::SplineWrapper::RemovePoint(int idx)
    {
        spline->RemoveKey(idx);
    }

    Vec2F SplineTool::SplineWrapper::GetPointPos(int idx) const
    {
        return spline->GetKey(idx).value;
    }

    void SplineTool::SplineWrapper::SetPointPos(int idx, const Vec2F& pos)
    {
        auto key = spline->GetKey(idx);
        key.value = pos;
        spline->SetKey(key, idx);
    }

    void SplineTool::SplineWrapper::SetPointRangeValue(int idx, float value)
    {
        auto key = spline->GetKey(idx);
        key.valueRange = value;
        spline->SetKey(key, idx);
    }

    float SplineTool::SplineWrapper::GetPointRangeValue(int idx) const
    {
        return spline->GetKey(idx).valueRange;
    }

    Vec2F SplineTool::SplineWrapper::GetPointPrevSupportPos(int idx) const
    {
        auto key = spline->GetKey(idx);
        return key.prevSupport + key.value;
    }

    void SplineTool::SplineWrapper::SetPointPrevSupportPos(int idx, const Vec2F& pos)
    {
        auto key = spline->GetKey(idx);
        key.prevSupport = pos - key.value;
        spline->SetKey(key, idx);
    }

    Vec2F SplineTool::SplineWrapper::GetPointNextSupportPos(int idx) const
    {
        auto key = spline->GetKey(idx);
        return key.nextSupport + key.value;
    }

    void SplineTool::SplineWrapper::SetPointNextSupportPos(int idx, const Vec2F& pos)
    {
        auto key = spline->GetKey(idx);
        key.nextSupport = pos - key.value;
        spline->SetKey(key, idx);
    }

    const ApproximationVec2F* SplineTool::SplineWrapper::GetPointApproximationLeft(int idx) const
    {
        return spline->GetKeys()[idx].GetApproximatedPointsLeft();
    }

    const ApproximationVec2F* SplineTool::SplineWrapper::GetPointApproximationRight(int idx) const
    {
        return spline->GetKeys()[idx].GetApproximatedPointsRight();
    }

    int SplineTool::SplineWrapper::GetPointApproximationCount(int idx) const
    {
        return spline->GetKeys()[idx].GetApproximatedPointsCount();
    }

    void SplineTool::SplineWrapper::OnChanged()
    {
        tool.Lock()->onChanged();
    }

    void SplineTool::SplineSceneLayer::DrawOverScene()
    {
        tool.Lock()->splineEditor->Draw();
    }

    void SplineTool::SplineSceneLayer::Update(float dt)
    {
        tool.Lock()->splineEditor->Update(dt);
    }

    int SplineTool::SplineSceneLayer::GetOrder() const
    {
        return 0;
    }

    bool SplineTool::SplineSceneLayer::IsEnabled() const
    {
        return tool.Lock()->isEnabled;
    }

    const String& SplineTool::SplineSceneLayer::GetName() const
    {
        static String res("Animation spline");
        return res;
    }

    const String& SplineTool::SplineSceneLayer::GetIconName() const
    {
        return String::empty;
    }

    void SplineTool::SplineTool::SetSpline(const Ref<Spline>& spline, const Function<Vec2F()>& getOrigin)
    {
        auto wrapper = mmake<SplineWrapper>();
        wrapper->spline = spline;
        wrapper->getOrigin = getOrigin;
        wrapper->tool = Ref(this);
        spline->onKeysChanged += [=]() { wrapper->onChangedOutside(); };
        splineEditor->SetSpline(wrapper);
    }

    String SplineTool::SplineTool::GetPanelIcon() const
    {
        return "ui/UI4_path_tool.png";
    }

    void SplineTool::SplineTool::OnEnabled()
    {
        o2EditorSceneScreen.AddEditorLayer(sceneLayer);
        isEnabled = true;
    }

    void SplineTool::SplineTool::OnDisabled()
    {
        o2EditorSceneScreen.RemoveEditorLayer(sceneLayer);
        isEnabled = false;
    }
}
// --- META ---

DECLARE_CLASS(Editor::SplineTool, Editor__SplineTool);
// --- END META ---
