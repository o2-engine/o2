#pragma once

#include "o2/Scene/Component.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Render/Text.h"

namespace o2
{
    // -----------------------------------------------------------------------------------
	// Text splitter component splits text into symbols and creates actors for each symbol
	// -----------------------------------------------------------------------------------
    class TextSplitterComponent: public Component
    {
    public:
        PROPERTIES(TextSplitterComponent);
		PROPERTY(WString, text, SetText, GetText);   // Text to split

		PROPERTY(AssetRef<FontAsset>, font, SetFont, GetFont); // Font reference

		PROPERTY(LinkRef<Actor>, symbolPrototype, SetSymbolPrototype, GetSymbolPrototype); // Symbol prototype actor

        PROPERTY(int, height, SetHeight, GetHeight); // Text height

		PROPERTY(float, symbolsDistanceCoef, SetSymbolsDistanceCoef, GetSymbolsDistanceCoef); // Characters distance coef
		PROPERTY(float, linesDistanceCoef, SetLinesDistanceCoef, GetLinesDistanceCoef);       // Lines distance coef

        PROPERTY(HorAlign, horAlign, SetHorAlign, GetHorAlign); // Horizontal align
        PROPERTY(VerAlign, verAlign, SetVerAlign, GetVerAlign); // Vertical align

        PROPERTY(bool, wordWrap, SetWordWrap, GetWordWrap);         // Words wrapping flag
        PROPERTY(bool, dotsEndings, SetDotsEndings, IsDotsEndings); // Dots endings when overflow

		PROPERTY(float, symbolsAnimationDelay, SetSymbolsAnimationDelay, GetSymbolsAnimationDelay);        // Symbols animation delay
		PROPERTY(bool, autoPlaySymbolsAnimation, SetAutoPlaySymbolsAnimation, IsAutoPlaySymbolsAnimation); // If symbols animation should be started automatically

    public:
		// Default constructor
        TextSplitterComponent();

		// Copy constructor
        TextSplitterComponent(const TextSplitterComponent& other);

		// Destructor
        ~TextSplitterComponent() override;

        // Sets text to split
        void SetText(const WString& text);

        // Returns current text
        const WString& GetText() const;

        // Sets font
        void SetFont(const AssetRef<FontAsset>& font);

        // Returns current font
        const AssetRef<FontAsset>& GetFont() const;

		// Sets symbol prototype actor
        void SetSymbolPrototype(const LinkRef<Actor>& prototype);

		// Returns symbol prototype actor
		const LinkRef<Actor>& GetSymbolPrototype() const;

        // Sets text height
        void SetHeight(int height);

        // Returns current text height
        int GetHeight() const;

        // Sets characters distance coefficient
        void SetSymbolsDistanceCoef(float coef);

        // Returns characters distance coefficient
        float GetSymbolsDistanceCoef() const;

        // Sets lines distance coefficient
        void SetLinesDistanceCoef(float coef);

        // Returns lines distance coefficient
        float GetLinesDistanceCoef() const;

        // Sets horizontal align
        void SetHorAlign(HorAlign align);

        // Returns horizontal align
        HorAlign GetHorAlign() const;

        // Sets vertical align
        void SetVerAlign(VerAlign align);

        // Returns vertical align
        VerAlign GetVerAlign() const;

        // Sets word wrapping
        void SetWordWrap(bool flag);

        // Returns word wrapping
        bool GetWordWrap() const;

        // Sets dots endings
        void SetDotsEndings(bool flag);

        // Returns dots endings
        bool IsDotsEndings() const;

		// Runs symbols animation, if symbols are animated
        void RunSymbolsAnimation();

		// Sets symbols animation delay
		void SetSymbolsAnimationDelay(float delay);

		// Returns symbols animation delay
		float GetSymbolsAnimationDelay() const;

		// Sets is symbols animation should be started automatically
		void SetAutoPlaySymbolsAnimation(bool autoPlay);

		// Returns is symbols animation should be started automatically
		bool IsAutoPlaySymbolsAnimation() const;

        SERIALIZABLE(TextSplitterComponent);
        CLONEABLE_REF(TextSplitterComponent);

    protected:
		AssetRef<FontAsset> mFont; // Font asset @SERIALIZABLE

		LinkRef<Actor> mSymbolPrototype; // Prototype actor for symbol @SERIALIZABLE

		WString mText; // Text to split @SERIALIZABLE

		int   mHeight = 11;            // Text height @SERIALIZABLE
		float mSymbolsDistCoef = 1.0f; // Symbols distance coefficient @SERIALIZABLE
		float mLinesDistCoef = 1.0f;   // Lines distance coefficient @SERIALIZABLE

		HorAlign mHorAlign = HorAlign::Left; // Horizontal align @SERIALIZABLE
		VerAlign mVerAlign = VerAlign::Top;  // Vertical align @SERIALIZABLE

		bool mWordWrap = false;    // Words wrapping flag @SERIALIZABLE
		bool mDotsEndings = false; // Dots endings flag @SERIALIZABLE

		bool  mAutoPlaySymbolsAnimation = false; // If symbols animation should be started automatically @SERIALIZABLE
		float mSymbolsAnimationDelay = 0.05f;    // Symbols animation delay, used to run symbols animation @SERIALIZABLE
		bool  mSymbolsAnimationStarted = false;  // If symbols animation was started
		int   mSymbolAnimationIndex = 0;         // Current symbol animation index
		float mSymbolsAnimationTime = 0.0f;      // Symbols animation time

		Vec2F mLastSize; // Last size of text, used to check if rebuild is needed

    protected:
		// Called on component start, splits text into symbols
		void OnStart() override;

		// Called on component destroying, clears symbols
		void OnDestroy() override;

		// Called on component enabling, splits text into symbols, subscribes to font resetting
		void OnEnabled() override;

		// Called on component disabling, unsubscribes from font resetting
		void OnDisabled() override;

		// Updates component, updates symbols animation
        void OnUpdate(float dt) override;

		// Called when actor's transform was changed
        void OnTransformUpdated() override;

		// Splits text into symbol actors
		void RebuildCharacters();

		// Clears all symbol actors
		void ClearSymbols();

		// Checks test's characters in font and rebuilds mesh. Used when fond is resetting
		void CheckCharactersAndRebuild();

		// Updates symbols animation, called every frame
		void UpdateSymbolsAnimation(float dt);
    };
}
// --- META ---

CLASS_BASES_META(o2::TextSplitterComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::TextSplitterComponent)
{
    FIELD().PUBLIC().NAME(text);
    FIELD().PUBLIC().NAME(font);
    FIELD().PUBLIC().NAME(symbolPrototype);
    FIELD().PUBLIC().NAME(height);
    FIELD().PUBLIC().NAME(symbolsDistanceCoef);
    FIELD().PUBLIC().NAME(linesDistanceCoef);
    FIELD().PUBLIC().NAME(horAlign);
    FIELD().PUBLIC().NAME(verAlign);
    FIELD().PUBLIC().NAME(wordWrap);
    FIELD().PUBLIC().NAME(dotsEndings);
    FIELD().PUBLIC().NAME(symbolsAnimationDelay);
    FIELD().PUBLIC().NAME(autoPlaySymbolsAnimation);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mFont);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSymbolPrototype);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mText);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(11).NAME(mHeight);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mSymbolsDistCoef);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mLinesDistCoef);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(HorAlign::Left).NAME(mHorAlign);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(VerAlign::Top).NAME(mVerAlign);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mWordWrap);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mDotsEndings);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mAutoPlaySymbolsAnimation);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.05f).NAME(mSymbolsAnimationDelay);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSymbolsAnimationStarted);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mSymbolAnimationIndex);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mSymbolsAnimationTime);
    FIELD().PROTECTED().NAME(mLastSize);
}
END_META;
CLASS_METHODS_META(o2::TextSplitterComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const TextSplitterComponent&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetText, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(const WString&, GetText);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFont, const AssetRef<FontAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<FontAsset>&, GetFont);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSymbolPrototype, const LinkRef<Actor>&);
    FUNCTION().PUBLIC().SIGNATURE(const LinkRef<Actor>&, GetSymbolPrototype);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHeight, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetHeight);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSymbolsDistanceCoef, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSymbolsDistanceCoef);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLinesDistanceCoef, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLinesDistanceCoef);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHorAlign, HorAlign);
    FUNCTION().PUBLIC().SIGNATURE(HorAlign, GetHorAlign);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVerAlign, VerAlign);
    FUNCTION().PUBLIC().SIGNATURE(VerAlign, GetVerAlign);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWordWrap, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetWordWrap);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDotsEndings, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsDotsEndings);
    FUNCTION().PUBLIC().SIGNATURE(void, RunSymbolsAnimation);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSymbolsAnimationDelay, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSymbolsAnimationDelay);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAutoPlaySymbolsAnimation, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsAutoPlaySymbolsAnimation);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStart);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDestroy);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildCharacters);
    FUNCTION().PROTECTED().SIGNATURE(void, ClearSymbols);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckCharactersAndRebuild);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateSymbolsAnimation, float);
}
END_META;
// --- END META ---
