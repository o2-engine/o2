#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
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

    static const Vector<String> textModelPresets = {
        "gemini-pro-latest", "gemini-flash-latest", "gemini-3.5-pro", "gemini-3.5-flash", "gemini-3.1-pro-preview",
        "gemini-3-flash-preview", "gemini-3.1-flash-lite", "gemini-2.5-pro", "gemini-2.5-flash", "gemini-2.5-flash-lite"
    };

    static const Vector<String> videoModelPresets = {
        "veo-3.1-fast-generate-preview", "veo-3.1-generate-preview", "veo-3.0-fast-generate-001", "veo-3.0-generate-001",
        "veo-2.0-generate-001", "kling-v2-1-master", "kling-v2-1", "kling-v2-master", "kling-v1-6"
    };

    class FinishBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        PipelinePortType kind = PipelinePortType::Image;

        void Build() override
        {
            mExtension = kind == PipelinePortType::Image ? "png" : kind == PipelinePortType::Text ? "txt" : kind == PipelinePortType::Video ? "mp4" : "mp3";
            String assetPath = GetString("assetPath", "Generated/output");
            WeakRef<FinishBody> weakThis(this);

            // The result comes first: a finish node shows what reaches it before anything runs
            if (kind == PipelinePortType::Image)
                AddCropSection("nothing connected - link a source to save it", "Result");
            else if (kind == PipelinePortType::Text)
                AddResultText("nothing connected - link a source to save it", 100);
            else if (kind == PipelinePortType::Audio)
                AddResultAudio("nothing connected - link a source to save it");
            else
                AddResultVideo("nothing connected - link a source to save it");

            auto folderRow = MakeRow("Folder", nullptr, 56);
            mFolderEdit = MakeEditBox(FolderOf(assetPath), false, "folder inside Assets");
            mFolderEdit->onChangeCompleted = [weakThis](const WString&) { if (auto self = weakThis.Lock()) self->OnPathEdited(); };
            folderRow->AddChild(mFolderEdit);
            auto browse = MakeButton("...");
            browse->name = "browse";
            browse->layout->minWidth = 28;
            browse->layout->maxWidth = 28;
            browse->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->ShowFolderMenu(); };
            folderRow->AddChild(browse);
            AddRow(folderRow, 22);

            auto nameRow = MakeRow("Name", nullptr, 56);
            mNameEdit = MakeEditBox(NameOf(assetPath), false, "file name");
            mNameEdit->onChangeCompleted = [weakThis](const WString&) { if (auto self = weakThis.Lock()) self->OnPathEdited(); };
            nameRow->AddChild(mNameEdit);
            auto extLabel = MakeLabel("." + mExtension, true);
            extLabel->horOverflow = Label::HorOverflow::None;
            extLabel->layout->minWidth = 34;
            extLabel->layout->maxWidth = 34;
            nameRow->AddChild(extLabel);
            AddRow(nameRow, 22);

            auto pathRow = mmake<HorizontalLayout>();
            pathRow->spacing = 4;
            pathRow->expandWidth = true;
            pathRow->expandHeight = true;
            pathRow->baseCorner = BaseCorner::Left;
            mPathLabel = MakeLabel("", false);
            mPathLabel->SetColor(dimTextColor);
            mPathLabel->horOverflow = Label::HorOverflow::Dots;
            pathRow->AddChild(mPathLabel);
            mShowButton = MakeIconButton("ui/pipeline/btn_locate.png", accentColor, Color4(0, 0, 0, 0));
            mShowButton->name = "show in assets";
            mShowButton->layout->minWidth = 22;
            mShowButton->layout->maxWidth = 22;
            mShowButton->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->ShowInAssets(); };
            pathRow->AddChild(mShowButton);
            AddRow(pathRow, 20);

            mStatusLabel = MakeLabel("", true);
            mStatusLabel->horOverflow = Label::HorOverflow::Dots;
            AddRow(mStatusLabel, 18);

            auto row = mmake<HorizontalLayout>();
            row->spacing = 6; row->expandWidth = true; row->expandHeight = true; row->baseCorner = BaseCorner::Left;
            auto save = MakeButton("Save to Assets");
            save->name = "save";
            save->onClick = [weakThis]()
            {
                if (auto self = weakThis.Lock())
                    if (auto editor = self->mEditor.Lock()) editor->RunNode(self->mNode->id, false);
            };
            row->AddChild(save);
            if (kind == PipelinePortType::Text)
            {
                auto copy = MakeButton("Copy");
                copy->layout->minWidth = 60;
                copy->onClick = [weakThis]()
                {
                    if (auto self = weakThis.Lock())
                    {
                        auto value = self->GetOutput();
                        if (value.IsText()) Clipboard::SetText(value.data);
                    }
                };
                row->AddChild(copy);
            }
            AddRow(row, 24);

            if (kind == PipelinePortType::Image)
            {
                auto resizeRow = mmake<HorizontalLayout>();
                resizeRow->spacing = 6; resizeRow->expandWidth = true; resizeRow->expandHeight = true; resizeRow->baseCorner = BaseCorner::Left;
                auto toggle = MakeCheckbox("Resize", GetBool("resize", false));
                toggle->layout->minWidth = 70; toggle->layout->maxWidth = 70;
                toggle->onToggleByUser = [weakThis](bool value) { if (auto self = weakThis.Lock()) { self->SetBool("resize", value, true); self->UpdateAssetInfo(); } };
                resizeRow->AddChild(toggle);
                auto w = MakeEditBox(GetString("resizeW", ""), false, "width");
                w->SetFilterInteger();
                w->onChangeCompleted = [weakThis](const WString& t) { if (auto self = weakThis.Lock()) { self->SetNumber("resizeW", (float)atoi(((String)t).Data()), true); self->UpdateAssetInfo(); } };
                resizeRow->AddChild(w);
                auto h = MakeEditBox(GetString("resizeH", ""), false, "height");
                h->SetFilterInteger();
                h->onChangeCompleted = [weakThis](const WString& t) { if (auto self = weakThis.Lock()) { self->SetNumber("resizeH", (float)atoi(((String)t).Data()), true); self->UpdateAssetInfo(); } };
                resizeRow->AddChild(h);
                AddRow(resizeRow, 22);
            }

            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            UpdateAssetInfo();
        }

        void OnConfigChanged() override
        {
            String assetPath = GetString("assetPath", "Generated/output");
            if (mFolderEdit) mFolderEdit->SetText(FolderOf(assetPath));
            if (mNameEdit) mNameEdit->SetText(NameOf(assetPath));
            UpdateAssetInfo();
        }

    private:
        String       mExtension;   // File extension of the result kind
        Ref<EditBox> mFolderEdit;  // Folder inside Assets
        Ref<EditBox> mNameEdit;    // File name without the extension
        Ref<Label>   mPathLabel;   // Full asset path the node writes to
        Ref<Button>  mShowButton;  // Reveals the saved asset in the assets window
        Ref<Label>   mStatusLabel; // Saved state and the size of the result

        static String FolderOf(const String& assetPath)
        {
            String path = assetPath;
            path.ReplaceAll("\\", "/");
            int slash = path.FindLast("/");
            return slash < 0 ? String() : path.SubStr(0, slash);
        }

        static String NameOf(const String& assetPath)
        {
            String path = assetPath;
            path.ReplaceAll("\\", "/");
            int slash = path.FindLast("/");
            String name = slash < 0 ? path : path.SubStr(slash + 1);
            int dot = name.FindLast(".");
            return dot > 0 ? name.SubStr(0, dot) : name;
        }

        static String Clean(const String& text)
        {
            String value = text.Trimed(" \n\r\t");
            value.ReplaceAll("\\", "/");
            while (value.StartsWith("/"))
                value = value.SubStr(1);
            while (value.EndsWith("/"))
                value = value.SubStr(0, value.Length() - 1);
            return value;
        }

        // Full path of the written file relative to the assets folder
        String GetFullAssetPath() const
        {
            String folder = Clean((String)mFolderEdit->GetText());
            String name = Clean((String)mNameEdit->GetText());
            if (name.IsEmpty())
                name = "output";
            return (folder.IsEmpty() ? name : folder + "/" + name) + "." + mExtension;
        }

        void OnPathEdited()
        {
            String folder = Clean((String)mFolderEdit->GetText());
            String name = Clean((String)mNameEdit->GetText());
            SetString("assetPath", folder.IsEmpty() ? name : folder + "/" + name, true);
            UpdateAssetInfo();
        }

        void UpdateAssetInfo()
        {
            if (!mPathLabel)
                return;

            String full = GetFullAssetPath();
            bool exists = o2FileSystem.IsFileExist(o2Assets.GetAssetsPath() + full);
            mPathLabel->text = "Assets/" + full;
            mShowButton->interactable = exists;
            mShowButton->transparency = exists ? 1.0f : 0.4f;

            String size;
            if (kind == PipelinePortType::Image)
            {
                if (mCropEditor && mCropEditor->HasImage())
                {
                    Vec2I imageSize = mCropEditor->GetImageSize();
                    size = (String)imageSize.x + " x " + (String)imageSize.y;
                    if (GetBool("resize", false) && GetNumber("resizeW", 0) > 0 && GetNumber("resizeH", 0) > 0)
                        size += " -> " + (String)(int)GetNumber("resizeW", 0) + " x " + (String)(int)GetNumber("resizeH", 0);
                }
            }
            else
            {
                auto value = GetOutput();
                if (!value.IsValid())
                    value = GetInput("in");

                if (value.IsText())
                    size = (String)value.data.Length() + " chars";
                else if (value.IsValid())
                    size = (String)(value.data.Length() / 1024) + " KB";
            }

            String state = exists ? "Saved" : "Not saved yet";
            mStatusLabel->text = size.IsEmpty() ? state : state + " - " + size;
        }

        void ShowFolderMenu()
        {
            WeakRef<FinishBody> weakThis(this);
            ShowAssetFolderMenu([weakThis](const String& folder)
            {
                if (auto self = weakThis.Lock())
                {
                    self->mFolderEdit->SetText(folder);
                    self->OnPathEdited();
                }
            });
        }

        void ShowInAssets()
        {
            if (!AssetsWindow::IsSingletonInitialzed())
                return;

            String full = GetFullAssetPath();
            if (!o2Assets.IsAssetExist(full))
                return;

            o2EditorAssets.SelectAsset(full);
            o2EditorAssets.ShowAssetIcon(full);
        }

    };

    class SourceTextBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddTextArea("text", "Inline text...", 0);
        }
    };

    class SourceImageBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            bool has = !GetString("uploadId").IsEmpty() || !GetString("assetPath").IsEmpty();
            AddResultImage("no image - choose png / jpg / bmp");
            String name = GetString("name");
            if (!GetString("assetPath").IsEmpty())
                AddMutedLine("asset: " + GetString("assetPath"));
            else if (!name.IsEmpty())
                AddMutedLine(name);
            AddFilePicker(has ? "Replace image..." : "Choose image...", { "png", "jpg", "jpeg", "bmp" }, true);
            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            // The source shows the chosen file even before the first run
            Ref<Bitmap> bitmap = GetOutputBitmap();
            if (!bitmap)
            {
                String path = !GetString("assetPath").IsEmpty() ? o2Assets.GetAssetsPath() + GetString("assetPath") : PipelineUtils::GetUploadPath(GetString("uploadId"));
                if (!path.IsEmpty() && o2FileSystem.IsFileExist(path))
                    bitmap = DecodeImageBytes(PipelineUtils::ReadFileBytes(path));
            }
            if (mImageView)
                mImageView->SetBitmap(bitmap);
        }
    };

    class SourceAudioBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            bool has = !GetString("uploadId").IsEmpty() || !GetString("assetPath").IsEmpty();
            AddResultAudio("no audio - choose mp3 / wav / ogg");
            String name = GetString("name");
            if (!GetString("assetPath").IsEmpty())
                AddMutedLine("asset: " + GetString("assetPath"));
            else if (!name.IsEmpty())
                AddMutedLine(name);
            AddFilePicker(has ? "Replace audio..." : "Choose audio...", { "mp3", "wav", "ogg", "flac" }, false);
            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineValue value = GetOutput();
            if (!value.IsAudio())
            {
                String path = !GetString("assetPath").IsEmpty() ? o2Assets.GetAssetsPath() + GetString("assetPath") : PipelineUtils::GetUploadPath(GetString("uploadId"));
                if (!path.IsEmpty() && o2FileSystem.IsFileExist(path))
                {
                    String ext = o2FileSystem.GetFileExtension(path);
                    value = PipelineValue::Bytes(PipelinePortType::Audio, PipelineUtils::ReadFileBytes(path), PipelineUtils::MimeForExtension(ext));
                }
            }
            if (mAudioView)
                mAudioView->SetAudio(value, GetString("name", "audio"));
        }
    };

    class TextComposeBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultText("connect a template - add variables on the inputs (+) and reference them as {name}");
            OnOutputChanged();
        }
    };

    class TextConcatBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultText("no result - connect the texts to join on the inputs (+)");
            if (BeginParams({ "Separate parts by an empty line" }))
                AddCheckbox("Separate parts by an empty line", "newlineSeparator", false);
            EndParams();
            OnOutputChanged();
        }
    };

    class AiTextBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultText("no result - press play to compute the branch");
            if (BeginParams({ "Model", "System prompt" }))
            {
                AddModelRow(textModelPresets, GeminiProvider::defaultTextModel);
                AddTextArea("systemPrompt", "System prompt (optional)", 46);
            }
            EndParams();
            OnOutputChanged();
        }
    };

    class TextEditBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultText("no result - connect the text and describe the edit");
            AddPrimaryField("instruction", "Describe the edit to apply - everything else stays unchanged");
            if (BeginParams({ "Model" }))
                AddModelRow(textModelPresets, GeminiProvider::defaultTextModel);
            EndParams();
            OnOutputChanged();
        }
    };

    class PromptGenBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            static const Vector<Pair<String, String>> targets = {
                { "image", "Image prompt" }, { "video", "Video prompt" }, { "text", "Text prompt" },
                { "sfx", "Sound effect prompt" }, { "music", "Music prompt" }, { "speech", "Spoken line" }
            };
            static const Map<String, String> hints = {
                { "image", "Detailed English prompt - subject, style, light, composition" },
                { "video", "Scene plus action and camera work over the clip" },
                { "text", "Self-contained instruction for a text model" },
                { "sfx", "Short and concrete (10-60 words) - ElevenLabs caps the prompt at 450 characters" },
                { "music", "Genre, instruments, BPM, key, mood - for Lyria" },
                { "speech", "The actual line of dialogue, in the character's language" }
            };

            AddResultText("no result - press play to compute the branch");

            if (BeginParams({ "Target", "Model", "Max chars", "System prompt" }))
            {
                String current = GetString("target", "image");
                Vector<String> labels;
                String currentLabel;
                for (auto& t : targets)
                {
                    labels.Add(t.second);
                    if (t.first == current) currentLabel = t.second;
                }
                auto dropdown = MakeDropDown(labels, currentLabel.IsEmpty() ? labels[0] : currentLabel);
                WeakRef<PipelineNodeBody> weakThis(this);
                dropdown->onSelectedText = [weakThis](const WString& text)
                {
                    auto self = weakThis.Lock();
                    if (!self) return;
                    for (auto& t : targets)
                    {
                        if (t.second == (String)text && self->GetString("target", "image") != t.first)
                        {
                            self->SetString("target", t.first, true);
                            self->RebuildBody();
                            return;
                        }
                    }
                };
                AddRow(MakeRow("Target", dropdown), 22);
                String hint;
                hints.TryGetValue(current, hint);
                AddMutedLine(hint, 30);

                AddModelRow(textModelPresets, GeminiProvider::defaultTextModel);

                int budget = current == "sfx" ? 450 : current == "music" ? 1200 : current == "speech" ? 300 : 0;
                if (budget > 0)
                {
                    auto edit = MakeEditBox(GetString("maxChars", ""), false, (String)budget);
                    edit->SetFilterInteger();
                    edit->onChangeCompleted = [weakThis](const WString& t) { if (auto self = weakThis.Lock()) self->SetString("maxChars", (String)t, true); };
                    AddRow(MakeRow("Max chars", edit), 22);
                }

                AddTextArea("systemPrompt", "System prompt (optional extra instructions)", 46);
            }
            EndParams();
            OnOutputChanged();
        }
    };

    class VideoGenBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultVideo("no result - press play to compute the branch");
            AddPrimaryField("extraPrompt", "Extra prompt (optional style hints)");
            if (BeginParams({ "Aspect", "Duration", "Model", "Solid background colour" }))
            {
                AddSelectRow("Aspect", "aspectRatio", { "16:9", "9:16", "1:1" }, "16:9");
                AddSelectRow("Duration", "duration", { "4", "5", "6", "8", "10" }, "8");
                AddMutedLine("Veo 3 renders 8s clips when reference images are used", 20);
                AddModelRow(videoModelPresets, VeoProvider::defaultModel);
                auto toggle = AddCheckbox("Solid background colour", "bgColorEnabled", false);
                WeakRef<PipelineNodeBody> weakThis(this);
                toggle->onToggleByUser = [weakThis](bool value)
                {
                    if (auto self = weakThis.Lock()) { self->SetBool("bgColorEnabled", value, true); self->RebuildBody(); }
                };
                if (GetBool("bgColorEnabled", false))
                    AddColor("Color", "bgColor", "#00b140");
            }
            EndParams();
            OnOutputChanged();
        }
    };

    class SfxGenBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultAudio("no result - press play to compute the branch");
            AddPrimaryField("extraPrompt", "Extra prompt (e.g. dry, close-mic, cartoon)");
            if (BeginParams({ "Seamless loop", "Length", "Model", "Influence" }))
            {
                AddCheckbox("Seamless loop", "loop", false);
                AddSelectRow("Length", "duration", { "auto", "0.5", "1", "2", "3", "5", "10", "20", "30" }, "auto");
                AddModelRow({ ElevenLabsProvider::sfxModel }, ElevenLabsProvider::sfxModel);
                AddTextRow("Influence", "promptInfluence", "0.3");
            }
            EndParams();
            OnOutputChanged();
        }
    };

    class TtsSpeechBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            String provider = GetString("provider", "gemini");
            AddResultAudio("no result - press play to compute the branch");
            if (provider != "elevenlabs")
                AddTextRow("Delivery", "styleInstructions", "Say cheerfully / whisper / shout");

            if (BeginParams({ "Provider", "Voice", "Model" }))
            {
                AddSegmented("provider", { { "gemini", "Gemini TTS" }, { "elevenlabs", "ElevenLabs" } }, "gemini");
                if (provider == "elevenlabs")
                {
                    AddTextRow("Voice id", "voice", "21m00Tcm4TlvDq8ikWAM");
                    AddModelRow({ "eleven_multilingual_v2", "eleven_turbo_v2_5", "eleven_flash_v2_5" }, ElevenLabsProvider::defaultTtsModel);
                }
                else
                {
                    AddSelectRow("Voice", "voice", GeminiProvider::voices, "Kore");
                    AddModelRow({ "gemini-2.5-flash-preview-tts", "gemini-2.5-pro-preview-tts" }, GeminiProvider::defaultTtsModel);
                }
            }
            EndParams();
            OnOutputChanged();
        }
    };

    class MusicGenBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultAudio("no result - press play to compute the branch");
            AddPrimaryField("extraPrompt", "Style, instruments, tempo, mood");
            if (BeginParams({ "Instrumental only", "Model" }))
            {
                AddCheckbox("Instrumental only (no vocals)", "instrumental", true);
                AddSegmented("model", { { "lyria-3-clip-preview", "Lyria 3 clip" }, { "lyria-3-pro-preview", "Lyria 3 pro" } }, GeminiProvider::defaultMusicModel);
            }
            EndParams();
            OnOutputChanged();
        }
    };

    // An instant node: its controls are its work, so they stay on view under the result
    class AudioProcessBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultAudio("no audio - connect the input");
            AddSegmented("format", { { "keep", "keep" }, { "wav", "wav" }, { "ogg", "ogg" }, { "mp3", "mp3" } }, "keep");
            AddSegmented("channels", { { "keep", "keep" }, { "mono", "mono" }, { "stereo", "stereo" } }, "keep");
            AddCheckbox("Trim silence", "trimSilence", false);
            auto normalize = AddCheckbox("Normalise loudness", "normalize", false);
            WeakRef<PipelineNodeBody> weakThis(this);
            normalize->onToggleByUser = [weakThis](bool value) { if (auto self = weakThis.Lock()) { self->SetBool("normalize", value, true); self->RebuildBody(); } };
            if (GetBool("normalize", false))
                AddSlider("LUFS", "loudnessTarget", -40, 0, 1, -14);
            auto loop = AddCheckbox("Seamless loop (crossfade)", "seamlessLoop", false);
            loop->onToggleByUser = [weakThis](bool value) { if (auto self = weakThis.Lock()) { self->SetBool("seamlessLoop", value, true); self->RebuildBody(); } };
            if (GetBool("seamlessLoop", false))
                AddSlider("Fade", "crossfadeMs", 20, 5000, 10, 250, " ms");
            AddSelectRow("Rate", "sampleRate", { "keep", "48000", "44100", "22050" }, "keep");
            OnOutputChanged();
        }
    };

    Ref<PipelineNodeBody> CreateBasicNodeBody(const String& type)
    {
        if (type == "finishImage" || type == "finishText" || type == "finishVideo" || type == "finishAudio")
        {
            auto finish = mmake<FinishBody>();
            finish->kind = type == "finishImage" ? PipelinePortType::Image : type == "finishText" ? PipelinePortType::Text
                : type == "finishVideo" ? PipelinePortType::Video : PipelinePortType::Audio;
            return finish;
        }

        if (type == "sourceText")
            return mmake<SourceTextBody>();

        if (type == "sourceImage")
            return mmake<SourceImageBody>();

        if (type == "sourceAudio")
            return mmake<SourceAudioBody>();

        if (type == "textCompose")
            return mmake<TextComposeBody>();

        if (type == "textConcat")
            return mmake<TextConcatBody>();

        if (type == "aiText")
            return mmake<AiTextBody>();

        if (type == "textEdit")
            return mmake<TextEditBody>();

        if (type == "promptGen")
            return mmake<PromptGenBody>();

        if (type == "videoGen")
            return mmake<VideoGenBody>();

        if (type == "sfxGen")
            return mmake<SfxGenBody>();

        if (type == "ttsSpeech")
            return mmake<TtsSpeechBody>();

        if (type == "musicGen")
            return mmake<MusicGenBody>();

        if (type == "audioProcess")
            return mmake<AudioProcessBody>();

        return nullptr;
    }
}
