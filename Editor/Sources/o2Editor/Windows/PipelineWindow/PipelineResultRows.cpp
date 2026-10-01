#include "o2Editor/stdafx.h"
#include "PipelineNodeBody.h"

#include "o2/Assets/Assets.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Assets/Types/FolderAsset.h"

#include "o2Editor/Windows/PipelineWindow/PipelineNodeBodyFactories.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Bitmap/PngFormat.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/FrameHandles.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/System/Clipboard.h"
#include "o2Editor/Dialogs/ColorPickerDlg.h"
#include "o2Editor/Dialogs/System/OpenSaveDialog.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineModelPicker.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/ElevenLabsProvider.h"
#include "o2Editor/Pipeline/Providers/GeminiProvider.h"
#include "o2Editor/Pipeline/Providers/VideoProviders.h"
#include "o2Editor/Windows/PipelineWindow/PipelineComposerStage.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineMediaViews.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePaintEditor.h"

namespace Editor
{
    using namespace PipelineControls;

    Ref<PipelineIoPair> PipelineNodeBody::AddPairRow(const Ref<Widget>& result)
    {
        mPair = mmake<PipelineIoPair>();
        mPair->name = "pair";
        mPair->Setup(mNode->id, mNode->nodeType, result);
        auto pair = mPair;
        AddFlexible(mPair, [pair](float) { return pair->GetMinHeight(); });
        MarkContent(mPair);
        return mPair;
    }

    Ref<PipelineImageView> PipelineNodeBody::MakeResultView(const String& hint)
    {
        mImageView = mmake<PipelineImageView>();
        mImageView->SetHint(hint);
        return mImageView;
    }

    Ref<PipelineCropEditor> PipelineNodeBody::MakeCropEditor(const String& emptyHint)
    {
        WeakRef<PipelineNodeBody> weakThis(this);
        mCropEditor = mmake<PipelineCropEditor>();
        mCropEditor->SetHint(emptyHint);
        mCropEditor->SetNode(mNode, "crop");
        mCropEditor->SetCropEnabled(GetBool("cropEnabled", false));
        mCropEditor->onCropChanged = [weakThis](bool completed)
        {
            if (auto self = weakThis.Lock())
                self->Notify("crop", completed);
        };
        return mCropEditor;
    }

    void PipelineNodeBody::AddCropSection(const String& emptyHint, bool transparentSwitch, const String& caption /*= ""*/,
                                          float minHeight /*= 176.0f*/)
    {
        AddFlexible(MakeCropEditor(emptyHint), minHeight);
        MarkContent(mCropEditor);
        AddPictureSwitches(transparentSwitch, caption);
    }

    Ref<Toggle> PipelineNodeBody::MakeTransparentSwitch()
    {
        auto toggle = MakeCheckbox("Transparent bg", GetBool("transparentBg", false));
        toggle->name = "transparent switch";
        toggle->layout->minWidth = 140;
        toggle->layout->maxWidth = 140;
        WeakRef<PipelineNodeBody> weakThis(this);
        toggle->onToggleByUser = [weakThis](bool value)
        {
            if (auto self = weakThis.Lock())
            {
                self->SetBool("transparentBg", value, true);
                self->RebuildBodyKeepingArea();
            }
        };
        return toggle;
    }

    Ref<Widget> PipelineNodeBody::MakePictureSwitches(bool transparentSwitch, float& width)
    {
        WeakRef<PipelineNodeBody> weakThis(this);
        auto switches = mmake<HorizontalLayout>();
        switches->name = "picture switches";
        switches->spacing = 6;
        switches->expandWidth = true;
        switches->expandHeight = true;
        switches->baseCorner = BaseCorner::Left;
        width = 0.0f;

        if (transparentSwitch)
        {
            auto toggle = MakeTransparentSwitch();
            width += toggle->layout->minWidth + switches->spacing;
            switches->AddChild(toggle);
        }

        auto cropButton = MakeSegment("Crop", GetBool("cropEnabled", false));
        cropButton->name = "crop";
        cropButton->layout->minWidth = 60;
        cropButton->layout->maxWidth = 60;
        cropButton->onToggleByUser = [weakThis](bool)
        {
            if (auto self = weakThis.Lock())
            {
                bool on = !self->GetBool("cropEnabled", false);
                self->SetBool("cropEnabled", on, true);
                if (on && !self->mNode->HasConfig("crop"))
                {
                    auto& crop = self->mNode->config["crop"];
                    crop.SetObject();
                    crop["x"] = 0.1f; crop["y"] = 0.1f; crop["w"] = 0.8f; crop["h"] = 0.8f;
                }
                self->RebuildBody();
            }
        };
        switches->AddChild(cropButton);
        width += 60.0f;
        return switches;
    }

    void PipelineNodeBody::AddPictureSwitches(bool transparentSwitch, const String& caption /*= ""*/)
    {
        auto row = mmake<HorizontalLayout>();
        row->spacing = 6;
        row->expandWidth = true;
        row->expandHeight = true;
        row->baseCorner = BaseCorner::Left;

        // The caption, or nothing, takes the free width so the switches keep to the right
        auto label = MakeLabel(caption, true);
        label->horOverflow = caption.Length() > 26 ? Label::HorOverflow::Wrap : Label::HorOverflow::Dots;
        row->AddChild(label);

        float width = 0.0f;
        auto switches = MakePictureSwitches(transparentSwitch, width);
        switches->layout->minWidth = width;
        switches->layout->maxWidth = width;
        row->AddChild(switches);
        AddRow(row, caption.Length() > 26 ? 30.0f : 20.0f);
    }

    PipelineCropEditor::PipelineCropEditor(RefCounter* refCounter):
        PipelineImageView(refCounter)
    {
        mFrame = mmake<FrameHandles>();
        mFrame->SetPivotEnabled(false);
        mFrame->SetRotationEnabled(false);
        mFrame->onTransformed = THIS_FUNC(OnFrameTransformed);
        mFrame->onChangeCompleted = THIS_FUNC(OnFrameCompleted);
        mFrame->onPressed = [this]() { mFrameDragging = true; };
        mFrame->onReleased = [this]() { mFrameDragging = false; };
    }

    void PipelineCropEditor::SetNode(const Ref<PipelineNode>& node, const String& key /*= "crop"*/)
    {
        mNode = node;
        mKey = key;
    }

    void PipelineCropEditor::SetCropEnabled(bool enabled)
    {
        mCropEnabled = enabled;
    }

    void PipelineCropEditor::SyncFrameFromConfig()
    {
        if (!mNode)
            return;

        PipelineImageOps::CropRect crop;
        PipelineImageOps::ParseCrop(mNode->GetConfigValue(mKey.Data()), crop);
        RectF image = GetImageRect();
        float left = image.left + image.Width() * crop.x;
        float top = image.top - image.Height() * crop.y;
        float w = image.Width() * crop.w;
        float h = image.Height() * crop.h;
        mSyncing = true;
        mFrame->SetBasis(Basis(Vec2F(left, top - h), Vec2F(w, 0), Vec2F(0, h)));
        mSyncing = false;
    }

    void PipelineCropEditor::Draw()
    {
        PipelineImageView::Draw();
        if (!mCropEnabled || !HasImage() || PipelineControls::IsFarView())
            return;

        if (!mFrameDragging)
            SyncFrameFromConfig();

        RectF image = GetImageRect();
        const Basis& b = mFrame->GetCurrentBasis();
        RectF frame(b.origin, b.origin + b.xv + b.yv);
        // Darken what is cut away
        Color4 shade(0, 0, 0, 120);
        o2Render.DrawFilledPolygon({ image.LeftBottom(), Vec2F(image.left, image.top), Vec2F(frame.left, image.top), Vec2F(frame.left, image.bottom) }, shade);
        o2Render.DrawFilledPolygon({ Vec2F(frame.right, image.bottom), Vec2F(frame.right, image.top), image.RightTop(), Vec2F(image.right, image.bottom) }, shade);
        o2Render.DrawFilledPolygon({ Vec2F(frame.left, frame.top), Vec2F(frame.left, image.top), Vec2F(frame.right, image.top), Vec2F(frame.right, frame.top) }, shade);
        o2Render.DrawFilledPolygon({ Vec2F(frame.left, image.bottom), Vec2F(frame.left, frame.bottom), Vec2F(frame.right, frame.bottom), Vec2F(frame.right, image.bottom) }, shade);
        mFrame->Draw();
    }

    void PipelineCropEditor::OnFrameTransformed(const Basis& basis)
    {
        if (mSyncing || !mNode)
            return;

        RectF image = GetImageRect();
        if (image.Width() <= 0 || image.Height() <= 0)
            return;

        RectF frame(basis.origin, basis.origin + basis.xv + basis.yv);
        float x = Math::Clamp((frame.left - image.left) / image.Width(), 0.0f, 1.0f);
        float y = Math::Clamp((image.top - frame.top) / image.Height(), 0.0f, 1.0f);
        float w = Math::Clamp(frame.Width() / image.Width(), 0.01f, 1.0f - x);
        float h = Math::Clamp(frame.Height() / image.Height(), 0.01f, 1.0f - y);

        auto& crop = mNode->config[mKey.Data()];
        crop.SetObject();
        crop["x"] = x; crop["y"] = y; crop["w"] = w; crop["h"] = h;

        if (onCropChanged)
            onCropChanged(false);
    }

    void PipelineCropEditor::OnFrameCompleted()
    {
        if (onCropChanged)
            onCropChanged(true);
    }

    namespace PipelineNodeBodies
    {
        Ref<PipelineNodeBody> Create(const String& type, const Ref<PipelineNodeWidget>& owner)
        {
            Ref<PipelineNodeBody> body = CreateBasicNodeBody(type);
            if (!body)
                body = CreateImageNodeBody(type);

            if (!body)
                body = CreateComposerNodeBody(type);

            if (body)
                body->Init(owner);

            return body;
        }
    }
}
