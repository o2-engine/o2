#pragma once

#include "o2/Scene/UI/Widget.h"
#include "o2/Render/Text.h"

namespace o2
{
    // -----------------
    // Text label widget
    // -----------------
    class Label: public Widget
    {
    public:
        enum class HorOverflow { Cut, Dots, Expand, Wrap, None };
        enum class VerOverflow { Cut, None, Expand };

    public:
        PROPERTIES(Label);
        PROPERTY(WString, text, SetText, GetText);   // Text property, wstring

        PROPERTY(Ref<Font>, font, SetFont, GetFont);                     // Font pointer property
        PROPERTY(AssetRef<FontAsset>, fontAsset, SetFontAsset, GetFontAsset); // Font asset reference property

        PROPERTY(Ref<FontStyle>, fontStyle, SetFontStyle, GetFontStyle);                          // Font style pointer property
        PROPERTY(AssetRef<FontStyleAsset>, fontStyleAsset, SetFontStyleAsset, GetFontStyleAsset); // Font style asset reference property

        PROPERTY(int, height, SetHeight, GetHeight); // Text height property
        PROPERTY(Color4, color, SetColor, GetColor); // Text color property

        PROPERTY(VerAlign, verAlign, SetVerAlign, GetVerAlign); // vertical align property
        PROPERTY(HorAlign, horAlign, SetHorAlign, GetHorAlign); // Horizontal align property

        PROPERTY(HorOverflow, horOverflow, SetHorOverflow, GetHorOverflow); // Horizontal text overflow logic property
        PROPERTY(VerOverflow, verOverflow, SetVerOverflow, GetVerOverflow); // Vertical text overflow logic property

        PROPERTY(Vec2F, expandBorder, SetExpandBorder, GetExpandBorder); // Overflow expanding border size property

        PROPERTY(float, symbolsDistanceCoef, SetSymbolsDistanceCoef, GetSymbolsDistanceCoef); // Characters distance coef, 1 is standard
        PROPERTY(float, linesDistanceCoef, SetLinesDistanceCoef, GetLinesDistanceCoef);       // Lines distance coef, 1 is standard

    public:
        // Default constructor @SCRIPTABLE
        explicit Label(RefCounter* refCounter);

        // Copy-constructor
        Label(RefCounter* refCounter, const Label& other);

        // Assign operator
        Label& operator=(const Label& other);

        // Draws widget
        void Draw() override;

        // Sets using font
        void SetFont(const Ref<Font>& font);

        // Returns using font
        Ref<Font> GetFont() const;

        // Sets bitmap font asset @SCRIPTABLE
        void SetFontAsset(const AssetRef<FontAsset>& asset);

        // Returns asset by font asset id
        AssetRef<FontAsset> GetFontAsset() const;

        // Sets font style
        void SetFontStyle(const Ref<FontStyle>& style);

        // Returns font style
        Ref<FontStyle> GetFontStyle() const;

        // Sets font style asset
        void SetFontStyleAsset(const AssetRef<FontStyleAsset>& asset);

        // Returns font style asset
        AssetRef<FontStyleAsset> GetFontStyleAsset() const;

        // Sets text @SCRIPTABLE
        void SetText(const WString& text);

        // Returns text
        const WString& GetText() const;

        // Sets text color @SCRIPTABLE
        void SetColor(const Color4& color);

        // Returns color of text
        Color4 GetColor() const;

        // Sets horizontal align @SCRIPTABLE
        void SetHorAlign(HorAlign align);

        // Returns horizontal align
        HorAlign GetHorAlign() const;

        // Sets vertical align @SCRIPTABLE
        void SetVerAlign(VerAlign align);

        // returns vertical align
        VerAlign GetVerAlign() const;

        // Sets horizontal overflow logic
        void SetHorOverflow(HorOverflow overflow);

        // Returns horizontal overflow logic
        HorOverflow GetHorOverflow();

        // Sets vertical overflow logic
        void SetVerOverflow(VerOverflow overflow);

        // Returns vertical overflow logic
        VerOverflow GetVerOverflow();

        // Sets characters distance coefficient
        void SetSymbolsDistanceCoef(float coef = 1);

        // Returns characters distance coef
        float GetSymbolsDistanceCoef() const;

        // Sets lines distance coefficient
        void SetLinesDistanceCoef(float coef = 1);

        // Returns lines distance coefficient
        float GetLinesDistanceCoef() const;

        // Sets overflow expanding border
        void SetExpandBorder(const Vec2F& border);

        // Returns expanding overflow border
        Vec2F GetExpandBorder() const;

        // Sets text height @SCRIPTABLE
        void SetHeight(int height);

        // Returns text height
        int GetHeight() const;

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(Label);
        CLONEABLE_REF(Label);

    protected:
        Ref<Text>   mTextDrawable;                    // Text layer drawable. Getting from layer "text"
        HorOverflow mHorOverflow = HorOverflow::None; // Text horizontal overflow logic @SERIALIZABLE
        VerOverflow mVerOverflow = VerOverflow::None; // Text vertical overflow logic @SERIALIZABLE
        Vec2F       mExpandBorder;                    // Expand overflow border size @SERIALIZABLE

    protected:
        // Called when layer added and updates drawing sequence
        void OnLayerAdded(const Ref<WidgetLayer>& layer) override;

        // Creates default text layer
        void CreateDefaultText();

        // Called when deserialized
        void OnDeserialized(const DataValue& node) override;
    };
}
// --- META ---

PRE_ENUM_META(o2::Label::HorOverflow);

PRE_ENUM_META(o2::Label::VerOverflow);

CLASS_BASES_META(o2::Label)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(o2::Label)
{
    FIELD().PUBLIC().NAME(text);
    FIELD().PUBLIC().NAME(font);
    FIELD().PUBLIC().NAME(fontAsset);
    FIELD().PUBLIC().NAME(fontStyle);
    FIELD().PUBLIC().NAME(fontStyleAsset);
    FIELD().PUBLIC().NAME(height);
    FIELD().PUBLIC().NAME(color);
    FIELD().PUBLIC().NAME(verAlign);
    FIELD().PUBLIC().NAME(horAlign);
    FIELD().PUBLIC().NAME(horOverflow);
    FIELD().PUBLIC().NAME(verOverflow);
    FIELD().PUBLIC().NAME(expandBorder);
    FIELD().PUBLIC().NAME(symbolsDistanceCoef);
    FIELD().PUBLIC().NAME(linesDistanceCoef);
    FIELD().PROTECTED().NAME(mTextDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(HorOverflow::None).NAME(mHorOverflow);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(VerOverflow::None).NAME(mVerOverflow);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mExpandBorder);
}
END_META;
CLASS_METHODS_META(o2::Label)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Label&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFont, const Ref<Font>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Font>, GetFont);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetFontAsset, const AssetRef<FontAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(AssetRef<FontAsset>, GetFontAsset);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFontStyle, const Ref<FontStyle>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<FontStyle>, GetFontStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFontStyleAsset, const AssetRef<FontStyleAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(AssetRef<FontStyleAsset>, GetFontStyleAsset);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetText, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(const WString&, GetText);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(Color4, GetColor);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetHorAlign, HorAlign);
    FUNCTION().PUBLIC().SIGNATURE(HorAlign, GetHorAlign);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetVerAlign, VerAlign);
    FUNCTION().PUBLIC().SIGNATURE(VerAlign, GetVerAlign);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHorOverflow, HorOverflow);
    FUNCTION().PUBLIC().SIGNATURE(HorOverflow, GetHorOverflow);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVerOverflow, VerOverflow);
    FUNCTION().PUBLIC().SIGNATURE(VerOverflow, GetVerOverflow);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSymbolsDistanceCoef, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSymbolsDistanceCoef);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLinesDistanceCoef, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLinesDistanceCoef);
    FUNCTION().PUBLIC().SIGNATURE(void, SetExpandBorder, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetExpandBorder);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetHeight, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetHeight);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLayerAdded, const Ref<WidgetLayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateDefaultText);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;
// --- END META ---
