#pragma once

#include "o2Editor/Properties/IPropertyField.h"
#include "o2Editor/Properties/PropertiesContext.h"
#include "o2Editor/UI/SegmentedIcons.h"

using namespace o2;

namespace o2
{
    class DropDown;
    class HorizontalLayout;
    class VerticalLayout;
}

namespace Editor
{
    FORWARD_CLASS_REF(AssetProperty);
    FORWARD_CLASS_REF(BooleanProperty);
    FORWARD_CLASS_REF(ColorProperty);
    FORWARD_CLASS_REF(EnumIconsProperty);
    FORWARD_CLASS_REF(FloatProperty);
    FORWARD_CLASS_REF(IntegerProperty);
    FORWARD_CLASS_REF(WStringProperty);

    // Letter case of a text
    enum class TextCase { Upper, Title, Sentence, Lower };

    // ------------------------------------------------------------------------------------------------------------
    // Names of the fields of a text's type the block edits. An empty name leaves the row out: a text drawable has
    // word wrap and dots endings, a label has overflows instead
    // ------------------------------------------------------------------------------------------------------------
    struct TextFieldNames
    {
        String text = "text";
        String font = "fontAsset";
        String fontStyle = "fontStyleAsset";
        String ownFontStyle;
        String height = "height";
        String color;
        String horAlign = "horAlign";
        String verAlign = "verAlign";
        String wordWrap;
        String dotsEndings;
        String horOverflow;
        String verOverflow;
        String expandBorder;
        String letterSpacing = "symbolsDistanceCoef";
        String lineSpacing = "linesDistanceCoef";
    };

    // ------------------------------------------------------------------------------------------------------------
    // The text settings of the Properties for a label and a text drawable: the text and its case in a dropdown, the
    // font, the style and the size as fields, the alignments with the wrapping or the overflows and the spacings as
    // groups of icons. Every change goes through the fields' undoable changes. The fields are registered in the
    // viewer's context, which refreshes them
    // ------------------------------------------------------------------------------------------------------------
    class TextPropertiesBlock: public RefCounterable
    {
    public:
        typedef IPropertyField::OnChangeCompletedFunc OnChangeCompletedFunc;
        typedef IPropertyField::OnChangedFunc         OnChangedFunc;

    public:
        Ref<WStringProperty>   text;
        Ref<AssetProperty>     font;
        Ref<AssetProperty>     fontStyle;
        Ref<IPropertyField>    ownFontStyle;
        Ref<IntegerProperty>   height;
        Ref<ColorProperty>     color;
        Ref<EnumIconsProperty> horAlign;
        Ref<EnumIconsProperty> verAlign;
        Ref<BooleanProperty>   wordWrap;
        Ref<BooleanProperty>   dotsEndings;
        Ref<EnumIconsProperty> horOverflow;
        Ref<EnumIconsProperty> verOverflow;
        Ref<IPropertyField>    expandBorder;
        Ref<FloatProperty>     letterSpacing;
        Ref<FloatProperty>     lineSpacing;

        Ref<DropDown>       caseDropDown; // Case of the text: shows the one it is in, puts the text in the chosen one
        Ref<SegmentedIcons> wrapFlags;    // Word wrap and dots endings of a text drawable, in the row of the alignments

    public:
        // Default constructor
        TextPropertiesBlock();

        // Destructor, where the types of the fields are known
        ~TextPropertiesBlock();

        // Builds the rows into the layout for the fields of the type
        void Build(const Ref<VerticalLayout>& layout, const Type& type, const TextFieldNames& names,
                   const Ref<PropertiesContext>& context, const OnChangeCompletedFunc& onChangeCompleted,
                   const OnChangedFunc& onChanged);

        // Updates what depends on the values: the case shown, the wrap icons, the expand border row
        void Refresh();

        // Puts the text of every target in the case
        void ApplyCase(TextCase textCase);

        // Returns the text in the case
        static WString ToCase(const WString& text, TextCase textCase);

        // Returns the case the text is in: false when it has no letters or mixes cases
        static bool FindCase(const WString& text, TextCase& textCase);

        // Returns the name of the case, as the dropdown shows it
        static WString GetCaseName(TextCase textCase);

    protected:
        const Type*            mType = nullptr;
        Ref<PropertiesContext> mContext;
        OnChangeCompletedFunc  mOnChangeCompleted;
        OnChangedFunc          mOnChanged;
        bool                   mUpdatingCase = false; // The dropdown is set by the text, not by the user

    protected:
        // Makes a row with the caption like the captions of the fields; returns the layout of its content
        Ref<HorizontalLayout> AddRow(const Ref<VerticalLayout>& layout, const WString& caption);

        // Builds a field with its caption into the layout
        template<typename _type>
        Ref<_type> AddCaptionedField(const Ref<VerticalLayout>& layout, const String& fieldName, const WString& caption);

        // Builds a field of the style without a caption into the parent
        template<typename _type>
        Ref<_type> AddBareField(const Ref<Widget>& parent, const String& fieldName, const String& style);

        // Builds the word wrap and dots endings icons into the row
        void AddWrapFlags(const Ref<HorizontalLayout>& row, const TextFieldNames& names);

        // Shows the case of the text in the dropdown
        void UpdateCaseView();

        // Turns the wrap icons on by the fields' values
        void UpdateWrapFlags();
    };
}
// --- META ---

PRE_ENUM_META(Editor::TextCase);
// --- END META ---
