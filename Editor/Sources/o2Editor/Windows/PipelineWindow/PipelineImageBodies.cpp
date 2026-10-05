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
#include "o2Editor/Pipeline/PipelineUpscale.h"
#include "o2Editor/Pipeline/PipelineEditRegion.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
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
#include "o2/Render/VectorSprite.h"

namespace Editor
{
    using namespace PipelineControls;

    const Vector<String>& PipelineImageModelPresets()
    {
        static const Vector<String> presets = {
            "gemini-3.1-flash-image", "gemini-3-pro-image", "gemini-3-pro-image-preview", "gemini-2.5-flash-image",
            "gemini-2.5-flash-image-preview", "imagen-4.0-generate-001", "imagen-3.0-generate-001",
            "gpt-image-2.5-sunburst", "gpt-image-2.5-flare", "gpt-image-2", "gpt-image-1.5", "gpt-image-1", "gpt-image-1-mini",
            "google/gemini-3.1-flash-image", "google/gemini-3-pro-image", "google/gemini-2.5-flash-image",
            "openai/gpt-5.4-image-2", "openai/gpt-5-image", "openai/gpt-5-image-mini",
            "openai/gpt-image-2.5-sunburst", "openai/gpt-image-2.5-flare", "openai/gpt-image-1"
        };
        return presets;
    }

    // Summary of an image node's parameter list: the model and the seed first, then what follows them
    static Vector<String> ImageParamNames(const String& extra)
    {
        Vector<String> names = { "Model", "Seed" };
        if (!extra.IsEmpty())
            names.Add(extra);
        return names;
    }

    class NanoBananaBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddCropSection("no result - press play to compute the branch", true);
            AddPrimaryField("extraPrompt", "Extra prompt (optional style hints)");
            if (BeginParams(ImageParamNames(GetBool("transparentBg", false) ? "Transparency" : "")))
            {
                AddModelRow(PipelineImageModelPresets(), GeminiProvider::defaultImageModel, PipelineModelKind::Image);
                AddSeedRow();
                AddTransparencyBlock();
            }
            EndParams();
            OnOutputChanged();
        }
    };

    // Transparent by nature: only the method is a choice
    class AiRemoveBgBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddPairRow(MakeCropEditor("no result - press play to compute the branch"));
            AddPictureSwitches(false);
            AddPrimaryField("hint", "What to keep (optional), e.g. the character");
            bool native = PipelineTransparency::SupportsNativeTransparency(mNode->GetConfigString("model", GeminiProvider::defaultImageModel));
            if (BeginParams(ImageParamNames(native ? "" : "Transparent bg mode")))
            {
                AddModelRow(PipelineImageModelPresets(), GeminiProvider::defaultImageModel, PipelineModelKind::Image);
                AddSeedRow();
                AddTransparencyBlock(true);
            }
            EndParams();
            OnOutputChanged();
        }
    };

    class ImageEditBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            // The drawing stage is the input pane, the result sits beside it and the tool row spans the card under both
            WeakRef<ImageEditBody> weakThis(this);
            mBuiltWithRegion = HasEditRegion();
            mPaint = mmake<PipelinePaintEditor>();
            mPaint->name = "draw stage";
            mPaint->onConfigChanged = [weakThis](const String& key, bool completed)
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;

                self->Notify(key, completed);
                // Setting or clearing the frame changes the switches and the parameters
                if (key == "editRegion" && completed && self->HasEditRegion() != self->mBuiltWithRegion)
                    self->RebuildBodyKeepingArea();
            };
            mPaint->Init(mNode, true, "editRegion", true);
            mPaint->SetBeside(MakeCropEditor("no result - press play to compute the branch"));

            // The tools sit under the pictures with the switches at their right end; a frame keeps the background, so
            // there is no transparent switch then
            float switchesWidth = 0.0f;
            auto switches = MakePictureSwitches(!mBuiltWithRegion, switchesWidth);
            mPaint->SetToolbarBelow(switches, switchesWidth);
            AddFlexible(mPaint, [weakThis](float width)
            {
                auto self = weakThis.Lock();
                return self ? self->mPaint->GetBarsHeight(width) + PipelinePairLayout::PairRowHeight(self->mAnyImage) : 0.0f;
            });
            MarkContent(mPaint);
            AddPrimaryField("prompt", "Describe the change to apply to the image");
            String extra = mBuiltWithRegion ? "Frame" : GetBool("transparentBg", false) ? "Transparency" : "";
            if (BeginParams(ImageParamNames(extra)))
            {
                AddModelRow(PipelineImageModelPresets(), GeminiProvider::defaultImageModel, PipelineModelKind::Image);
                AddSeedRow();
                if (mBuiltWithRegion)
                {
                    auto hint = MakeLabel("Frame set: the background stays as it is", true);
                    hint->name = "edit region hint";
                    AddRow(hint, 18);
                }
                else
                    AddTransparencyBlock();
            }
            EndParams();

            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            if (!mPaint)
                return;

            auto input = GetInput("image");
            auto result = GetOutputBitmap();
            mPaint->SetBackground(input.IsImage() ? input.GetBitmap() : nullptr);
            mAnyImage = input.IsImage() || result;
            bool compare = PipelinePairLayout::ShowsCompare(PipelinePairLayout::GetIoView(), mNode->nodeType, input.IsImage(),
                                                             result != nullptr, GetBool("cropEnabled", false));
            mPaint->SetCompare(compare ? result : nullptr, mNode->id);
        }

        void OnConfigChanged() override
        {
            if (HasEditRegion() != mBuiltWithRegion)
                RebuildBody();
            else if (mPaint)
                mPaint->RefreshFromConfig();
        }

    private:
        Ref<PipelinePaintEditor> mPaint;                    // Drawing stage over the input, the result beside it
        bool                     mAnyImage = false;         // The input or the result has an image
        bool                     mBuiltWithRegion = false;  // The card was built with an edit region set

    private:
        bool HasEditRegion() const
        {
            PipelineImageOps::CropRect region;
            return PipelineEditRegion::Of(*mNode, region);
        }
    };

    class RemoveBgBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddPairRow(MakeResultView("no result - connect the white and black renders"));
            OnOutputChanged();
        }
    };

    class ImageOutlineBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddPairRow(MakeResultView("no result - press play to compute the branch"));
            AddColor("Color", "color", "#000000");
            AddSlider("Width", "width", 0, 64, 1, 4, "px");
            AddSegmented("position", { { "outside", "Outside" }, { "center", "Center" }, { "inside", "Inside" } }, "outside");
            AddSlider("Soft", "softness", 0, 32, 1, 0, "px");
            AddSlider("Alpha", "opacity", 0, 1, 0.05f, 1);
            OnOutputChanged();
        }
    };

    class ImageShadowBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddPairRow(MakeResultView("no result - press play to compute the branch"));
            AddColor("Color", "color", "#000000");
            AddSlider("Alpha", "opacity", 0, 1, 0.05f, 0.6f);
            AddSlider("Angle", "angle", 0, 359, 1, 45, "\xC2\xB0");
            AddSlider("Dist", "distance", 0, 128, 1, 8, "px");
            AddSlider("Blur", "blur", 0, 128, 1, 8, "px");
            AddSlider("Spread", "spread", 0, 64, 1, 0, "px");
            AddCheckbox("Inner shadow", "inner", false);
            OnOutputChanged();
        }
    };

    class ImageGradientBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddPairRow(MakeResultView("no result - press play to compute the branch"));
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
            AddPairRow(MakeResultView("no result - press play to compute the branch"));
            AddSlider("Bright", "brightness", -100, 100, 1, 0);
            AddSlider("Contr", "contrast", -100, 100, 1, 0);
            AddSlider("Sat", "saturation", -100, 100, 1, 0);
            AddSlider("Hue", "hue", -180, 180, 1, 0, "\xC2\xB0");
            AddColor("Tint", "tint", "#ff8800");
            AddSlider("Tint a", "tintStrength", 0, 1, 0.05f, 0);
            AddCheckbox("Colorize (tint hue, keep luminance)", "colorize", false);
            AddCheckbox("Grayscale", "grayscale", false);
            AddCheckbox("Invert", "invert", false);
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

    // Goes through GetString/SetString and the like, which route the keys to the own settings of a part
    void PipelineNodeBody::AddTransparencyBlock(bool always /*= false*/)
    {
        bool on = always || GetBool("transparentBg", false);
        WeakRef<PipelineNodeBody> weakThis(this);
        if (!on)
            return;

        // The model renders the alpha itself: the methods and their parameters have nothing to set
        if (PipelineTransparency::SupportsNativeTransparency(mNode->GetConfigString("model", GeminiProvider::defaultImageModel)))
        {
            auto note = MakeLabel("Rendered by the model", true);
            note->name = "native transparency";
            AddRow(note, 18);
            return;
        }

        // The label stands over the choice, as the segmented rows take the whole width
        auto modeLabel = MakeLabel("Transparent bg mode", false);
        modeLabel->name = "transparent mode label";
        AddRow(modeLabel, 18);
        AddSegmented("transparentMode", { { "twoPass", "White / black x2" }, { "chroma", "Chroma key x1" } }, "twoPass");

        if (GetString("transparentMode", "twoPass") == "chroma")
        {
            auto colorRow = mmake<HorizontalLayout>();
            colorRow->spacing = 4;
            colorRow->expandWidth = true;
            colorRow->expandHeight = true;
            colorRow->baseCorner = BaseCorner::Left;

            Color4 color;
            if (!PipelineUtils::ParseHexColor(GetString("chromaColor", "#00b140"), color))
                color = Color4(0, 177, 64, 255);
            auto field = mmake<PipelineColorField>();
            field->Setup("Key", color);
            field->onChanged = [weakThis](const Color4& value, bool completed)
            {
                if (auto self = weakThis.Lock()) self->SetString("chromaColor", PipelineUtils::ColorToHex(value), completed);
            };
            colorRow->AddChild(field);

            static const Vector<String> presets = { "#00b140", "#0047bb", "#ff00ff", "#ffffff" };
            for (auto& preset : presets)
            {
                Color4 pc;
                PipelineUtils::ParseHexColor(preset, pc);
                auto swatch = MakeButton("");
                swatch->layout->minWidth = 18;
                swatch->layout->maxWidth = 18;
                if (auto regular = swatch->GetLayerDrawableBasedOn<IRectDrawable>("regular")) regular->color = pc;
                String value = preset;
                Ref<PipelineColorField> fieldRef = field;
                swatch->onClick = [weakThis, value, fieldRef]()
                {
                    if (auto self = weakThis.Lock())
                    {
                        Color4 c; PipelineUtils::ParseHexColor(value, c);
                        fieldRef->SetColor(c);
                        self->SetString("chromaColor", value, true);
                    }
                };
                colorRow->AddChild(swatch);
            }
            AddRow(colorRow, 22);

            AddSlider("Tol", "chromaTolerance", 0, 100, 1, 30);
            AddSlider("Soft", "chromaSoftness", 0, 100, 1, 15);
            AddSlider("Spill", "chromaSpill", 0, 100, 1, 60);
        }
    }

    // AI upscale: the input beside the result, the size, the details to add and the model; a model that answers at its own
    // size gets a note that the rest is resampled
    class AiUpscaleBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddPairRow(MakeResultView("no result - press play to compute the branch"));
            AddSizeRow();
            AddPrimaryField("prompt", "Details to add (optional), e.g. crisp outlines, fabric weave");
            if (BeginParams(ImageParamNames("")))
            {
                AddModelRow(PipelineImageModelPresets(), GeminiProvider::defaultImageModel, PipelineModelKind::Image);
                if (!PipelineUpscale::RendersLargeSizes(GetString("model", GeminiProvider::defaultImageModel)))
                {
                    auto note = MakeLabel(PipelineUpscale::smallModelNote, true);
                    note->name = "small model note";
                    note->horOverflow = Label::HorOverflow::Wrap;
                    AddRow(note, 30);
                }
                AddSeedRow();
            }
            EndParams();
            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            RefreshSize();
        }

    private:
        Ref<EditBox> mWidthEdit;  // Target width in Size mode
        Ref<EditBox> mHeightEdit; // Target height in Size mode
        Ref<Label>   mInfo;       // "input → output" once the input size is known

    private:
        Vec2I InputSize() const
        {
            auto input = GetInput("image");
            auto bitmap = input.IsImage() ? input.GetBitmap() : nullptr;
            return bitmap ? bitmap->GetSize() : Vec2I();
        }

        bool Locked() const { return GetBool("lockAspect", true); }

        void AddSizeRow()
        {
            WeakRef<AiUpscaleBody> weakThis(this);
            auto row = mmake<HorizontalLayout>();
            row->name = "upscale size";
            row->spacing = 4;
            row->expandWidth = true;
            row->expandHeight = true;
            row->baseCorner = BaseCorner::Left;

            String mode = PipelineUpscale::ModeOf(*mNode);
            static const Vector<Pair<String, String>> modes = {
                { "x2", "\xC3\x97" "2" }, { "x3", "\xC3\x97" "3" }, { "x4", "\xC3\x97" "4" }, { "size", "Size" }
            };
            for (auto& m : modes)
            {
                auto segment = MakeSegment(m.second, m.first == mode);
                segment->name = "mode " + m.first;
                float width = m.first == "size" ? 44.0f : 36.0f;
                segment->layout->minWidth = width;
                segment->layout->maxWidth = width;
                String value = m.first;
                segment->onToggleByUser = [weakThis, value](bool)
                {
                    if (auto self = weakThis.Lock())
                    {
                        self->SetString("upscale", value, true);
                        self->RebuildBodyKeepingArea();
                    }
                };
                row->AddChild(segment);
            }

            if (mode == "size")
            {
                auto numberEdit = [&](const String& name, const Function<void(int)>& onChange)
                {
                    auto edit = MakeEditBox("", false);
                    edit->name = name;
                    edit->SetFilterInteger();
                    edit->layout->minWidth = 54;
                    edit->layout->maxWidth = 54;
                    edit->onChangeCompleted = [onChange](const WString& text)
                    {
                        int value = atoi(((String)text).Data());
                        if (value > 0)
                            onChange(Math::Min(value, PipelineUpscale::maxSide));
                    };
                    row->AddChild(edit);
                    return edit;
                };

                mWidthEdit = numberEdit("target width", [weakThis](int value)
                {
                    if (auto self = weakThis.Lock()) { self->SetNumber("targetW", (float)value, true); self->RefreshSize(); }
                });

                auto lock = MakeSegment("", Locked());
                lock->name = "lock aspect";
                lock->layout->minWidth = 22;
                lock->layout->maxWidth = 22;
                auto lockIcon = mmake<VectorSprite>("ui/pipeline/btn_link.svg");
                lockIcon->color = PipelineControls::textColor;
                lock->AddLayer("icon", lockIcon, Layout::Based(BaseCorner::Center, Vec2F(14, 14)));
                lock->onToggleByUser = [weakThis](bool)
                {
                    auto self = weakThis.Lock();
                    if (!self)
                        return;

                    // Unlocking keeps the height on view: it becomes the stored one
                    if (self->Locked())
                    {
                        Vec2I target = PipelineUpscale::Target(*self->mNode, self->InputSize());
                        self->mNode->SetConfigNumber("targetH", (float)target.y);
                    }
                    self->SetBool("lockAspect", !self->Locked(), true);
                    self->RebuildBodyKeepingArea();
                };
                row->AddChild(lock);

                mHeightEdit = numberEdit("target height", [weakThis](int value)
                {
                    auto self = weakThis.Lock();
                    if (!self)
                        return;

                    // Locked, the height follows the width: typing a height sets the width that gives it
                    Vec2I input = self->InputSize();
                    if (self->Locked() && input.x > 0 && input.y > 0)
                        self->SetNumber("targetW", (float)Math::Max(1, (int)std::lround((double)value*input.x/input.y)), true);
                    else
                        self->SetNumber("targetH", (float)value, true);
                    self->RefreshSize();
                });
            }

            mInfo = MakeLabel("", true);
            mInfo->name = "upscale info";
            mInfo->horAlign = HorAlign::Right;
            mInfo->horOverflow = Label::HorOverflow::Dots;
            row->AddChild(mInfo);
            AddRow(row, 22);
        }

        // Shows the target of the current input in the fields and the info
        void RefreshSize()
        {
            Vec2I input = InputSize();
            Vec2I target = PipelineUpscale::Target(*mNode, input.x > 0 ? input : Vec2I(512, 512));
            if (mWidthEdit)
                mWidthEdit->SetText((String)target.x);
            if (mHeightEdit)
                mHeightEdit->SetText((String)target.y);
            if (mInfo)
            {
                mInfo->text = input.x > 0 ? (String)input.x + "\xC3\x97" + (String)input.y + " -> " + (String)target.x + "\xC3\x97" +
                    (String)target.y : String();
            }
        }
    };

    Ref<PipelineNodeBody> CreateImageNodeBody(const String& type)
    {
        if (type == "aiUpscale")
            return mmake<AiUpscaleBody>();

        if (type == "nanoBananaGen")
            return mmake<NanoBananaBody>();

        if (type == "imageEdit")
            return mmake<ImageEditBody>();

        if (type == "imageExtract")
            return CreateExtractNodeBody(type);

        if (type == "removeBackground")
            return mmake<RemoveBgBody>();

        if (type == "aiRemoveBg")
            return mmake<AiRemoveBgBody>();

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
