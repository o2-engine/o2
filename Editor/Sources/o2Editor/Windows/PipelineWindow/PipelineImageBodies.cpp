#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
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
#include "o2Editor/Pipeline/PipelineExecutor.h"
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

    static const Vector<String> imageModelPresets = {
        "gemini-3.1-flash-image", "gemini-3-pro-image", "gemini-3-pro-image-preview", "gemini-2.5-flash-image",
        "gemini-2.5-flash-image-preview", "imagen-4.0-generate-001", "imagen-3.0-generate-001"
    };

    class NanoBananaBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddModelRow(imageModelPresets, GeminiProvider::defaultImageModel);
            AddSeedRow();
            AddTextArea("extraPrompt", "Extra prompt (optional style hints)", 44);
            AddTransparencyBlock();
            AddMutedLine("Prompt optional - add reference images on the inputs (+)");
            AddCropSection("run the node, then crop the result", 200);
            OnOutputChanged();
        }
    };

    class ImageEditBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddModelRow(imageModelPresets, GeminiProvider::defaultImageModel);
            AddSeedRow();
            AddTextArea("prompt", "Describe the change to apply to the image", 44);
            AddTransparencyBlock();

            bool drawOver = GetBool("drawOver", false);
            auto head = mmake<HorizontalLayout>();
            head->spacing = 4;
            head->expandWidth = true;
            head->expandHeight = true;
            head->baseCorner = BaseCorner::Left;
            auto expand = o2UI.CreateWidget<Button>("expand");
            expand->layout->minWidth = 18;
            expand->layout->maxWidth = 18;
            expand->SetStateForcible("expanded", drawOver);
            WeakRef<PipelineNodeBody> weakThis(this);
            expand->onClick = [weakThis]()
            {
                if (auto self = weakThis.Lock())
                {
                    self->SetBool("drawOver", !self->GetBool("drawOver", false), true);
                    self->RebuildBody();
                }
            };
            head->AddChild(expand);
            head->AddChild(MakeLabel("Draw over the image", false));
            AddRow(head, 20);

            if (drawOver)
            {
                mPaint = mmake<PipelinePaintEditor>();
                mPaint->onConfigChanged = [weakThis](const String& key, bool completed) { if (auto self = weakThis.Lock()) self->Notify(key, completed); };
                mPaint->Init(mNode, false);
                { auto paint = mPaint; AddFlexible(mPaint, [paint](float width) { return Math::Max(220.0f, paint->GetMinHeightForWidth(width) + 70.0f); }); MarkContent(mPaint); }
            }

            AddCropSection("run the node, then crop the result", 200);
            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            if (mPaint)
            {
                auto input = GetInput("image");
                mPaint->SetBackground(input.IsImage() ? input.GetBitmap() : nullptr);
            }
        }

        void OnConfigChanged() override
        {
            if (mPaint)
                mPaint->RefreshFromConfig();
        }

    private:
        Ref<PipelinePaintEditor> mPaint;
    };

    class ImageExtractBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddModelRow(imageModelPresets, GeminiProvider::defaultImageModel);
            AddSeedRow();
            AddTextArea("prompt", "What to keep, e.g. 'the green chip'", 44);
            AddTransparencyBlock();

            AddMutedLine("Region - drag the box around the part; only the box is sent to the AI. Brush strokes guide the extraction");
            mPaint = mmake<PipelinePaintEditor>();
            WeakRef<PipelineNodeBody> weakThis(this);
            mPaint->onConfigChanged = [weakThis](const String& key, bool completed) { if (auto self = weakThis.Lock()) self->Notify(key, completed); };
            mPaint->Init(mNode, true);
            { auto paint = mPaint; AddFlexible(mPaint, [paint](float width) { return Math::Max(220.0f, paint->GetMinHeightForWidth(width) + 70.0f); }); MarkContent(mPaint); }

            AddCropSection(GetBool("transparentBg", false) ? "run the node, then crop the transparent sprite" : "run the node, then crop the sprite", 160);
            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            if (mPaint)
            {
                auto input = GetInput("image");
                mPaint->SetBackground(input.IsImage() ? input.GetBitmap() : nullptr);
            }
        }

        void OnConfigChanged() override
        {
            if (mPaint)
                mPaint->RefreshFromConfig();
        }

    private:
        Ref<PipelinePaintEditor> mPaint;
    };

    class RemoveBgBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultImage("connect white and black inputs", 150);
            OnOutputChanged();
        }
    };

    class ImageOutlineBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddColor("Color", "color", "#000000");
            AddSlider("Width", "width", 0, 64, 1, 4, "px");
            AddSegmented("position", { { "outside", "Outside" }, { "center", "Center" }, { "inside", "Inside" } }, "outside");
            AddSlider("Soft", "softness", 0, 32, 1, 0, "px");
            AddSlider("Alpha", "opacity", 0, 1, 0.05f, 1);
            AddResultImage("connect an image", 140);
            OnOutputChanged();
        }
    };

    class ImageShadowBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddColor("Color", "color", "#000000");
            AddSlider("Alpha", "opacity", 0, 1, 0.05f, 0.6f);
            AddSlider("Angle", "angle", 0, 359, 1, 45, "\xC2\xB0");
            AddSlider("Dist", "distance", 0, 128, 1, 8, "px");
            AddSlider("Blur", "blur", 0, 128, 1, 8, "px");
            AddSlider("Spread", "spread", 0, 64, 1, 0, "px");
            AddCheckbox("Inner shadow", "inner", false);
            AddResultImage("connect an image", 140);
            OnOutputChanged();
        }
    };

    class ImageGradientBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddSegmented("kind", { { "linear", "Linear" }, { "radial", "Radial" } }, "linear");
            AddSelectRow("Blend", "blend", { "normal", "multiply", "screen", "overlay", "map" }, "normal");
            AddColor("A", "color1", "#ffffff");
            AddSlider("A alpha", "alpha1", 0, 1, 0.05f, 1);
            AddColor("B", "color2", "#000000");
            AddSlider("B alpha", "alpha2", 0, 1, 0.05f, 1);
            if (GetString("blend", "normal") != "map" && GetString("kind", "linear") != "radial")
                AddSlider("Angle", "angle", 0, 359, 1, 90, "\xC2\xB0");
            AddSlider("Amount", "opacity", 0, 1, 0.05f, 1);
            AddCheckbox("Keep inside the artwork", "clipToAlpha", true);
            if (GetString("blend", "normal") == "map")
                AddMutedLine("Gradient map: A -> dark tones, B -> light tones");
            AddResultImage("connect an image", 140);
            OnOutputChanged();
        }

        void OnConfigChanged() override { RebuildBody(); }
    };

    class ImageColorBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddSlider("Bright", "brightness", -100, 100, 1, 0);
            AddSlider("Contr", "contrast", -100, 100, 1, 0);
            AddSlider("Sat", "saturation", -100, 100, 1, 0);
            AddSlider("Hue", "hue", -180, 180, 1, 0, "\xC2\xB0");
            AddColor("Tint", "tint", "#ff8800");
            AddSlider("Tint a", "tintStrength", 0, 1, 0.05f, 0);
            AddCheckbox("Colorize (tint hue, keep luminance)", "colorize", false);
            AddCheckbox("Grayscale", "grayscale", false);
            AddCheckbox("Invert", "invert", false);
            AddResultImage("connect an image", 140);
            OnOutputChanged();
        }
    };

    class DrawImageBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            mPaint = mmake<PipelinePaintEditor>();
            WeakRef<PipelineNodeBody> weakThis(this);
            mPaint->onConfigChanged = [weakThis](const String& key, bool completed) { if (auto self = weakThis.Lock()) self->Notify(key, completed); };
            mPaint->Init(mNode, false);
            { auto paint = mPaint; AddFlexible(mPaint, [paint](float width) { return Math::Max(240.0f, paint->GetMinHeightForWidth(width) + 90.0f); }); MarkContent(mPaint); }
            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            if (mPaint)
            {
                auto input = GetInput("background");
                mPaint->SetBackground(input.IsImage() ? input.GetBitmap() : nullptr);
            }
        }

        void OnConfigChanged() override
        {
            if (mPaint)
                mPaint->RefreshFromConfig();
        }

    private:
        Ref<PipelinePaintEditor> mPaint;
    };

    Ref<PipelineNodeBody> CreateImageNodeBody(const String& type)
    {
        if (type == "nanoBananaGen")
            return mmake<NanoBananaBody>();

        if (type == "imageEdit")
            return mmake<ImageEditBody>();

        if (type == "imageExtract")
            return mmake<ImageExtractBody>();

        if (type == "removeBackground")
            return mmake<RemoveBgBody>();

        if (type == "imageOutline")
            return mmake<ImageOutlineBody>();

        if (type == "imageShadow")
            return mmake<ImageShadowBody>();

        if (type == "imageGradient")
            return mmake<ImageGradientBody>();

        if (type == "imageColor")
            return mmake<ImageColorBody>();

        if (type == "drawImage")
            return mmake<DrawImageBody>();

        return nullptr;
    }
}
