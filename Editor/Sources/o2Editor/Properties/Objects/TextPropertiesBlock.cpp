#include "o2Editor/stdafx.h"
#include "TextPropertiesBlock.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Properties/Basic/AssetProperty.h"
#include "o2Editor/Properties/Basic/BooleanProperty.h"
#include "o2Editor/Properties/Basic/ColorProperty.h"
#include "o2Editor/Properties/Basic/EnumIconsProperty.h"
#include "o2Editor/Properties/Basic/FloatProperty.h"
#include "o2Editor/Properties/Basic/IntegerProperty.h"
#include "o2Editor/Properties/Basic/WStringProperty.h"
#include "o2Editor/Properties/Properties.h"

namespace Editor
{
    namespace
    {
        const TextCase allCases[] = { TextCase::Upper, TextCase::Title, TextCase::Sentence, TextCase::Lower };

        wchar_t UpperOf(wchar_t c)
        {
            if (c >= L'a' && c <= L'z')
                return c - 32;

            if ((c >= 0xE0 && c <= 0xFE && c != 0xF7) || (c >= 0x430 && c <= 0x44F))
                return c - 0x20;

            if (c >= 0x450 && c <= 0x45F)
                return c - 0x50;

            return c;
        }

        wchar_t LowerOf(wchar_t c)
        {
            if (c >= L'A' && c <= L'Z')
                return c + 32;

            if ((c >= 0xC0 && c <= 0xDE && c != 0xD7) || (c >= 0x410 && c <= 0x42F))
                return c + 0x20;

            if (c >= 0x400 && c <= 0x40F)
                return c + 0x50;

            return c;
        }

        bool IsLetter(wchar_t c)
        {
            return UpperOf(c) != LowerOf(c);
        }

        Vector<EnumIconOption> HorAlignOptions()
        {
            return {
                { (int)HorAlign::Left, "ui/UI4_text_align_left_icon.svg" },
                { (int)HorAlign::Middle, "ui/UI4_text_align_center_icon.svg" },
                { (int)HorAlign::Right, "ui/UI4_text_align_right_icon.svg" },
                { (int)HorAlign::Both, "ui/UI4_text_align_justify_icon.svg" }
            };
        }

        Vector<EnumIconOption> VerAlignOptions()
        {
            return {
                { (int)VerAlign::Top, "ui/UI4_text_valign_top_icon.svg" },
                { (int)VerAlign::Middle, "ui/UI4_text_valign_middle_icon.svg" },
                { (int)VerAlign::Bottom, "ui/UI4_text_valign_bottom_icon.svg" },
                { (int)VerAlign::Both, "ui/UI4_text_valign_stretch_icon.svg" }
            };
        }

        Vector<EnumIconOption> HorOverflowOptions()
        {
            return {
                { (int)Label::HorOverflow::None, "ui/UI4_text_overflow_none_icon.svg" },
                { (int)Label::HorOverflow::Cut, "ui/UI4_text_overflow_cut_icon.svg" },
                { (int)Label::HorOverflow::Dots, "ui/UI4_text_dots_icon.svg" },
                { (int)Label::HorOverflow::Expand, "ui/UI4_text_overflow_expand_icon.svg" },
                { (int)Label::HorOverflow::Wrap, "ui/UI4_text_wrap_icon.svg" }
            };
        }

        Vector<EnumIconOption> VerOverflowOptions()
        {
            return {
                { (int)Label::VerOverflow::None, "ui/UI4_text_voverflow_none_icon.svg" },
                { (int)Label::VerOverflow::Cut, "ui/UI4_text_voverflow_cut_icon.svg" },
                { (int)Label::VerOverflow::Expand, "ui/UI4_text_voverflow_expand_icon.svg" }
            };
        }
    }

    TextPropertiesBlock::TextPropertiesBlock()
    {}

    TextPropertiesBlock::~TextPropertiesBlock()
    {}

    void TextPropertiesBlock::Build(const Ref<VerticalLayout>& layout, const Type& type, const TextFieldNames& names,
                                    const Ref<PropertiesContext>& context, const OnChangeCompletedFunc& onChangeCompleted,
                                    const OnChangedFunc& onChanged)
    {
        PushEditorScopeOnStack scope;

        mType = &type;
        mContext = context;
        mOnChangeCompleted = onChangeCompleted;
        mOnChanged = onChanged;

        // The text and its case
        if (!names.text.IsEmpty())
        {
            text = AddCaptionedField<WStringProperty>(layout, names.text, "Text");

            auto caseRow = AddRow(layout, "Case");
            caseDropDown = o2UI.CreateDropdown();
            caseDropDown->name = "case";
            for (auto textCase : allCases)
                caseDropDown->AddItem(GetCaseName(textCase));

            caseDropDown->onSelectedText = [this](const WString& name)
            {
                if (mUpdatingCase)
                    return;

                for (auto textCase : allCases)
                {
                    if (GetCaseName(textCase) == name)
                        ApplyCase(textCase);
                }
            };

            caseRow->AddChild(caseDropDown);

            if (text)
                text->onChanged += [this](const Ref<IPropertyField>&, bool) { UpdateCaseView(); };
        }

        if (!names.font.IsEmpty())
            font = AddCaptionedField<AssetProperty>(layout, names.font, "Font");

        if (!names.fontStyle.IsEmpty())
            fontStyle = AddCaptionedField<AssetProperty>(layout, names.fontStyle, "Style");

        if (!names.ownFontStyle.IsEmpty())
            ownFontStyle = AddCaptionedField<IPropertyField>(layout, names.ownFontStyle, "Own style");

        if (!names.height.IsEmpty())
            height = AddCaptionedField<IntegerProperty>(layout, names.height, "Size");

        if (!names.color.IsEmpty())
            color = AddCaptionedField<ColorProperty>(layout, names.color, "Color");

        // Alignments, and the wrapping of a text drawable after them
        if (!names.horAlign.IsEmpty() || !names.verAlign.IsEmpty())
        {
            auto row = AddRow(layout, "Align");
            row->spacing = 6;

            if (!names.horAlign.IsEmpty())
            {
                horAlign = AddBareField<EnumIconsProperty>(row, names.horAlign, "standard");
                horAlign->SetOptions(HorAlignOptions());
            }

            if (!names.verAlign.IsEmpty())
            {
                verAlign = AddBareField<EnumIconsProperty>(row, names.verAlign, "standard");
                verAlign->SetOptions(VerAlignOptions());
            }

            AddWrapFlags(row, names);

            auto spacer = mmake<Widget>();
            spacer->name = "spacer";
            row->AddChild(spacer);
        }

        // Overflows of a label, the border of expanding when one expands
        if (!names.horOverflow.IsEmpty() || !names.verOverflow.IsEmpty())
        {
            auto row = AddRow(layout, "Overflow");
            row->spacing = 6;

            if (!names.horOverflow.IsEmpty())
            {
                horOverflow = AddBareField<EnumIconsProperty>(row, names.horOverflow, "standard");
                horOverflow->SetOptions(HorOverflowOptions());
            }

            if (!names.verOverflow.IsEmpty())
            {
                verOverflow = AddBareField<EnumIconsProperty>(row, names.verOverflow, "standard");
                verOverflow->SetOptions(VerOverflowOptions());
            }

            auto spacer = mmake<Widget>();
            spacer->name = "spacer";
            row->AddChild(spacer);

            if (!names.expandBorder.IsEmpty())
                expandBorder = AddCaptionedField<IPropertyField>(layout, names.expandBorder, "Expand border");

            auto refresh = [this](const Ref<IPropertyField>&, bool) { Refresh(); };
            if (horOverflow)
                horOverflow->onChanged += refresh;

            if (verOverflow)
                verOverflow->onChanged += refresh;
        }

        // Spacings: an icon before each field
        if (!names.letterSpacing.IsEmpty() || !names.lineSpacing.IsEmpty())
        {
            auto row = AddRow(layout, "Spacing");
            row->spacing = 4;

            auto addIcon = [&](const String& iconName)
            {
                auto icon = mmake<Widget>();
                icon->name = iconName;
                icon->AddLayer("icon", mmake<VectorSprite>(iconName), Layout::Based(BaseCorner::Center, Vec2F(16, 16)));
                icon->layout->minWidth = 16;
                icon->layout->maxWidth = 16;
                row->AddChild(icon);
            };

            if (!names.letterSpacing.IsEmpty())
            {
                addIcon("ui/UI4_text_letter_spacing_icon.svg");
                letterSpacing = AddBareField<FloatProperty>(row, names.letterSpacing, "standard");
            }

            if (!names.lineSpacing.IsEmpty())
            {
                addIcon("ui/UI4_text_line_spacing_icon.svg");
                lineSpacing = AddBareField<FloatProperty>(row, names.lineSpacing, "standard");
            }
        }

        Refresh();
    }

    void TextPropertiesBlock::AddWrapFlags(const Ref<HorizontalLayout>& row, const TextFieldNames& names)
    {
        Vector<String> icons;
        Vector<Ref<BooleanProperty>> flags;

        auto addFlag = [&](const String& fieldName, const String& icon) -> Ref<BooleanProperty>
        {
            if (fieldName.IsEmpty())
                return nullptr;

            // The field stays hidden in the context, the icon shows and sets it
            auto field = AddBareField<BooleanProperty>(row, fieldName, "standard");
            field->SetEnabledForcible(false);
            field->onChanged += [this](const Ref<IPropertyField>&, bool) { UpdateWrapFlags(); };

            icons.Add(icon);
            flags.Add(field);
            return field;
        };

        wordWrap = addFlag(names.wordWrap, "ui/UI4_text_wrap_icon.svg");
        dotsEndings = addFlag(names.dotsEndings, "ui/UI4_text_dots_icon.svg");

        if (flags.IsEmpty())
            return;

        wrapFlags = mmake<SegmentedIcons>();
        wrapFlags->name = "wrap";
        wrapFlags->SetMode(SegmentedIcons::Mode::Flags);
        wrapFlags->SetSegments(icons);
        wrapFlags->onClicked = [this, flags](int index)
        {
            if (index >= 0 && index < flags.Count())
                flags[index]->SetValueByUserAndComplete(!flags[index]->GetCommonValue() || flags[index]->IsValuesDifferent());

            UpdateWrapFlags();
        };

        row->AddChild(wrapFlags);
    }

    void TextPropertiesBlock::Refresh()
    {
        UpdateCaseView();
        UpdateWrapFlags();

        if (!expandBorder)
            return;

        auto expands = [](const Ref<EnumIconsProperty>& field, int expandValue)
        {
            return field && (field->IsValuesDifferent() || field->GetCommonValue() == expandValue);
        };

        bool expand = expands(horOverflow, (int)Label::HorOverflow::Expand) ||
            expands(verOverflow, (int)Label::VerOverflow::Expand);

        if (expandBorder->IsEnabled() != expand)
            expandBorder->SetEnabled(expand);
    }

    void TextPropertiesBlock::UpdateCaseView()
    {
        if (!caseDropDown || !text)
            return;

        // The case every selected text is in, none when they are in different ones
        WString name;
        for (auto& value : text->GetValues())
        {
            TextCase textCase;
            WString valueName = FindCase(value, textCase) ? GetCaseName(textCase) : WString();
            if (valueName.IsEmpty() || (!name.IsEmpty() && valueName != name))
            {
                name = WString();
                break;
            }

            name = valueName;
        }

        if (caseDropDown->GetSelectedItemText() == name)
            return;

        mUpdatingCase = true;
        caseDropDown->SelectItemText(name);
        mUpdatingCase = false;
    }

    void TextPropertiesBlock::UpdateWrapFlags()
    {
        if (!wrapFlags)
            return;

        int index = 0;
        for (auto& field : { wordWrap, dotsEndings })
        {
            if (!field)
                continue;

            wrapFlags->SetOn(index++, !field->IsValuesDifferent() && field->GetCommonValue());
        }
    }

    Ref<HorizontalLayout> TextPropertiesBlock::AddRow(const Ref<VerticalLayout>& layout, const WString& caption)
    {
        auto row = mmake<HorizontalLayout>();
        row->name = caption + " row";
        row->layout->minHeight = 20;
        row->expandHeight = true;
        row->expandWidth = true;
        row->fitByChildren = false;
        row->borderLeft = 10;

        auto label = o2UI.CreateWidget<Label>();
        label->name = "propertyName";
        label->text = caption;
        label->horAlign = HorAlign::Left;
        label->layout->widthWeight = 0.6f;
        label->horOverflow = Label::HorOverflow::Dots;
        row->AddChild(label);

        // A plain container as the fields have: no minimal width, so the columns line up with theirs
        auto container = mmake<Widget>();
        container->name = "container";
        row->AddChild(container);

        auto content = mmake<HorizontalLayout>();
        content->name = "content";
        *content->layout = WidgetLayout::BothStretch();
        content->expandHeight = true;
        content->expandWidth = true;
        content->fitByChildren = false;
        container->AddChild(content);

        layout->AddChild(row);
        return content;
    }

    template<typename _type>
    Ref<_type> TextPropertiesBlock::AddCaptionedField(const Ref<VerticalLayout>& layout, const String& fieldName,
                                                      const WString& caption)
    {
        auto info = mType->GetField(fieldName);
        if (!info)
            return nullptr;

        auto field = o2EditorProperties.BuildField(layout, info, mContext, "", mOnChangeCompleted, mOnChanged);
        if (!field)
            return nullptr;

        field->SetCaption(caption);
        return DynamicCast<_type>(field);
    }

    template<typename _type>
    Ref<_type> TextPropertiesBlock::AddBareField(const Ref<Widget>& parent, const String& fieldName, const String& style)
    {
        auto info = mType->GetField(fieldName);
        if (!info)
            return nullptr;

        auto field = o2UI.CreateWidget<_type>(style);
        field->name = info->GetName() + " : " + info->GetType()->GetName();
        field->onChanged = mOnChanged;
        field->onChangeCompleted = mOnChangeCompleted;
        field->SetParentContext(mContext);
        field->SetValuePath(info->GetName());
        field->SetFieldInfo(info);

        if constexpr (std::is_base_of<EnumProperty, _type>::value)
            field->SpecializeType(info->GetType());

        parent->AddChild(field);
        mContext->properties.Add(info, field);
        return field;
    }

    WString TextPropertiesBlock::ToCase(const WString& source, TextCase textCase)
    {
        WString res = source;
        bool wordStart = true;
        bool sentenceStart = true;

        for (int i = 0; i < res.Length(); i++)
        {
            wchar_t c = res[i];
            bool letter = IsLetter(c);

            switch (textCase)
            {
                case TextCase::Upper: res[i] = UpperOf(c); break;
                case TextCase::Lower: res[i] = LowerOf(c); break;
                case TextCase::Title: res[i] = wordStart && letter ? UpperOf(c) : LowerOf(c); break;
                case TextCase::Sentence: res[i] = sentenceStart && letter ? UpperOf(c) : LowerOf(c); break;
            }

            if (letter || (c >= L'0' && c <= L'9') || c == L'\'')
            {
                wordStart = false;
                sentenceStart = false;
            }
            else
            {
                wordStart = true;
                if (c == L'.' || c == L'!' || c == L'?' || c == L'\n')
                    sentenceStart = true;
            }
        }

        return res;
    }

    bool TextPropertiesBlock::FindCase(const WString& source, TextCase& textCase)
    {
        bool hasLetters = false;
        for (int i = 0; i < source.Length() && !hasLetters; i++)
            hasLetters = IsLetter(source[i]);

        if (!hasLetters)
            return false;

        for (auto candidate : allCases)
        {
            if (ToCase(source, candidate) == source)
            {
                textCase = candidate;
                return true;
            }
        }

        return false;
    }

    WString TextPropertiesBlock::GetCaseName(TextCase textCase)
    {
        switch (textCase)
        {
            case TextCase::Upper: return "UPPER CASE";
            case TextCase::Title: return "Title Case";
            case TextCase::Sentence: return "Sentence case";
            case TextCase::Lower: return "lower case";
        }

        return WString();
    }

    void TextPropertiesBlock::ApplyCase(TextCase textCase)
    {
        if (text)
            text->TransformByUser([textCase](const WString& x) { return ToCase(x, textCase); });

        UpdateCaseView();
    }
}
// --- META ---

ENUM_META(Editor::TextCase, Editor__TextCase)
{
    ENUM_ENTRY(Lower);
    ENUM_ENTRY(Sentence);
    ENUM_ENTRY(Title);
    ENUM_ENTRY(Upper);
}
END_ENUM_META;
// --- END META ---
