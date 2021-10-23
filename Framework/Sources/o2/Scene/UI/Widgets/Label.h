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

		PROPERTY(FontRef, font, SetFont, GetFont);                     // Font pointer property
		PROPERTY(FontAssetRef, fontAsset, SetFontAsset, GetFontAsset); // Font asset reference property
		
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
		// Default constructor
		Label();

		// Copy-constructor
		Label(const Label& other);

		// Assign operator
		Label& operator=(const Label& other);

		// Draws widget
		void Draw() override;

		// Sets using font
		void SetFont(FontRef font);

		// Returns using font
		FontRef GetFont() const;

		// Sets bitmap font asset 
		void SetFontAsset(const FontAssetRef& asset);

		// Returns asset by font asset id
		FontAssetRef GetFontAsset() const;

		// Sets text
		void SetText(const WString& text);

		// Returns text
		const WString& GetText() const;

		// Sets text color
		void SetColor(const Color4& color);

		// Returns color of text
		Color4 GetColor() const;

		// Sets horizontal align
		void SetHorAlign(HorAlign align);

		// Returns horizontal align
		HorAlign GetHorAlign() const;

		// Sets vertical align
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

		// Sets text height
		void SetHeight(int height);

		// Returns text height
		int GetHeight() const;

		// Updates layout
		void UpdateSelfTransform() override;

		// Returns create menu group in editor
		static String GetCreateMenuGroup();

		SERIALIZABLE(Label);

	protected:
		Text*       mTextDrawable = nullptr;             // Text layer drawable. Getting from layer "text"
		HorOverflow mHorOverflow = HorOverflow::None; // Text horizontal overflow logic @SERIALIZABLE
		VerOverflow mVerOverflow = VerOverflow::None; // Text vertical overflow logic @SERIALIZABLE
		Vec2F       mExpandBorder;                    // Expand overflow border size @SERIALIZABLE

	protected:
		// It is called when layer added and updates drawing sequence
		void OnLayerAdded(WidgetLayer* layer) override;

		// Creates default text layer
		void CreateDefaultText();

		// It is called when deserialized
		void OnDeserialized(const DataValue& node) override;
	};
}

PRE_ENUM_META(o2::Label::HorOverflow);

PRE_ENUM_META(o2::Label::VerOverflow);

CLASS_BASES_META(o2::Label)
{
	BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(o2::Label)
{
	FIELD().NAME(text).PUBLIC();
	FIELD().NAME(font).PUBLIC();
	FIELD().NAME(fontAsset).PUBLIC();
	FIELD().NAME(height).PUBLIC();
	FIELD().NAME(color).PUBLIC();
	FIELD().NAME(verAlign).PUBLIC();
	FIELD().NAME(horAlign).PUBLIC();
	FIELD().NAME(horOverflow).PUBLIC();
	FIELD().NAME(verOverflow).PUBLIC();
	FIELD().NAME(expandBorder).PUBLIC();
	FIELD().NAME(symbolsDistanceCoef).PUBLIC();
	FIELD().NAME(linesDistanceCoef).PUBLIC();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mTextDrawable).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(HorOverflow::None).NAME(mHorOverflow).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(VerOverflow::None).NAME(mVerOverflow).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().NAME(mExpandBorder).PROTECTED();
}
END_META;
CLASS_METHODS_META(o2::Label)
{

	PUBLIC_FUNCTION(void, Draw);
	PUBLIC_FUNCTION(void, SetFont, FontRef);
	PUBLIC_FUNCTION(FontRef, GetFont);
	PUBLIC_FUNCTION(void, SetFontAsset, const FontAssetRef&);
	PUBLIC_FUNCTION(FontAssetRef, GetFontAsset);
	PUBLIC_FUNCTION(void, SetText, const WString&);
	PUBLIC_FUNCTION(const WString&, GetText);
	PUBLIC_FUNCTION(void, SetColor, const Color4&);
	PUBLIC_FUNCTION(Color4, GetColor);
	PUBLIC_FUNCTION(void, SetHorAlign, HorAlign);
	PUBLIC_FUNCTION(HorAlign, GetHorAlign);
	PUBLIC_FUNCTION(void, SetVerAlign, VerAlign);
	PUBLIC_FUNCTION(VerAlign, GetVerAlign);
	PUBLIC_FUNCTION(void, SetHorOverflow, HorOverflow);
	PUBLIC_FUNCTION(HorOverflow, GetHorOverflow);
	PUBLIC_FUNCTION(void, SetVerOverflow, VerOverflow);
	PUBLIC_FUNCTION(VerOverflow, GetVerOverflow);
	PUBLIC_FUNCTION(void, SetSymbolsDistanceCoef, float);
	PUBLIC_FUNCTION(float, GetSymbolsDistanceCoef);
	PUBLIC_FUNCTION(void, SetLinesDistanceCoef, float);
	PUBLIC_FUNCTION(float, GetLinesDistanceCoef);
	PUBLIC_FUNCTION(void, SetExpandBorder, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetExpandBorder);
	PUBLIC_FUNCTION(void, SetHeight, int);
	PUBLIC_FUNCTION(int, GetHeight);
	PUBLIC_FUNCTION(void, UpdateSelfTransform);
	PUBLIC_STATIC_FUNCTION(String, GetCreateMenuGroup);
	PROTECTED_FUNCTION(void, OnLayerAdded, WidgetLayer*);
	PROTECTED_FUNCTION(void, CreateDefaultText);
	PROTECTED_FUNCTION(void, OnDeserialized, const DataValue&);
}
END_META;
