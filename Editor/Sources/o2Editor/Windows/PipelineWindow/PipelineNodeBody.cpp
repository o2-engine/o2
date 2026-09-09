#include "o2Editor/stdafx.h"
#include "PipelineNodeBody.h"

#include "o2Editor/Windows/PipelineWindow/PipelineNodeBodyFactories.h"

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

    PipelineNodeBody::PipelineNodeBody(RefCounter* refCounter):
        Widget(refCounter)
    {}

    void PipelineNodeBody::Init(const Ref<PipelineNodeWidget>& owner)
    {
        mOwner = owner;
        mNode = owner->GetNode();
        mEditor = owner->GetEditor();
    }

    float PipelineNodeBody::GetPreferredHeight(float width) const
    {
        float rowWidth = width - mPadding * 2.0f;
        float total = mSpacing;
        for (auto& row : mRows)
            total += RowHeight(row, rowWidth) + mSpacing;
        return total;
    }

    float PipelineNodeBody::RowHeight(const Row& row, float rowWidth)
    {
        return row.heightForWidth ? row.heightForWidth(rowWidth) : row.height;
    }

    Ref<Widget> PipelineNodeBody::AddRow(const Ref<Widget>& widget, float height)
    {
        Row row;
        row.widget = widget;
        row.height = height;
        row.flexible = false;
        mRows.Add(row);
        AddChild(widget);
        return widget;
    }

    Ref<Widget> PipelineNodeBody::AddRow(const Ref<Widget>& widget, const Function<float(float)>& heightForWidth)
    {
        Row row;
        row.widget = widget;
        row.heightForWidth = heightForWidth;
        row.flexible = false;
        mRows.Add(row);
        AddChild(widget);
        return widget;
    }

    Ref<Widget> PipelineNodeBody::AddFlexible(const Ref<Widget>& widget, float minHeight)
    {
        Row row;
        row.widget = widget;
        row.height = minHeight;
        row.flexible = true;
        mRows.Add(row);
        AddChild(widget);
        return widget;
    }

    Ref<Widget> PipelineNodeBody::AddFlexible(const Ref<Widget>& widget, const Function<float(float)>& minHeightForWidth)
    {
        Row row;
        row.widget = widget;
        row.heightForWidth = minHeightForWidth;
        row.flexible = true;
        mRows.Add(row);
        AddChild(widget);
        return widget;
    }

    void PipelineNodeBody::MarkContent(const Ref<Widget>& widget)
    {
        for (auto& row : mRows)
        {
            if (row.widget == widget)
                row.content = true;
        }
    }

    void PipelineNodeBody::Draw()
    {
        if (!PipelineControls::IsFarView())
        {
            Widget::Draw();
            return;
        }

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        DrawLayers();
        OnDrawn();
        DrawFarContent();
        DrawTopLayers();
    }

    void PipelineNodeBody::DrawFarContent()
    {
        for (auto& row : mRows)
        {
            if (row.content)
                row.widget->Draw();
        }
    }

    void PipelineNodeBody::ClearRows()
    {
        for (auto& row : mRows)
            RemoveChild(row.widget);
        mRows.Clear();
        mImageView = nullptr;
        mTextView = nullptr;
        mAudioView = nullptr;
        mCropEditor = nullptr;
        mVideoView = nullptr;
    }

    void PipelineNodeBody::Relayout(float width, float height)
    {
        float rowWidth = width - mPadding * 2.0f;
        float fixed = mSpacing;
        int flexibleCount = 0;
        float flexibleMin = 0.0f;
        for (auto& row : mRows)
        {
            fixed += mSpacing;
            if (row.flexible) { flexibleCount++; flexibleMin += RowHeight(row, rowWidth); }
            else fixed += RowHeight(row, rowWidth);
        }

        // Every flexible row keeps its own minimum; only the height left over is shared equally
        float bonus = flexibleCount > 0 ? Math::Max(0.0f, height - fixed - flexibleMin) / flexibleCount : 0.0f;

        float y = mSpacing;
        for (auto& row : mRows)
        {
            float h = RowHeight(row, rowWidth) + (row.flexible ? bonus : 0.0f);
            *row.widget->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(mPadding, -y - h), Vec2F(-mPadding, -y));
            y += h + mSpacing;
        }
    }

    String PipelineNodeBody::GetString(const String& key, const String& def /*= ""*/) const { return mNode->GetConfigString(key, def); }
    float PipelineNodeBody::GetNumber(const String& key, float def /*= 0.0f*/) const { return mNode->GetConfigNumber(key, def); }
    bool PipelineNodeBody::GetBool(const String& key, bool def /*= false*/) const { return mNode->GetConfigBool(key, def); }

    void PipelineNodeBody::Notify(const String& key, bool completed)
    {
        if (auto editor = mEditor.Lock())
        {
            if (auto owner = mOwner.Lock())
                editor->OnNodeConfigChanged(owner, key, completed);
        }
    }

    void PipelineNodeBody::SetString(const String& key, const String& value, bool completed /*= true*/)
    {
        if (mNode->GetConfigString(key, "\x01") == value && completed == true && mNode->HasConfig(key))
        {
            Notify(key, completed);
            return;
        }
        mNode->SetConfigString(key, value);
        Notify(key, completed);
    }

    void PipelineNodeBody::SetNumber(const String& key, float value, bool completed /*= true*/)
    {
        mNode->SetConfigNumber(key, value);
        Notify(key, completed);
    }

    void PipelineNodeBody::SetBool(const String& key, bool value, bool completed /*= true*/)
    {
        mNode->SetConfigBool(key, value);
        Notify(key, completed);
    }

    void PipelineNodeBody::RebuildBody()
    {
        if (auto owner = mOwner.Lock())
        {
            auto editor = mEditor.Lock();
            owner->Rebuild();
            owner->UpdateFromNode();
        }
    }

    PipelineValue PipelineNodeBody::GetOutput() const
    {
        auto owner = mOwner.Lock();
        return owner ? owner->GetRuntime().output : PipelineValue();
    }

    Ref<Bitmap> PipelineNodeBody::GetOutputBitmap() const
    {
        auto value = GetOutput();
        return value.IsImage() ? value.GetBitmap() : nullptr;
    }

    Ref<Bitmap> PipelineNodeBody::GetSourceOutputBitmap() const
    {
        auto owner = mOwner.Lock();
        if (!owner)
            return nullptr;

        auto& rt = owner->GetRuntime();
        if (!rt.srcPreviewPath.IsEmpty() && o2FileSystem.IsFileExist(rt.srcPreviewPath))
        {
            if (auto bmp = DecodeImageBytes(PipelineUtils::ReadFileBytes(rt.srcPreviewPath)))
                return bmp;
        }
        return GetOutputBitmap();
    }

    PipelineValue PipelineNodeBody::GetInput(const String& portName) const
    {
        auto editor = mEditor.Lock();
        return editor ? editor->GetInputValue(mNode, portName) : PipelineValue();
    }

    void PipelineNodeBody::OnOutputChanged()
    {
        auto value = GetOutput();
        if (mImageView)
        {
            mImageView->SetBitmap(value.IsImage() ? value.GetBitmap() : nullptr);
        }
        if (mCropEditor)
        {
            bool isFinish = PipelineNodeRegistry::IsFinishType(mNode->nodeType);
            Ref<Bitmap> source = isFinish ? (GetInput("in").IsImage() ? GetInput("in").GetBitmap() : nullptr) : GetSourceOutputBitmap();
            mCropEditor->SetBitmap(source);
        }
        if (mTextView)
            mTextView->SetText(value.IsText() ? value.data : String());
        if (mAudioView)
            mAudioView->SetAudio(value, value.IsAudio() ? "Result (" + PipelineUtils::ExtensionForMime(value.mimeType) + ")" : "");
        if (mVideoView)
        {
            String cacheDir;
            if (value.IsVideo() && !value.data.IsEmpty())
            {
                auto editor = mEditor.Lock();
                String pipelineId = editor ? editor->GetPipelineId() : String("none");
                cacheDir = PipelineExecutor::GetCachePath(pipelineId) + "video/" + PipelineUtils::Fnv1a64Hex(value.data) + "/";
            }
            mVideoView->SetVideo(value.IsVideo() ? value : PipelineValue(), cacheDir);
        }
    }

    Ref<DropDown> PipelineNodeBody::AddModelRow(const Vector<String>& presets, const String& defaultModel)
    {
        return AddSelectRow("Model", "model", presets, defaultModel);
    }

    Ref<DropDown> PipelineNodeBody::AddSelectRow(const String& label, const String& key, const Vector<String>& options, const String& def)
    {
        auto dropdown = MakeDropDown(options, GetString(key, def));
        String k = key;
        WeakRef<PipelineNodeBody> weakThis(this);
        dropdown->onSelectedText = [weakThis, k](const WString& text)
        {
            if (auto self = weakThis.Lock())
            {
                if (self->mNode->GetConfigString(k, "") != (String)text)
                    self->SetString(k, (String)text, true);
            }
        };
        AddRow(MakeRow(label, dropdown), 22);
        return dropdown;
    }

    Ref<EditBox> PipelineNodeBody::AddTextArea(const String& key, const String& placeholder, float height)
    {
        auto edit = MakeEditBox(GetString(key, ""), true, placeholder);
        String k = key;
        WeakRef<PipelineNodeBody> weakThis(this);
        edit->onChanged = [weakThis, k](const WString& text) { if (auto self = weakThis.Lock()) self->SetString(k, (String)text, false); };
        edit->onChangeCompleted = [weakThis, k](const WString& text) { if (auto self = weakThis.Lock()) self->SetString(k, (String)text, true); };
        if (height <= 0)
            AddFlexible(edit, 60);
        else
            AddRow(edit, height);
        return edit;
    }

    Ref<EditBox> PipelineNodeBody::AddTextRow(const String& label, const String& key, const String& placeholder /*= ""*/)
    {
        auto edit = MakeEditBox(GetString(key, ""), false, placeholder);
        String k = key;
        WeakRef<PipelineNodeBody> weakThis(this);
        edit->onChangeCompleted = [weakThis, k](const WString& text) { if (auto self = weakThis.Lock()) self->SetString(k, (String)text, true); };
        AddRow(MakeRow(label, edit), 22);
        return edit;
    }

    Ref<Toggle> PipelineNodeBody::AddCheckbox(const String& caption, const String& key, bool def)
    {
        auto toggle = MakeCheckbox(caption, GetBool(key, def));
        String k = key;
        WeakRef<PipelineNodeBody> weakThis(this);
        toggle->onToggleByUser = [weakThis, k](bool value) { if (auto self = weakThis.Lock()) self->SetBool(k, value, true); };
        AddRow(toggle, 20);
        return toggle;
    }

    Ref<PipelineSlider> PipelineNodeBody::AddSlider(const String& label, const String& key, float minValue, float maxValue, float step, float def, const String& suffix /*= ""*/)
    {
        auto slider = mmake<PipelineSlider>();
        slider->Setup(label, minValue, maxValue, step, GetNumber(key, def), suffix);
        String k = key;
        WeakRef<PipelineNodeBody> weakThis(this);
        slider->onChanged = [weakThis, k](float value, bool completed) { if (auto self = weakThis.Lock()) self->SetNumber(k, value, completed); };
        AddRow(slider, 20);
        return slider;
    }

    Ref<PipelineColorField> PipelineNodeBody::AddColor(const String& label, const String& key, const String& defHex)
    {
        Color4 color;
        if (!PipelineUtils::ParseHexColor(GetString(key, defHex), color))
            PipelineUtils::ParseHexColor(defHex, color);

        auto field = mmake<PipelineColorField>();
        field->Setup(label, color);
        String k = key;
        WeakRef<PipelineNodeBody> weakThis(this);
        field->onChanged = [weakThis, k](const Color4& value, bool completed)
        {
            if (auto self = weakThis.Lock())
                self->SetString(k, PipelineUtils::ColorToHex(value), completed);
        };
        AddRow(field, 22);
        return field;
    }

    void PipelineNodeBody::AddSegmented(const String& key, const Vector<Pair<String, String>>& options, const String& def)
    {
        auto row = mmake<HorizontalLayout>();
        row->spacing = 4;
        row->expandWidth = true;
        row->expandHeight = true;
        row->baseCorner = BaseCorner::Left;

        String current = GetString(key, def);
        for (auto& option : options)
        {
            auto toggle = MakeSegment(option.second, option.first == current);
            String value = option.first;
            String k = key;
            WeakRef<PipelineNodeBody> weakThis(this);
            toggle->onToggleByUser = [weakThis, k, value](bool)
            {
                if (auto self = weakThis.Lock())
                {
                    self->SetString(k, value, true);
                    self->RebuildBody();
                }
            };
            row->AddChild(toggle);
        }
        AddRow(row, 22);
    }

    void PipelineNodeBody::AddMutedLine(const String& text, float height /*= 18.0f*/)
    {
        auto label = MakeLabel(text, true);
        auto owner = mOwner.Lock();
        float width = owner ? owner->GetCardSize().x - mPadding * 2 : 240.0f;
        int perLine = Math::Max(8, (int)(width / 6.6f));
        int lines = Math::Max(1, (text.Length() + perLine - 1) / perLine);
        AddRow(label, Math::Max(height, lines * 16.0f + 2.0f));
    }

    void PipelineNodeBody::AddSeedRow()
    {
        auto row = mmake<HorizontalLayout>();
        row->spacing = 6;
        row->expandWidth = true;
        row->expandHeight = true;
        row->baseCorner = BaseCorner::Left;

        bool inherit = GetBool("inheritSeed", true);
        auto toggle = MakeCheckbox("Inherit seed", inherit);
        toggle->layout->minWidth = 110;
        toggle->layout->maxWidth = 110;
        WeakRef<PipelineNodeBody> weakThis(this);
        toggle->onToggleByUser = [weakThis](bool value)
        {
            if (auto self = weakThis.Lock())
            {
                self->SetBool("inheritSeed", value, true);
                self->RebuildBody();
            }
        };
        row->AddChild(toggle);

        auto seedEdit = MakeEditBox(GetString("seed", ""), false, "auto");
        seedEdit->SetFilterInteger();
        seedEdit->interactable = !inherit;
        if (inherit)
            seedEdit->transparency = 0.5f;
        seedEdit->onChangeCompleted = [weakThis](const WString& text)
        {
            if (auto self = weakThis.Lock())
            {
                String s = ((String)text).Trimed();
                if (s.IsEmpty()) { self->mNode->RemoveConfig("seed"); self->Notify("seed", true); }
                else self->SetNumber("seed", Math::Max(0.0f, (float)Math::Floor((float)atof(s.Data()))), true);
            }
        };
        row->AddChild(seedEdit);
        AddRow(row, 22);
    }

    void PipelineNodeBody::AddTransparencyBlock()
    {
        bool on = GetBool("transparentBg", false);
        auto toggle = MakeCheckbox("Transparent background", on);
        WeakRef<PipelineNodeBody> weakThis(this);
        toggle->onToggleByUser = [weakThis](bool value)
        {
            if (auto self = weakThis.Lock())
            {
                self->SetBool("transparentBg", value, true);
                self->RebuildBody();
            }
        };
        AddRow(toggle, 20);

        if (!on)
            return;

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
                if (auto regular = swatch->GetLayerDrawable<Sprite>("regular")) regular->color = pc;
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

    void PipelineNodeBody::AddResultImage(const String& hint, float minHeight /*= 120.0f*/)
    {
        mImageView = mmake<PipelineImageView>();
        mImageView->SetHint(hint);
        AddFlexible(mImageView, minHeight);
        MarkContent(mImageView);
    }

    void PipelineNodeBody::AddResultText(const String& hint, float minHeight /*= 70.0f*/)
    {
        mTextView = mmake<PipelineTextView>();
        mTextView->SetHint(hint);
        AddFlexible(mTextView, minHeight);
        MarkContent(mTextView);
    }

    void PipelineNodeBody::AddResultAudio(const String& hint)
    {
        mAudioView = mmake<PipelineAudioView>();
        mAudioView->SetHint(hint);
        AddRow(mAudioView, PipelineAudioView::height);
        MarkContent(mAudioView);
    }

    void PipelineNodeBody::AddResultVideo(const String& hint, float minHeight /*= 170.0f*/)
    {
        mVideoView = mmake<PipelineVideoView>();
        mVideoView->SetHint(hint);
        AddFlexible(mVideoView, minHeight);
        MarkContent(mVideoView);
    }

    void PipelineNodeBody::AddCropSection(const String& hint, float minHeight /*= 150.0f*/)
    {
        auto head = mmake<HorizontalLayout>();
        head->spacing = 6;
        head->expandWidth = true;
        head->expandHeight = true;
        head->baseCorner = BaseCorner::Left;

        auto caption = MakeLabel("Result", true);
        head->AddChild(caption);

        bool cropOn = GetBool("cropEnabled", false);
        auto cropButton = MakeSegment("Crop", cropOn);
        cropButton->layout->minWidth = 70;
        cropButton->layout->maxWidth = 70;
        WeakRef<PipelineNodeBody> weakThis(this);
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
        head->AddChild(cropButton);
        AddRow(head, 20);

        mCropEditor = mmake<PipelineCropEditor>();
        mCropEditor->SetHint(hint);
        mCropEditor->SetNode(mNode, "crop");
        mCropEditor->SetCropEnabled(cropOn);
        mCropEditor->onCropChanged = [weakThis](bool completed)
        {
            if (auto self = weakThis.Lock())
                self->Notify("crop", completed);
        };
        AddFlexible(mCropEditor, minHeight);
        MarkContent(mCropEditor);
    }

    void PipelineNodeBody::AddFilePicker(const String& buttonCaption, const Vector<String>& extensions, bool image)
    {
        auto button = MakeButton(buttonCaption);
        WeakRef<PipelineNodeBody> weakThis(this);
        Vector<String> exts = extensions;
        button->onClick = [weakThis, exts]()
        {
            auto self = weakThis.Lock();
            if (!self) return;

            Map<String, String> filter;
            String joined;
            for (auto& e : exts)
                joined += (joined.IsEmpty() ? "" : ";") + String("*.") + e;
            filter["Files"] = joined;

            String file = GetOpenFileNameDialog("Choose file", filter, "");
            if (file.IsEmpty())
                return;

            // A file inside the project's Assets folder is referenced as an asset; anything else is copied into uploads
            String assetsPath = o2FileSystem.CanonicalizePath(o2Assets.GetAssetsPath());
            String canonical = o2FileSystem.CanonicalizePath(file);
            if (!assetsPath.IsEmpty() && canonical.StartsWith(assetsPath))
            {
                self->mNode->SetConfigString("assetPath", canonical.SubStr(assetsPath.Length()));
                self->mNode->RemoveConfig("uploadId");
            }
            else
            {
                String uploadId = PipelineUtils::StoreUpload(file);
                if (uploadId.IsEmpty())
                    return;
                self->mNode->SetConfigString("uploadId", uploadId);
                self->mNode->RemoveConfig("assetPath");
            }
            self->mNode->SetConfigString("name", o2FileSystem.GetPathWithoutDirectories(file));
            self->Notify("uploadId", true);
            self->RebuildBody();
        };
        AddRow(button, 24);
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
// --- META ---

DECLARE_CLASS(Editor::PipelineNodeBody, Editor__PipelineNodeBody);

DECLARE_CLASS(Editor::PipelineCropEditor, Editor__PipelineCropEditor);
// --- END META ---
