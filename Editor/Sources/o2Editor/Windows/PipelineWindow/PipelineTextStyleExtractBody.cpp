#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2/Render/Render.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiDoc/UiTextStyle.h"
#include "o2Editor/Windows/PipelineWindow/PipelineCheckDlg.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

namespace Editor
{
    using namespace PipelineControls;
    using UiDoc::Json;

    // ---------------------------------------------------------------------------------------------
    // The chips of a style's look on one line: its fill (flat or the gradient's first and last colours),
    // then one per live effect in the list's order - an outline as a ring, the rest as dots of their colour
    // ---------------------------------------------------------------------------------------------
    class PipelineStyleChips : public Widget
    {
    public:
        explicit PipelineStyleChips(RefCounter* refCounter): Widget(refCounter) {}

        // Sets the style the chips show
        void SetStyle(const UiDoc::TextStyle& style)
        {
            mChips.Clear();
            Color4 first, last;
            PipelineUtils::ParseHexColor(style.fill && !style.fill->stops.IsEmpty() ? style.fill->stops[0].color : style.color, first);
            PipelineUtils::ParseHexColor(style.fill && !style.fill->stops.IsEmpty() ? style.fill->stops.Last().color : style.color, last);
            mChips.Add({ false, first, last });
            for (auto& effect : UiDoc::LiveEffects(style))
            {
                Color4 a, b;
                PipelineUtils::ParseHexColor(effect.type == "bevel" ? effect.highlight : effect.color, a);
                PipelineUtils::ParseHexColor(effect.type == "bevel" ? effect.shade : effect.color, b);
                mChips.Add({ effect.type == "stroke", a, b });
            }
        }

        // Returns how wide the chips are
        float GetChipsWidth() const { return mChips.Count()*(chip + gap); }

        void Draw() override
        {
            if (!mResEnabledInHierarchy || mIsClipped || PipelineControls::IsFarView())
                return;

            RectF rect = layout->GetWorldRect();
            float x = rect.left + chip*0.5f, y = (rect.top + rect.bottom)*0.5f;
            for (auto& c : mChips)
            {
                Vec2F center(x, y);
                if (c.ring)
                    PipelineDraw::Ring(center, chip*0.5f - 1.0f, 2.5f, c.top);
                else
                {
                    RectF box(center.x - chip*0.5f, center.y + chip*0.5f, center.x + chip*0.5f, center.y - chip*0.5f);
                    PipelineDraw::Fill(box, PipelineDraw::Corners(chip*0.5f), PipelineDraw::Gradient(c.top, c.bottom));
                }
                PipelineDraw::Ring(center, chip*0.5f, 1.0f, PipelineTheme::Get().line);
                x += chip + gap;
            }
        }

    private:
        static constexpr float chip = 11.0f, gap = 4.0f;

        struct Chip
        {
            bool   ring = false;
            Color4 top, bottom;
        };

        Vector<Chip> mChips;
    };

    // ---------------------------------------------------------------------------------------------------------------
    // The AI text style card (AssetsLine TextStyleExtractBody.tsx): the source picture with a frame round the lettering to
    // read (srcCrop, fractions; the whole picture without it); the style the model read off it drawn as a sample by the
    // layout renderer with the words of the lettering, on dark or light paper; a line with its name, font, size, weight and
    // chips of its look; the hint - which lettering to take - as the primary field; "-> Text style", which makes a Text
    // style node of the result beside the card; the folded parameters: model, name, role and the check against the picture
    // ---------------------------------------------------------------------------------------------------------------
    class TextStyleExtractBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;

        void Build() override
        {
            auto& theme = PipelineTheme::Get();
            WeakRef<TextStyleExtractBody> weakThis(this);
            ReadStyle();

            // The source with the frame round the lettering
            mSourceBox = mmake<Widget>();
            mSourceBox->name = "style source";
            mSource = mmake<PipelineCropEditor>();
            mSource->name = "style source crop";
            mSource->SetHint("connect a picture with the lettering");
            mSource->SetNode(mNode, "srcCrop");
            mSource->SetCropEnabled(true);
            mSource->onCropChanged = [weakThis](bool completed)
            {
                if (auto self = weakThis.Lock())
                {
                    self->Notify("srcCrop", completed);
                    self->UpdateWholeButton();
                }
            };
            *mSource->layout = WidgetLayout::BothStretch();
            mSourceBox->AddChild(mSource);
            mWhole = MakeIconButton("ui/pipeline/btn_maximize.svg", theme.fgDim, theme.field);
            mWhole->name = "whole picture";
            *mWhole->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-28, -28), Vec2F(-6, -6));
            mWhole->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->ReadWholePicture(); };
            mSourceBox->AddChild(mWhole);
            AddFlexible(mSourceBox, [](float width) { return Math::Max(180.0f, Math::Round(width*0.62f)); });
            MarkContent(mSourceBox);

            if (mHasStyle)
                AddResult();
            else
            {
                auto empty = MakeLabel("no style \xE2\x80\x94 connect a picture and \xE2\x96\xB6 to read its lettering", true);
                empty->name = "style empty";
                StyleLabel(empty, 11.0f, 400, theme.fg3);
                empty->horAlign = HorAlign::Middle;
                empty->verAlign = VerAlign::Middle;
                AddRow(empty, 64);
            }

            AddPrimaryField("hint", "Which lettering (optional), e.g. the title");

            if (mHasStyle)
            {
                auto toStyle = MakeGhostButton("\xE2\x86\x92 Text style");
                toStyle->name = "to text style";
                toStyle->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->MakeTextStyleNode(); };
                AddActions({ toStyle });
            }

            if (BeginParams(ParamsSummary()))
                AddParamRows();
            EndParams();

            UpdateWholeButton();
            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            auto input = GetInput("image");
            if (mSource)
            {
                auto picture = input.IsImage() ? Picture(input) : nullptr;
                mSource->SetBitmap(picture);
                mSource->SetLoading(!picture && PicturePending(input));
            }

            // A style arriving or going changes the rows; a new style only redraws the sample
            String before = mStyleJson;
            bool had = mHasStyle;
            ReadStyle();
            if (had != mHasStyle)
            {
                RebuildBodyKeepingArea();
                return;
            }
            if (before != mStyleJson)
                RedrawSample();
            OnRunStateChanged();
        }

        void OnConfigChanged() override
        {
            RebuildBodyKeepingArea();
        }

        void OnRunStateChanged() override
        {
            auto owner = mOwner.Lock();
            if (!owner || !mPhase)
                return;

            auto& rt = owner->GetRuntime();
            String phase;
            rt.phases.TryGetValue("", phase);
            mPhase->SetEnabledForcible(rt.state == "running" && phase == "check");
        }

        // Returns the sample's picture (tests)
        const Ref<PipelineImageView>& GetSampleView() const { return mSample; }

    private:
        Ref<Widget>             mSourceBox; // The source with the "whole picture" button over it
        Ref<PipelineCropEditor> mSource;    // The input picture with the frame round the lettering (srcCrop)
        Ref<Button>             mWhole;     // Drops the frame: the whole picture is read
        Ref<PipelineImageView>  mSample;    // The style drawn
        Ref<Label>              mPhase;     // "checking..." over the sample while the check runs

        bool             mHasStyle = false;
        String           mStyleJson;
        UiDoc::TextStyle mStyle;
        Json             mOwn; // What the model answered: {name, role, text}

        // Reads the style the node shows
        void ReadStyle()
        {
            auto output = GetOutput();
            Json raw;
            mHasStyle = output.IsTextLike() && Json::Parse(output.data, raw) && raw.IsObject() && UiDoc::JsTruthy(raw.Get("font"));
            mStyleJson = mHasStyle ? output.data : String();
            mStyle = mHasStyle ? UiDoc::NormalizeTextStyle(raw) : UiDoc::TextStyleDefault();
            mOwn = mHasStyle && raw.Get("extracted") ? *raw.Get("extracted") : Json();
        }

        bool Dark() const { return GetBool("tsDark", true); }

        // The words of the lettering itself, so the two compare at a glance
        String SampleText() const
        {
            String text = mOwn.IsObject() ? UiDoc::JsStringOr(mOwn.Get("text")) : String();
            if (!text.IsEmpty())
                return text;

            const String& role = mStyle.role;
            return role == "number" ? "1 250" : role == "button" ? "Play" : role == "title" ? "Forest Quest" :
                role == "caption" ? "Tap to continue" : "Hello, world";
        }

        void AddResult()
        {
            auto& theme = PipelineTheme::Get();
            WeakRef<TextStyleExtractBody> weakThis(this);

            auto box = mmake<Widget>();
            box->name = "style sample box";
            mSample = mmake<PipelineImageView>();
            mSample->name = "style sample";
            mSample->SetFramed(true);
            *mSample->layout = WidgetLayout::BothStretch();
            box->AddChild(mSample);

            auto paper = MakeGhostButton(Dark() ? "Light" : "Dark");
            paper->name = "paper";
            *paper->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-54, -26), Vec2F(-6, -6));
            paper->onClick = [weakThis]()
            {
                if (auto self = weakThis.Lock())
                {
                    self->SetBool("tsDark", !self->Dark(), true);
                    self->RebuildBodyKeepingArea();
                }
            };
            box->AddChild(paper);

            if (PipelineGraph::IsRefCheckOn(*mNode))
            {
                auto debug = MakeIconButton("ui/pipeline/al_bug.svg", theme.fgDim, theme.field);
                debug->name = "check details";
                *debug->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-80, -26), Vec2F(-60, -6));
                debug->onClick = [weakThis]()
                {
                    if (auto self = weakThis.Lock())
                        PipelineCheckDlg::Show(self->mEditor.Lock(), self->mNode, "", "");
                };
                box->AddChild(debug);
            }

            mPhase = MakeLabel("checking\xE2\x80\xA6", true);
            mPhase->name = "style phase";
            StyleLabel(mPhase, 10.5f, 500, theme.accent);
            *mPhase->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 0), Vec2F(8, 4), Vec2F(-8, 20));
            box->AddChild(mPhase);
            mPhase->SetEnabledForcible(false);

            AddFlexible(box, 112.0f);
            MarkContent(box);
            RedrawSample();

            // The style in words: name, font, size, weight, case, and the chips of its look
            auto line = mmake<HorizontalLayout>();
            line->name = "style summary";
            line->spacing = 6;
            line->expandWidth = false;
            line->expandHeight = true;
            line->baseCorner = BaseCorner::Left;
            auto name = MakeLabel(mStyle.name, false);
            StyleLabel(name, 11.5f, 600, theme.fg);
            name->layout->minWidth = name->layout->maxWidth = Math::Ceil(TextWidth(mStyle.name, 11.5f, 600)) + 2.0f;
            line->AddChild(name);
            String numbers = mStyle.font + " \xC2\xB7 " + UiDoc::JsNum(mStyle.size) + " px \xC2\xB7 " + UiDoc::JsNum(mStyle.weight) +
                (mStyle.uppercase ? " \xC2\xB7 AA" : "");
            auto info = MakeLabel(numbers, true);
            StyleLabel(info, 10.5f, 400, theme.fgDim);
            info->horOverflow = Label::HorOverflow::Dots;
            info->layout->minWidth = info->layout->maxWidth = Math::Ceil(TextWidth(numbers, 10.5f)) + 2.0f;
            line->AddChild(info);
            auto chips = mmake<PipelineStyleChips>();
            chips->name = "style chips";
            chips->SetStyle(mStyle);
            chips->layout->minWidth = chips->layout->maxWidth = chips->GetChipsWidth();
            line->AddChild(chips);
            AddRow(line, 20);
        }

        void RedrawSample()
        {
            if (!mSample)
                return;

            auto picture = UiDoc::RenderTextStyleSample(mStyle, SampleText(), Dark(), 1.0);
            mSample->SetBitmap(picture.IsEmpty() ? nullptr : picture.ToBitmap());
        }

        void UpdateWholeButton()
        {
            if (!mWhole)
                return;

            PipelineImageOps::CropRect crop;
            bool framed = PipelineImageOps::ParseCrop(mNode->GetConfigValue("srcCrop"), crop) &&
                (crop.x > 0.001f || crop.y > 0.001f || crop.w < 0.999f || crop.h < 0.999f);
            mWhole->SetEnabledForcible(framed);
        }

        void ReadWholePicture()
        {
            mNode->RemoveConfig("srcCrop");
            Notify("srcCrop", true);
            UpdateWholeButton();
        }

        // What the folded list says: the model, the name and the role set here, the check
        Vector<String> ParamsSummary() const
        {
            String model = GetString("model", "");
            if (model.IsEmpty())
                model = UiDoc::TEXT_STYLE_AI_DEFAULT_MODEL;
            Vector<String> names = { PipelineUtils::PrettyModelName(model) };
            if (!GetString("name", "").IsEmpty())
                names.Add(GetString("name", ""));
            if (UiDoc::TextRoles().Contains(GetString("role", "")))
                names.Add(GetString("role", ""));
            if (PipelineGraph::IsRefCheckOn(*mNode))
                names.Add("check");
            return names;
        }

        void AddParamRows()
        {
            WeakRef<TextStyleExtractBody> weakThis(this);
            AddModelRow(PipelineTextModelPresets(), UiDoc::TEXT_STYLE_AI_DEFAULT_MODEL, PipelineModelKind::Text);

            // The name the layout refers to (style=<name>); empty = the model's own. Applies without a new model call
            String ownName = mOwn.IsObject() ? UiDoc::JsStringOr(mOwn.Get("name")) : String();
            auto name = MakeEditBox(GetString("name", ""), false, ownName.IsEmpty() ? String("as the model names it") : ownName);
            name->name = "style name";
            name->onChangeCompleted = [weakThis](const WString& text)
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;

                String clean;
                for (auto c : (String)text)
                {
                    if (isalnum((unsigned char)c) || c == '_' || c == '-')
                        clean += c;
                }
                self->SetString("name", clean, true);
            };
            AddRow(MakeRow("Name", name, 64.0f), boxRowHeight);

            // What the style is for; auto = the model's own
            String ownRole = mOwn.IsObject() ? UiDoc::JsStringOr(mOwn.Get("role")) : String();
            String autoItem = ownRole.IsEmpty() ? String("auto") : "auto \xC2\xB7 " + ownRole;
            Vector<String> items = { autoItem };
            items.Add(UiDoc::TextRoles());
            String role = GetString("role", "");
            auto pick = MakeDropDown(items, UiDoc::TextRoles().Contains(role) ? role : autoItem);
            pick->name = "style role";
            pick->onSelectedText = [weakThis](const WString& text)
            {
                if (auto self = weakThis.Lock())
                {
                    String value = (String)text;
                    self->SetString("role", UiDoc::TextRoles().Contains(value) ? value : String(), true);
                    self->RebuildBodyKeepingArea();
                }
            };
            AddRow(MakeRow("Role", pick, 64.0f), selectRowHeight);

            // The check against the picture: off unless asked for (a vision call per attempt)
            PipelineCheckRows::Add(*this, "Check against the picture", "Corrections", [weakThis]()
            {
                if (auto self = weakThis.Lock())
                    self->RebuildBodyKeepingArea();
            });
        }

        // The style as a Text style node of its own beside this card, to keep editing it by hand: one undo step
        void MakeTextStyleNode()
        {
            auto editor = mEditor.Lock();
            if (!editor || !mHasStyle)
                return;

            Json look = UiDoc::TextStyleToJson(mStyle);
            look.Remove("extracted");
            if (!look.Has("description"))
                look["description"] = Json();
            String sample = mOwn.IsObject() ? UiDoc::JsStringOr(mOwn.Get("text")) : String();
            bool dark = Dark();
            editor->AddNodeBeside(mNode->id, "textStyle", [look, sample, dark](PipelineNode& node)
            {
                for (auto& member : look.Members())
                    UiDoc::SetConfigJson(node.config, member.first.Data(), member.second);
                node.SetConfigString("tsSample", sample);
                node.SetConfigBool("tsDark", dark);
            }, "Text style from the AI text style");
        }
    };

    Ref<PipelineNodeBody> CreateTextStyleExtractNodeBody(const String& type)
    {
        return type == "textStyleExtract" ? mmake<TextStyleExtractBody>() : nullptr;
    }
}
