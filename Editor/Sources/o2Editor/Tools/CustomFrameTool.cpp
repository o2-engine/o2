#include "o2Editor/stdafx.h"
#include "CustomFrameTool.h"

#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    CustomFrameTool::CustomFrameTool(RefCounter* refCounter):
        IEditTool(refCounter)
    {
        sceneLayer->tool = Ref(this);

        frameHandles = mmake<FrameHandles>();
        frameHandles->isInputTransparent = true;
        frameHandles->onTransformed = [&](const Basis& x) {
            auto b = x*o2EditorSceneScreen.GetScreenToLocalTransform();
            b.origin -= getOrigin();
            mBasis = b;
            onChanged(mBasis);
        };
    }

    void CustomFrameTool::SceneLayer::DrawOverScene()
    {
        auto toolRef = tool.Lock();

        Basis b = toolRef->mBasis;
        b.origin += toolRef->getOrigin();

        toolRef->frameHandles->SetBasis(b*o2EditorSceneScreen.GetLocalToScreenTransform());
        toolRef->frameHandles->Draw();
    }

    void CustomFrameTool::SceneLayer::Update(float dt)
    {}

    int CustomFrameTool::SceneLayer::GetOrder() const
    {
        return 0;
    }

    bool CustomFrameTool::SceneLayer::IsEnabled() const
    {
        return tool.Lock()->isEnabled;
    }

    const String& CustomFrameTool::SceneLayer::GetName() const
    {
        static String res("Custom frame");
        return res;
    }

    const String& CustomFrameTool::SceneLayer::GetIconName() const
    {
        return String::empty;
    }

    void CustomFrameTool::SetFrame(const Basis& frame)
    {
        mBasis = frame;
    }

    const Basis& CustomFrameTool::GetFrame() const
    {
        return mBasis;
    }

    void CustomFrameTool::Reset()
    {
        onChanged.Clear();
        getOrigin.Clear();
    }

    String CustomFrameTool::CustomFrameTool::GetPanelIcon() const
    {
        return "ui/UI4_image_frame_tool.png";
    }

    void CustomFrameTool::CustomFrameTool::OnEnabled()
    {
        o2EditorSceneScreen.AddEditorLayer(sceneLayer);
        isEnabled = true;
    }

    void CustomFrameTool::CustomFrameTool::OnDisabled()
    {
        o2EditorSceneScreen.RemoveEditorLayer(sceneLayer);
        isEnabled = false;
    }
}
// --- META ---

DECLARE_CLASS(Editor::CustomFrameTool, Editor__CustomFrameTool);
// --- END META ---
