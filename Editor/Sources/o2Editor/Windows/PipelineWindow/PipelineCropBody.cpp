#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2/Render/Text.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

namespace Editor
{
    using namespace PipelineControls;

    // ---------------------------------------------------------------------------------------------------------------
    // Image crop (AssetsLine ImageCropBody): the input with its frame, always on - drag its corners and edges; the frame in
    // the picture's pixels (X, Y, W, H, typed) with the whole-picture button; the new size with the proportions lock
    // ---------------------------------------------------------------------------------------------------------------
    class ImageCropBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;

        void Build() override
        {
            auto& theme = PipelineTheme::Get();
            WeakRef<ImageCropBody> weakThis(this);

            // The picture with its frame: taller than a result card, the frame is the whole point of the node
            AddFlexible(MakeCropEditor("nothing connected - link a picture to crop it"), 240.0f);
            MarkContent(mCropEditor);
            mCropEditor->SetCropEnabled(true);
            mCropEditor->onCropChanged = [weakThis](bool completed)
            {
                if (auto self = weakThis.Lock())
                {
                    self->Notify("crop", completed);
                    self->UpdateFields();
                }
            };

            auto pixels = mmake<HorizontalLayout>();
            pixels->name = "crop pixels";
            pixels->spacing = 4;
            pixels->expandWidth = true;
            pixels->expandHeight = true;
            pixels->baseCorner = BaseCorner::Left;
            const char* letters[] = { "X", "Y", "W", "H" };
            for (int i = 0; i < 4; i++)
            {
                mPixel[i] = MakeNumberField(String("crop ") + String(letters[i]).ToLowerCase());
                mPixel[i]->onChangeCompleted = [weakThis, i](const WString& text)
                {
                    if (auto self = weakThis.Lock())
                        self->OnPixelEdited(i, ((String)text).Trimed());
                };
                pixels->AddChild(MakeLettered(letters[i], mPixel[i]));
            }

            mWhole = MakeIconButton("ui/pipeline/btn_maximize.svg", theme.fgDim, Color4(0, 0, 0, 0));
            mWhole->name = "whole picture";
            AddBorderLayer(*mWhole, "border", theme.line, fieldRadius, 1.0f, Layout::BothStretch(), -1.0f);
            mWhole->layout->minWidth = mWhole->layout->maxWidth = 26;
            mWhole->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->SetWhole(); };
            pixels->AddChild(mWhole);
            AddRow(pixels, 22);

            // .resize-row: the Resize switch, W x H in mono fields with the aspect lock between
            auto resizeRow = mmake<HorizontalLayout>();
            resizeRow->name = "resize row";
            resizeRow->spacing = 6;
            resizeRow->expandWidth = true;
            resizeRow->expandHeight = true;
            resizeRow->baseCorner = BaseCorner::Left;
            mResize = MakeCheckbox("New size", GetBool("resize", false));
            mResize->name = "resize";
            float toggleWidth = switchWidth + rowGap + Math::Ceil(TextWidth("New size", bodyFont));
            mResize->layout->minWidth = mResize->layout->maxWidth = toggleWidth;
            mResize->onToggleByUser = [weakThis](bool value) { if (auto self = weakThis.Lock()) self->OnResizeToggled(value); };
            resizeRow->AddChild(mResize);

            mResizeW = MakeNumberField("resize width");
            mResizeW->onChangeCompleted = [weakThis](const WString& t) { if (auto self = weakThis.Lock()) self->OnResizeEdited(true, atoi(((String)t).Data())); };
            resizeRow->AddChild(mResizeW);

            mLock = MakeIconButton("ui/pipeline/btn_link.svg", theme.accent, Color4(0, 0, 0, 0));
            mLock->name = "lock aspect";
            AddBorderLayer(*mLock, "border", PipelineTheme::Alpha(theme.accent, 0.5f), 6.0f, 1.0f, Layout::BothStretch(), -1.0f);
            mLock->layout->minWidth = mLock->layout->maxWidth = 22;
            mLock->onClick = [weakThis]()
            {
                if (auto self = weakThis.Lock())
                {
                    self->SetBool("lockAspect", !self->GetBool("lockAspect", true), true);
                    self->UpdateFields();
                }
            };
            resizeRow->AddChild(mLock);

            mResizeH = MakeNumberField("resize height");
            mResizeH->onChangeCompleted = [weakThis](const WString& t) { if (auto self = weakThis.Lock()) self->OnResizeEdited(false, atoi(((String)t).Data())); };
            resizeRow->AddChild(mResizeH);
            resizeRow->AddChild(mmake<Widget>());
            AddRow(resizeRow, smallButtonHeight);

            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();

            // The frame is drawn on the input; without one the last result is shown
            auto input = GetInput("image");
            if (mCropEditor)
            {
                Ref<Bitmap> picture = input.IsImage() ? Picture(input) : Picture(GetOutput());
                mCropEditor->SetBitmap(picture);
                mCropEditor->SetLoading(!picture && PicturePending(input.IsImage() ? input : GetOutput()));
                mCropEditor->SetCropEnabled(input.IsImage());
            }

            UpdateFields();
        }

        void OnConfigChanged() override
        {
            if (mCropEditor)
                mCropEditor->SetNode(mNode, "crop");

            if (mResize)
                mResize->SetValue(GetBool("resize", false));

            UpdateFields();
        }

    private:
        Ref<EditBox> mPixel[4]; // The frame in the picture's pixels: x, y, w, h
        Ref<Button>  mWhole;    // Puts the frame round the whole picture
        Ref<Toggle>  mResize;   // Scales the cut to a new size
        Ref<EditBox> mResizeW;  // Target width; the cut's own size while resizing is off
        Ref<EditBox> mResizeH;  // Target height
        Ref<Button>  mLock;     // Keeps the proportions while one side is typed

        static Ref<EditBox> MakeNumberField(const String& name)
        {
            auto edit = MakeEditBox("", false, "");
            edit->name = name;
            edit->SetFilterInteger();
            if (auto text = edit->GetTextDrawable())
            {
                text->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::MonoFont()));
                text->height = PipelineTheme::Size(monoFont);
            }
            edit->SetViewLayout(Layout::BothStretch(6, 0, 6, 0));
            edit->layout->minWidth = 40;
            return edit;
        }

        // A number field with its letter inside on the left (.numf)
        static Ref<Widget> MakeLettered(const String& letter, const Ref<EditBox>& field)
        {
            auto box = mmake<Widget>();
            box->name = "numf " + letter;
            box->layout->minWidth = 46;
            *field->layout = WidgetLayout::BothStretch();
            field->SetViewLayout(Layout::BothStretch(21, 0, 4, 0));
            box->AddChild(field);
            auto label = MakeLabel(letter, true);
            StyleLabel(label, 10.5f, 600, PipelineTheme::Get().fg3);
            *label->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(6, 0), Vec2F(16, 0));
            box->AddChild(label);
            return box;
        }

        Vec2I InputSize() const
        {
            return mCropEditor && mCropEditor->HasImage() && GetInput("image").IsImage() ? mCropEditor->GetImageSize() : Vec2I();
        }

        PipelineImageOps::CropRect Frame() const
        {
            PipelineImageOps::CropRect crop;
            if (!ReadNodeCrop(*mNode, "crop", crop))
                crop = PipelineImageOps::CropRect{ 0.0f, 0.0f, 1.0f, 1.0f };
            return crop;
        }

        // The frame in pixels of the picture, as the web rounds it
        bool FramePixels(int px[4]) const
        {
            Vec2I size = InputSize();
            if (size.x <= 0 || size.y <= 0)
                return false;

            auto crop = Frame();
            px[0] = (int)std::lround(crop.x*size.x);
            px[1] = (int)std::lround(crop.y*size.y);
            px[2] = Math::Max(1, (int)std::lround(crop.w*size.x));
            px[3] = Math::Max(1, (int)std::lround(crop.h*size.y));
            return true;
        }

        void WriteFrame(float x, float y, float w, float h)
        {
            if (!mNode->config.IsObject())
                mNode->config.SetObject();

            mNode->RemoveConfig("crop");
            auto& crop = mNode->config["crop"];
            crop.SetObject();
            crop["x"] = x;
            crop["y"] = y;
            crop["w"] = w;
            crop["h"] = h;
            if (mCropEditor)
                mCropEditor->SetNode(mNode, "crop");

            Notify("crop", true);
            UpdateFields();
        }

        // A typed value moves that edge or size; the frame stays inside the picture and at least a pixel big
        void OnPixelEdited(int index, const String& text)
        {
            int px[4];
            if (text.IsEmpty() || !FramePixels(px))
            {
                UpdateFields();
                return;
            }

            Vec2I size = InputSize();
            px[index] = atoi(text.Data());
            px[0] = Math::Clamp(px[0], 0, size.x - 1);
            px[1] = Math::Clamp(px[1], 0, size.y - 1);
            px[2] = Math::Clamp(px[2], 1, size.x - px[0]);
            px[3] = Math::Clamp(px[3], 1, size.y - px[1]);
            WriteFrame((float)px[0]/size.x, (float)px[1]/size.y, (float)px[2]/size.x, (float)px[3]/size.y);
        }

        void SetWhole()
        {
            WriteFrame(0.0f, 0.0f, 1.0f, 1.0f);
        }

        // Size of the cut: the input scaled by the frame; zero while nothing is connected
        Vec2I CutSize() const
        {
            int px[4];
            return FramePixels(px) ? Vec2I(px[2], px[3]) : Vec2I();
        }

        void OnResizeToggled(bool on)
        {
            // Turned on, the fields start from the size the cut has
            Vec2I size = CutSize();
            if (on && GetNumber("resizeW", 0) <= 0 && size.x > 0)
                mNode->SetConfigNumber("resizeW", (float)size.x);
            if (on && GetNumber("resizeH", 0) <= 0 && size.y > 0)
                mNode->SetConfigNumber("resizeH", (float)size.y);

            SetBool("resize", on, true);
            UpdateFields();
        }

        void OnResizeEdited(bool width, int value)
        {
            // Typing a size while resizing is off asks for it
            if (!GetBool("resize", false))
            {
                mNode->SetConfigBool("resize", true);
                mResize->SetValue(true);
            }

            value = Math::Max(value, 1);
            Vec2I size = CutSize();
            bool lock = GetBool("lockAspect", true) && size.x > 0 && size.y > 0;
            if (width)
            {
                if (lock)
                    mNode->SetConfigNumber("resizeH", (float)Math::Max(1, (int)Math::Round(value*(float)size.y/size.x)));
                SetNumber("resizeW", (float)value, true);
            }
            else
            {
                if (lock)
                    mNode->SetConfigNumber("resizeW", (float)Math::Max(1, (int)Math::Round(value*(float)size.x/size.y)));
                SetNumber("resizeH", (float)value, true);
            }

            UpdateFields();
        }

        void UpdateFields()
        {
            if (!mResizeW)
                return;

            int px[4];
            bool known = FramePixels(px);
            for (int i = 0; i < 4; i++)
            {
                if (!mPixel[i]->IsFocused())
                    mPixel[i]->SetText(known ? (String)px[i] : String());
                mPixel[i]->interactable = known;
                mPixel[i]->transparency = known ? 1.0f : 0.55f;
            }

            auto crop = Frame();
            bool whole = crop.x <= 0.001f && crop.y <= 0.001f && crop.w >= 0.999f && crop.h >= 0.999f;
            mWhole->interactable = !whole;
            mWhole->transparency = whole ? 0.4f : 1.0f;

            // Resizing off, the fields show the size of the cut
            bool on = GetBool("resize", false);
            Vec2I size = CutSize();
            auto show = [&](const Ref<EditBox>& edit, float configured, int natural)
            {
                int value = on && configured > 0 ? (int)configured : natural;
                if (!edit->IsFocused())
                    edit->SetText(value > 0 ? (String)value : String());
                edit->transparency = on ? 1.0f : 0.55f;
            };
            show(mResizeW, GetNumber("resizeW", 0), size.x);
            show(mResizeH, GetNumber("resizeH", 0), size.y);

            bool lock = GetBool("lockAspect", true);
            mLock->interactable = on;
            mLock->transparency = !on ? 0.3f : lock ? 1.0f : 0.5f;
        }
    };

    Ref<PipelineNodeBody> CreateCropNodeBody(const String& type)
    {
        return type == "imageCrop" ? mmake<ImageCropBody>() : nullptr;
    }
}
