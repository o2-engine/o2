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
#include "o2Editor/Pipeline/PipelineUpscale.h"
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

    float PipelineNodeBody::ParamsList::GetHeight() const
    {
        float total = 0.0f;
        for (float h : heights)
            total += h;
        return total + spacing * Math::Max(0, heights.Count() - 1);
    }

    Ref<Widget> PipelineNodeBody::AddRow(const Ref<Widget>& widget, float height)
    {
        // Inside an open parameter list the row is one of its lines
        if (mParams)
        {
            widget->layout->minHeight = height;
            widget->layout->maxHeight = height;
            mParams->list->AddChild(widget);
            mParams->heights.Add(height);
            return widget;
        }

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
        if (mParams)
        {
            auto owner = mOwner.Lock();
            float width = (owner ? owner->GetCardSize().x : 260.0f) - mPadding * 2.0f;
            return AddRow(widget, heightForWidth(width));
        }

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
        if (mParams)
            return AddRow(widget, minHeight);

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

    void PipelineNodeBody::ShowAssetFolderMenu(const Function<void(const String&)>& pick)
    {
        PushEditorScopeOnStack scope;
        auto editor = mEditor.Lock();
        if (!editor)
            return;

        Vector<String> folders;
        for (auto& weak : o2Assets.GetAssetsTree().allAssets)
        {
            auto info = weak.Lock();
            if (info && info->meta && info->meta->GetAssetType() == &TypeOf(FolderAsset))
                folders.Add(info->path);
        }
        folders.Sort([](const String& a, const String& b) { return a < b; });

        Vector<Pair<String, Function<void()>>> items;
        items.Add({ "Assets", [pick]() { pick(""); } });
        for (auto& folder : folders)
        {
            String value = folder;
            // A slash in a menu label opens a submenu, so the path is shown with another separator
            items.Add({ folder.ReplacedAll("/", " > "), [pick, value]() { pick(value); } });
        }
        editor->ShowPopupMenu(items);
    }

    void PipelineNodeBody::ClearRows()
    {
        for (auto& row : mRows)
            RemoveChild(row.widget);
        mRows.Clear();
        mParams = nullptr;
        mImageView = nullptr;
        mTextView = nullptr;
        mAudioView = nullptr;
        mCropEditor = nullptr;
        mVideoView = nullptr;
    }

    bool PipelineNodeBody::GetRowPlacement(const Ref<Widget>& widget, float width, float height, float& top, float& rowHeight) const
    {
        float rowWidth = width - mPadding*2.0f;
        float fixed = mSpacing, flexibleMin = 0.0f;
        int flexibleCount = 0;
        for (auto& row : mRows)
        {
            fixed += mSpacing;
            if (row.flexible) { flexibleCount++; flexibleMin += RowHeight(row, rowWidth); }
            else fixed += RowHeight(row, rowWidth);
        }

        float bonus = flexibleCount > 0 ? Math::Max(0.0f, height - fixed - flexibleMin)/flexibleCount : 0.0f;
        float y = mSpacing;
        for (auto& row : mRows)
        {
            float h = RowHeight(row, rowWidth) + (row.flexible ? bonus : 0.0f);
            if (row.widget == widget)
            {
                top = y;
                rowHeight = h;
                return true;
            }
            y += h + mSpacing;
        }
        return false;
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
            *row.widget->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(mPadding, -y - Math::Max(h, 0.0f)), Vec2F(-mPadding, -y));
            y += h + mSpacing;
        }
    }

    const DataValue* PipelineNodeBody::FindConfigValue(const String& key) const
    {
        if (PipelineTransparency::SettingKeys().Contains(key))
        {
            if (auto part = GetPartTransparency())
            {
                if (auto value = part->FindMember(key.Data()))
                    return value;
            }
        }

        return mNode->GetConfigValue(key);
    }

    DataValue& PipelineNodeBody::ConfigSlot(const String& key)
    {
        auto part = PipelineTransparency::SettingKeys().Contains(key) ? GetPartTransparency() : nullptr;
        if (part)
            return (*part)[key.Data()];

        if (!mNode->config.IsObject())
            mNode->config.SetObject();

        return mNode->config[key.Data()];
    }

    String PipelineNodeBody::GetString(const String& key, const String& def /*= ""*/) const
    {
        auto value = FindConfigValue(key);
        return value ? PipelineUtils::ValueToString(*value, def) : def;
    }

    float PipelineNodeBody::GetNumber(const String& key, float def /*= 0.0f*/) const
    {
        auto value = FindConfigValue(key);
        return value ? PipelineUtils::ValueToNumber(*value, def) : def;
    }

    bool PipelineNodeBody::GetBool(const String& key, bool def /*= false*/) const
    {
        auto value = FindConfigValue(key);
        return value ? PipelineUtils::ValueToBool(*value, def) : def;
    }

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
        auto current = FindConfigValue(key);
        if (!current || !completed || PipelineUtils::ValueToString(*current, "\x01") != value)
            ConfigSlot(key) = value;

        Notify(key, completed);
    }

    void PipelineNodeBody::SetNumber(const String& key, float value, bool completed /*= true*/)
    {
        ConfigSlot(key) = value;
        Notify(key, completed);
    }

    void PipelineNodeBody::SetBool(const String& key, bool value, bool completed /*= true*/)
    {
        ConfigSlot(key) = value;
        Notify(key, completed);
    }

    void PipelineNodeBody::RebuildBody()
    {
        PushEditorScopeOnStack scope;
        if (auto owner = mOwner.Lock())
        {
            auto editor = mEditor.Lock();
            owner->Rebuild();
            owner->UpdateFromNode();
        }
    }

    void PipelineNodeBody::RebuildBodyKeepingArea()
    {
        PushEditorScopeOnStack scope;
        if (auto owner = mOwner.Lock())
            owner->KeepAreaThrough([owner]() { owner->Rebuild(); });
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
        bool isFinish = PipelineNodeRegistry::IsFinishType(mNode->nodeType);
        auto value = GetOutput();
        // A finish node saves what reaches it, so it shows its input - visible before the node itself has run
        if (isFinish && !value.IsValid())
            value = GetInput("in");

        if (mImageView)
        {
            mImageView->SetBitmap(value.IsImage() ? value.GetBitmap() : nullptr);
        }
        if (mCropEditor)
        {
            Ref<Bitmap> source = isFinish ? (value.IsImage() ? value.GetBitmap() : nullptr) : GetSourceOutputBitmap();
            mCropEditor->SetBitmap(source);
        }
        if (mPair)
        {
            auto input = GetInput(PipelinePairLayout::InputPortOf(mNode->nodeType));
            mPair->SetCropOn(mCropEditor && GetBool("cropEnabled", false));
            mPair->SetImages(input.IsImage() ? input.GetBitmap() : nullptr, value.IsImage() ? value.GetBitmap() : nullptr);
        }
        if (mTextView)
            mTextView->SetText(value.IsText() ? value.data : String());
        if (mAudioView)
            mAudioView->SetAudio(value, value.IsAudio() ? "Result (" + PipelineUtils::ExtensionForMime(value.mimeType) + ")" : String());
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

    bool PipelineNodeBody::BeginParams(const Vector<String>& names)
    {
        bool open = GetBool("paramsOpen", false);

        String caption = "Parameters";
        for (int i = 0; i < names.Count() && i < 4; i++)
            caption += " · " + names[i];
        if (names.Count() > 4)
            caption += " · ...";

        auto head = o2UI.CreateWidget<Button>("pipeline icon");
        head->name = "params";
        auto arrow = mmake<PipelineFoldArrow>();
        // The dimmed text colour over the card back, opaque
        arrow->color = Color4(147, 167, 176, 255);
        arrow->open = open ? 1.0f : 0.0f;
        if (auto iconLayer = head->GetLayer("icon"))
        {
            iconLayer->SetDrawable(arrow);
            iconLayer->layout = Layout::Based(BaseCorner::Left, Vec2F(10, 10), Vec2F(0, 0));
        }

        auto text = mmake<Text>("stdFont.ttf");
        text->text = caption;
        text->horAlign = HorAlign::Left;
        text->verAlign = VerAlign::Middle;
        text->dotsEngings = true;
        text->color = dimTextColor;
        head->AddLayer("caption", text, Layout::BothStretch(14, 0, 2, 0));

        WeakRef<PipelineNodeBody> weakThis(this);
        head->onClick = [weakThis, open]()
        {
            auto self = weakThis.Lock();
            if (auto owner = self ? self->mOwner.Lock() : nullptr)
                owner->SetParamsOpen(!open);
        };
        AddRow(head, 20);

        if (!open)
            return false;

        auto params = mmake<ParamsList>();
        params->spacing = mSpacing;
        params->list = mmake<VerticalLayout>();
        params->list->name = "params list";
        params->list->spacing = mSpacing;
        params->list->expandWidth = true;
        params->list->expandHeight = false;
        params->list->fitByChildren = false;
        params->list->baseCorner = BaseCorner::Top;

        // The row is registered before its lines exist: the height is read when the body is laid out
        AddRow(params->list, [params](float) { return params->GetHeight(); });
        mParams = params;
        return true;
    }

    void PipelineNodeBody::EndParams()
    {
        mParams = nullptr;
    }

    void PipelineNodeBody::AddActions(const Vector<Ref<Widget>>& buttons)
    {
        auto row = mmake<HorizontalLayout>();
        row->spacing = 6;
        row->expandWidth = true;
        row->expandHeight = true;
        row->baseCorner = BaseCorner::Left;
        for (auto& button : buttons)
            row->AddChild(button);
        AddRow(row, 24);
    }

    Ref<EditBox> PipelineNodeBody::AddPrimaryField(const String& key, const String& placeholder)
    {
        return AddTextArea(key, placeholder, 46);
    }

    void PipelineNodeBody::AddSectionTitle(const String& title)
    {
        AddRow(MakeLabel(title, true), 18);
    }

    Ref<Button> PipelineNodeBody::AddModelRow(const Vector<String>& presets, const String& defaultModel, PipelineModelKind kind)
    {
        // The field shows the product name, the config keeps the model id; the menu groups and filters the list
        auto field = MakeModelField();
        SetModelFieldValue(field, GetString("model", defaultModel), kind);

        WeakRef<PipelineNodeBody> weakThis(this);
        WeakRef<Button> weakField(field);
        field->onClick = [weakThis, weakField, presets, defaultModel, kind]()
        {
            auto self = weakThis.Lock();
            auto fieldRef = weakField.Lock();
            auto editor = self ? self->mEditor.Lock() : nullptr;
            if (!self || !fieldRef || !editor)
                return;

            // A click on the field of the open menu closes it: the press already did, the click must not reopen it
            if (auto picker = editor->GetModelPicker())
            {
                if (picker->IsOpenFor(fieldRef))
                {
                    picker->Close();
                    return;
                }

                if (picker->ConsumeClosedByField(fieldRef))
                    return;
            }

            RectF world = fieldRef->layout->GetWorldRect();
            Vec2F a = editor->LocalToScreenPoint(Vec2F(world.left, world.bottom));
            Vec2F b = editor->LocalToScreenPoint(Vec2F(world.right, world.top));

            PipelineModelPickerRequest request;
            request.ids = presets;
            request.kind = kind;
            request.current = self->GetString("model", defaultModel);
            request.anchor = RectF(Math::Min(a.x, b.x), Math::Max(a.y, b.y), Math::Max(a.x, b.x), Math::Min(a.y, b.y));
            request.field = fieldRef;
            request.onPick = [weakThis, weakField, defaultModel, kind](const String& id)
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;

                String previous = self->GetString("model", defaultModel);
                if (previous == id)
                    return;

                self->SetString("model", id, true);

                // The background rows depend on whether the model renders the alpha itself, the upscale note on whether it
                // renders 2K / 4K itself
                bool upscaleNote = self->mNode->nodeType == "aiUpscale" &&
                    PipelineUpscale::RendersLargeSizes(previous) != PipelineUpscale::RendersLargeSizes(id);
                if (PipelineTransparency::SupportsNativeTransparency(previous) != PipelineTransparency::SupportsNativeTransparency(id) || upscaleNote)
                    self->RebuildBodyKeepingArea();
                else
                    SetModelFieldValue(weakField.Lock(), id, kind);
            };
            editor->ShowModelPicker(request);
        };

        AddRow(MakeRow("Model", field), 22);
        return field;
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
        row->name = key + " options";
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
                    self->RebuildBodyKeepingArea();
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
}
// --- META ---

DECLARE_CLASS(Editor::PipelineNodeBody, Editor__PipelineNodeBody);

DECLARE_CLASS(Editor::PipelineCropEditor, Editor__PipelineCropEditor);
// --- END META ---
