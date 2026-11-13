#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiDoc/UiTextStyle.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTextStyleFx.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineUiAi.h"

namespace Editor
{
    using namespace PipelineControls;

    // ---------------------------------------------------------------------------------------------------------------
    // The Text style card (AssetsLine TextStyleBody.tsx): the sample drawn by the layout renderer (what the AI UI layout
    // draws), its text and the paper; a request for the AI (pictures added as inputs are its references) and its model;
    // then the style's fields - the fill (a colour, or a gradient with its stops on a bar) and the layer effects among them
    // (PipelineTextStyleFx.h). The node's config IS the style. The sample's height follows the card's width (its own shape,
    // 680 x 224, across the body); a card sized by hand gives the sample the rest of its height
    // ---------------------------------------------------------------------------------------------------------------
    class TextStyleBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;

        void Build() override
        {
            auto& theme = PipelineTheme::Get();
            WeakRef<TextStyleBody> weakThis(this);
            auto style = Style();

            mPreview = mmake<PipelineImageView>();
            mPreview->name = "style sample";
            mPreview->SetFramed(true);
            AddFlexible(mPreview, [](float width) { return SampleHeight(width); });
            MarkContent(mPreview);

            // The sample's text and the paper it is drawn on
            auto sampleRow = mmake<HorizontalLayout>();
            sampleRow->spacing = 6;
            sampleRow->expandWidth = true;
            sampleRow->expandHeight = true;
            sampleRow->baseCorner = BaseCorner::Left;
            mSample = MakeEditBox(GetString("tsSample", ""), false, DefaultSample(style));
            mSample->name = "sample";
            mSample->onChanged = [weakThis](const WString& text)
            {
                if (auto self = weakThis.Lock())
                {
                    self->mNode->SetConfigString("tsSample", (String)text);
                    self->Redraw();
                }
            };
            mSample->onChangeCompleted = [weakThis](const WString& text)
            {
                if (auto self = weakThis.Lock())
                    self->SetString("tsSample", (String)text, true);
            };
            sampleRow->AddChild(mSample);
            auto paper = MakeGhostButton(Dark() ? "Light" : "Dark");
            paper->name = "paper";
            paper->layout->minWidth = paper->layout->maxWidth = 56;
            paper->onClick = [weakThis]()
            {
                if (auto self = weakThis.Lock())
                {
                    self->SetBool("tsDark", !self->Dark(), true);
                    self->RebuildBodyKeepingArea();
                }
            };
            sampleRow->AddChild(paper);
            AddRow(sampleRow, 24);

            // The request and the AI's model; pictures wired in are its references
            auto field = mmake<Widget>();
            field->name = "style request";
            mPrompt = MakeEditBox(GetString("stylePrompt", ""), true, "Describe the lettering, e.g. \"chunky golden title with a dark outline\"");
            mPrompt->name = "style prompt";
            *mPrompt->layout = WidgetLayout::BothStretch();
            mPrompt->onChangeCompleted = [weakThis](const WString& text)
            {
                if (auto self = weakThis.Lock())
                    self->SetString("stylePrompt", (String)text, true);
            };
            field->AddChild(mPrompt);
            mGenerate = MakeIconButton("ui/pipeline/uiws_sparkle.svg", theme.accent, Color4(0, 0, 0, 0));
            mGenerate->name = "generate style";
            *mGenerate->layout = WidgetLayout(Vec2F(1, 0), Vec2F(1, 0), Vec2F(-30, 6), Vec2F(-6, 30));
            mGenerate->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->Generate(); };
            field->AddChild(mGenerate);
            AddRow(field, 62);

            AddModelRow(PipelineTextModelPresets(), "gpt-6.1-sol", PipelineModelKind::Text, "styleModel", "AI model", 70);
            int refs = mNode->inputs.Sum<int>([](const PipelinePort& p) { return p.portType == PipelinePortType::Image ? 1 : 0; });
            mNote = MakeLabel(refs ? (String)refs + " reference picture(s) go along" : "Add picture inputs to match their lettering", true);
            StyleLabel(mNote, 10.5f, 400, theme.fg3);
            AddRow(mNote, 16);

            AddFields(style);
            Redraw();
        }

        void OnConfigChanged() override
        {
            RebuildBodyKeepingArea();
        }

        void Update(float dt) override
        {
            PipelineNodeBody::Update(dt);
            bool busy = PipelineUiAi::IsBusy("style", mNode->id);
            if (mGenerate)
            {
                mGenerate->interactable = !busy;
                mGenerate->transparency = busy ? 0.4f : 1.0f;
            }
        }

        // The sample's height in a body row of the width: the sample's own shape across it, never under 112
        static float SampleHeight(float rowWidth)
        {
            return Math::Max(112.0f, Math::Round(rowWidth*224.0f/680.0f));
        }

        // Returns the fill or the effects block (tests)
        Ref<PipelineTextFxPanel> GetFxPanel(bool effects) const { return effects ? mEffects : mFill; }

    private:
        Ref<PipelineTextFxPanel> mFill;    // The fill: a colour or a gradient and its stops
        Ref<PipelineTextFxPanel> mEffects; // The layer effects
        Ref<PipelineImageView> mPreview;  // The sample drawn with the style
        Ref<EditBox>           mSample;   // The sample's text
        Ref<EditBox>           mPrompt;   // The request to the AI
        Ref<Button>            mGenerate; // Sends it
        Ref<Label>             mNote;     // The references going along

        UiDoc::TextStyle Style() const { return UiDoc::TextStyleFromConfig(mNode->config); }

        bool Dark() const { return GetBool("tsDark", true); }

        static String DefaultSample(const UiDoc::TextStyle& st)
        {
            return st.role == "number" ? "1 250" : st.role == "button" ? "Play" : st.role == "title" ? "Forest Quest" :
                st.role == "caption" ? "Tap to continue" : "Hello, world";
        }

        void Redraw()
        {
            auto style = Style();
            String text = GetString("tsSample", "");
            if (text.IsEmpty())
                text = DefaultSample(style);

            auto picture = UiDoc::RenderTextStyleSample(style, text, Dark(), 1.0);
            mPreview->SetBitmap(picture.IsEmpty() ? nullptr : picture.ToBitmap());
        }

        void Generate()
        {
            if (mPrompt)
                SetString("stylePrompt", (String)mPrompt->GetText(), true);

            PipelineUiAi::GenerateStyle(mEditor.Lock(), mNode->id);
        }

        // Writes a style field: the config is the style
        void SetField(const String& key, const UiDoc::Json& value, bool completed = true)
        {
            UiDoc::SetConfigJson(mNode->config, key, value);
            Notify(key, completed);
            Redraw();
        }

        // The fill or the effects changed: they go to the config together with their mirrors (gradient, stroke, shadow),
        // which the engine exports and the older editors read
        void SetLook(const UiDoc::TextStyle& style, bool completed)
        {
            PipelineTextFxPanel::WriteLook(mNode->config, style);
            Notify("effects", completed);
            Redraw();
        }

        // A block of the drawn rows: the fill or the effects
        Ref<PipelineTextFxPanel> AddFx(bool effects)
        {
            WeakRef<TextStyleBody> weakThis(this);
            auto panel = mmake<PipelineTextFxPanel>();
            panel->name = effects ? "style effects" : "style fill";
            panel->effects = effects;
            panel->editor = mEditor;
            panel->getStyle = [weakThis]() { auto self = weakThis.Lock(); return self ? self->Style() : UiDoc::TextStyleDefault(); };
            panel->onLook = [weakThis](const UiDoc::TextStyle& style, bool done) { if (auto self = weakThis.Lock()) self->SetLook(style, done); };
            panel->onRows = [weakThis]() { if (auto self = weakThis.Lock()) self->RebuildBodyKeepingArea(); };
            AddRow(panel, [weakThis, effects](float)
            {
                auto self = weakThis.Lock();
                return PipelineTextFxPanel::Height(effects, self ? self->Style() : UiDoc::TextStyleDefault());
            });
            return panel;
        }

        // A labelled row of controls
        Ref<HorizontalLayout> Row(const String& label)
        {
            auto line = mmake<HorizontalLayout>();
            line->spacing = 6;
            line->expandWidth = true;
            line->expandHeight = true;
            line->baseCorner = BaseCorner::Left;
            AddRow(MakeRow(label, line, 70), boxRowHeight);
            return line;
        }

        // A labelled slider row
        Ref<PipelineSlider> Slider(const String& label, float minValue, float maxValue, float step, float value, const String& suffix,
                                   const Function<void(float, bool)>& onChanged)
        {
            auto slider = mmake<PipelineSlider>();
            slider->name = "style " + label;
            slider->Setup(label, minValue, maxValue, step, value, suffix);
            slider->onChanged = onChanged;
            AddRow(slider, 15.5f);
            return slider;
        }

        void Segments(const Ref<HorizontalLayout>& line, const Vector<Pair<String, String>>& options, const String& current,
                      const Function<void(const String&)>& onPick)
        {
            auto block = mmake<Widget>();
            auto row = mmake<HorizontalLayout>();
            row->spacing = 2;
            row->expandWidth = true;
            row->expandHeight = true;
            *row->layout = WidgetLayout::BothStretch(3, 3, 3, 3);
            SkinSegmentedRow(*row);
            for (auto& option : options)
            {
                String value = option.first;
                auto segment = MakeSegment(option.second, value == current);
                segment->name = "segment " + value;
                segment->onToggleByUser = [onPick, value](bool) { onPick(value); };
                row->AddChild(segment);
            }
            block->AddChild(row);
            line->AddChild(block);
        }

        void AddFields(const UiDoc::TextStyle& st)
        {
            WeakRef<TextStyleBody> weakThis(this);
            auto set = [weakThis](const String& key, const UiDoc::Json& value, bool completed = true)
            {
                if (auto self = weakThis.Lock())
                    self->SetField(key, value, completed);
            };
            auto rebuild = [weakThis]() { if (auto self = weakThis.Lock()) self->RebuildBodyKeepingArea(); };

            // Name and role
            auto nameLine = Row("Name");
            auto name = MakeEditBox(GetString("name", st.name), false, "title");
            name->name = "style name";
            name->onChangeCompleted = [set](const WString& text)
            {
                String clean;
                for (auto c : (String)text)
                {
                    if (isalnum((unsigned char)c) || c == '_' || c == '-')
                        clean += c;
                }
                set("name", UiDoc::Json(clean));
            };
            nameLine->AddChild(name);
            auto role = MakeDropDown(UiDoc::TextRoles(), st.role);
            role->name = "style role";
            role->onSelectedText = [set, rebuild](const WString& text) { set("role", UiDoc::Json((String)text)); rebuild(); };
            nameLine->AddChild(role);

            // Font and weight
            auto fontLine = Row("Font");
            Vector<String> families;
            for (auto& font : UiDoc::UiFonts())
                families.Add(font.family);
            auto& font = UiDoc::FontOf(st.font);
            auto fontPick = MakeDropDown(families, font.family);
            fontPick->name = "style font";
            fontPick->onSelectedText = [set, rebuild, st](const WString& text)
            {
                auto& picked = UiDoc::FontOf((String)text);
                int weight = picked.faces.IsEmpty() ? 400 : picked.faces[0].weight;
                for (auto& face : picked.faces)
                {
                    if (Math::Abs(face.weight - st.weight) < Math::Abs(weight - st.weight))
                        weight = face.weight;
                }
                set("font", UiDoc::Json(picked.family), false);
                set("weight", UiDoc::Json(weight));
                rebuild();
            };
            fontLine->AddChild(fontPick);
            if (font.faces.Count() > 1)
            {
                Vector<Pair<String, String>> weights;
                for (auto& face : font.faces)
                    weights.Add({ (String)face.weight, (String)face.weight });
                Segments(fontLine, weights, (String)(int)st.weight, [set, rebuild](const String& v) { set("weight", UiDoc::Json(atof(v.Data()))); rebuild(); });
            }

            Slider("Size", 8, 240, 1, (float)st.size, "px", [set](float v, bool done) { set("size", UiDoc::Json((double)v), done); });

            // The fill: a colour, or a gradient with its stops on a bar
            mFill = AddFx(false);

            Slider("Spacing", -10, 40, 0.5f, (float)st.letterSpacing, "px", [set](float v, bool done) { set("letterSpacing", UiDoc::Json((double)v), done); });
            Slider("Line", 0.8f, 2.0f, 0.05f, (float)st.lineHeight, "x", [set](float v, bool done) { set("lineHeight", UiDoc::Json((double)v), done); });

            auto caseLine = Row("Case");
            Segments(caseLine, { { "as", "Aa" }, { "up", "AA" } }, st.uppercase ? "up" : "as",
                     [set, rebuild](const String& v) { set("uppercase", UiDoc::Json(v == "up")); rebuild(); });
            Segments(caseLine, { { "left", "Left" }, { "center", "Mid" }, { "right", "Right" } }, st.align,
                     [set, rebuild](const String& v) { set("align", UiDoc::Json(v)); rebuild(); });

            // The layer effects: the head with "add an effect", a row or two per effect
            mEffects = AddFx(true);
        }
    };

    Ref<PipelineNodeBody> CreateTextStyleNodeBody(const String& type)
    {
        return type == "textStyle" ? mmake<TextStyleBody>() : nullptr;
    }
}
