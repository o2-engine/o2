#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/Types/BitmapFontAsset.h"
#include "o2/Assets/Types/VectorFontAsset.h"
#include "o2/Render/Font.h"
#include "o2/Render/RectDrawable.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    class Mesh;
    class Render;

    // ------------------------------------------------------------------------------------------
    // Text renderer class. Using font, basis and many style parameters. Caching text into meshes
    // ------------------------------------------------------------------------------------------
    class Text: public IRectDrawable
    {
    public:
        struct SymbolsSet;

    public:
        PROPERTIES(Text);
        PROPERTY(Ref<Font>, font, SetFont, GetFont);                     // Font reference property @SCRIPTABLE
        PROPERTY(AssetRef<FontAsset>, fontAsset, SetFontAsset, GetFontAsset); // Font asset reference property @SCRIPTABLE
        
        PROPERTY(WString, text, SetText, GetText); // Text property, wstring @SCRIPTABLE
        
        PROPERTY(int, height, SetHeight, GetFontHeight); // Text height @SCRIPTABLE
        
        PROPERTY(VerAlign, verAlign, SetVerAlign, GetVerAlign); // vertical align property @SCRIPTABLE
        PROPERTY(HorAlign, horAlign, SetHorAlign, GetHorAlign); // Horizontal align property @SCRIPTABLE
        
        PROPERTY(bool, wordWrap, SetWordWrap, GetWordWrap);         // Words wrapping flag property @SCRIPTABLE
        PROPERTY(bool, dotsEngings, SetDotsEngings, IsDotsEngings); // Dots endings when overflow property @SCRIPTABLE
        
        PROPERTY(float, symbolsDistanceCoef, SetSymbolsDistanceCoef, GetSymbolsDistanceCoef); // Characters distance coef, 1 is standard @SCRIPTABLE
        PROPERTY(float, linesDistanceCoef, SetLinesDistanceCoef, GetLinesDistanceCoef);       // Lines distance coef, 1 is standard @SCRIPTABLE

    public:
        // Default constructor @SCRIPTABLE
        Text();

        // Constructor
        Text(Ref<Font> font);

        // Constructor
        Text(const String& fontFileName);

        // Constructor
        Text(const UID& fontAssetId);

        // Constructor
        Text(const AssetRef<BitmapFontAsset>& fontAsset);

        // Constructor
        Text(const AssetRef<VectorFontAsset>& fontAsset);

        // Copy-constructor
        Text(const Text& text);

        // Destructor
        ~Text();

        // Assign operator
        Text& operator=(const Text& other);

        // Draw text @SCRIPTABLE
        void Draw() override;

        // Sets using font @SCRIPTABLE
        void SetFont(const Ref<Font>& font);

        // Returns using font @SCRIPTABLE
        const Ref<Font>& GetFont() const;

        // Sets bitmap font asset  @SCRIPTABLE
        void SetFontAsset(const AssetRef<FontAsset>& asset);

        // Returns asset by font asset id @SCRIPTABLE
        AssetRef<FontAsset> GetFontAsset() const;

        // Sets font height
        void SetHeight(int height);

        // Returns font height
        int GetFontHeight() const;

        // Sets text
        void SetText(const WString& text);

        // Returns text
        const WString& GetText() const;

        // Sets horizontal align
        void SetHorAlign(HorAlign align);

        // Returns horizontal align
        HorAlign GetHorAlign() const;

        // Sets vertical align
        void SetVerAlign(VerAlign align);

        // returns vertical align
        VerAlign GetVerAlign() const;

        // Sets word wrapping
        void SetWordWrap(bool flag);

        // Returns word wrapping
        bool GetWordWrap() const;

        // Sets dots endings
        void SetDotsEngings(bool flag);

        // Returns dots endings
        bool IsDotsEngings() const;

        // Sets characters distance coefficient
        void SetSymbolsDistanceCoef(float coef = 1);

        // Returns characters distance coef
        float GetSymbolsDistanceCoef() const;

        // Sets lines distance coefficient
        void SetLinesDistanceCoef(float coef = 1);

        // Returns lines distance coefficient
        float GetLinesDistanceCoef() const;

        // Returns symbol set structure pointer
        SymbolsSet& GetSymbolsSet();

        // Returns real text size @SCRIPTABLE
        Vec2F GetRealSize();

        // Returns real text rectangle @SCRIPTABLE
        RectF GetRealRect();

        // Returns text size
        static Vec2F GetTextSize(const WString& text, const Ref<Font>& font, int height = 11,
                                 const Vec2F& areaSize = Vec2F(),
                                 HorAlign horAlign = HorAlign::Left, VerAlign verAlign = VerAlign::Top,
                                 bool wordWrap = true, bool dotsEngings = false, float charsDistCoef = 1.0f,
                                 float linesDistCoef = 1.0f);

        SERIALIZABLE(Text);
        CLONEABLE_REF(Text);

    public:
        // ----------------
        // Text symbols set
        // ----------------
        struct SymbolsSet
        {
            // ----------------------------------
            // Simple symbol definition structure
            // ----------------------------------
            struct Symbol
            {
                RectF  mFrame;   // Frame of symbol layout
                RectF  mTexSrc;  // Texture source rect
                UInt16 mCharId;  // Character id
                Vec2F  mOrigin;  // Character offset
                float  mAdvance; // Character advance

            public:
                // Default constructor
                Symbol();

                // Constructor
                Symbol(const Vec2F& position, const Vec2F& size, const RectF& texSrc, UInt16 charId,
                       const Vec2F& origin, float advance);

             // Equals operator
                bool operator==(const Symbol& other) const;
            };

            // -------------------------
            // Line definition structure
            // -------------------------
            struct Line
            {
                Vector<Symbol> mSymbols;       // Symbols in line
                WString       mString;        // Line string
                Vec2F         mSize;          // Size of line in pixels
                Vec2F         mPosition;      // Position of line
                int           mLineBegSymbol; // Index of line beginning symbol
                int           mSpacesCount;   // Spaces count at line
                bool          mEndedNewLine;  // True, if line ended by new line character

            public:
                // Default constructor
                Line();

                // Equals operator
                bool operator==(const Line& other) const;
            };

        public:
            Ref<Font>  mFont;            // Font
            int        mHeight;          // Text height
            WString    mText;            // Text string
            Vec2F      mPosition;        // Position, in pixels
            Vec2F      mAreaSize;        // Area size, in pixels
            Vec2F      mRealSize;        // Real text size
            HorAlign   mHorAlign;        // Horizontal align
            VerAlign   mVerAlign;        // Vertical align
            bool       mWordWrap;        // True, when words wrapping
            bool       mDotsEndings;     // Dots ending when overflow
            float      mSymbolsDistCoef; // Characters distance coefficient, 1 is standard
            float      mLinesDistCoef;   // Lines distance coefficient, 1 is standard

            Vector<Line> mLines; // Lines definitions

        public:
            // Calculating characters layout by parameters
            void Initialize(const Ref<Font>& font, const WString& text, int height, const Vec2F& position, const Vec2F& areaSize,
                            HorAlign horAlign, VerAlign verAlign, bool wordWrap, bool dotsEngings, float charsDistCoef,
                            float linesDistCoef);

            // Moves symbols 
            void Move(const Vec2F& offs);
        };

    protected:
        const UInt mMeshMaxPolyCount = 4096;

        WString   mText;              // Wide char string, containing rendering text @SERIALIZABLE
        UID       mFontAssetId;       // Font asset id @SERIALIZABLE
        Ref<Font> mFont;              // Using font
        int       mHeight;            // Text height @SERIALIZABLE
        float     mSymbolsDistCoef;   // Characters distance coef, 1 is standard @SERIALIZABLE
        float     mLinesDistanceCoef; // Lines distance coef, 1 is standard @SERIALIZABLE
        VerAlign  mVerAlign;          // Vertical align @SERIALIZABLE
        HorAlign  mHorAlign;          // Horizontal align @SERIALIZABLE
        bool      mWordWrap;          // True, when words wrapping @SERIALIZABLE
        bool      mDotsEndings;       // If true, text will end on '...' @SERIALIZABLE

        Vector<Ref<Mesh>> mMeshes;        // Meshes vector
        Basis             mLastTransform; // Last mesh update transformation

        SymbolsSet mSymbolsSet; // Symbols set definition

        bool mUpdatingMesh; // True, when mesh is already updating

    protected:

        // Updating meshes
        void UpdateMesh();

        // Checks test's characters in font and rebuilds mesh. Used when fond is resetting
        void CheckCharactersAndRebuildMesh();

        // Transforming meshes by basis
        void TransformMesh(const Basis& bas);

        // Preparing meshes for characters count
        void PrepareMesh(int charactersCount);

        // Calculates and returns text basis
        Basis CalculateTextBasis() const;

        // Called when color was changed
        void OnColorChanged() override;

        // Called when basis was changed
        void BasisChanged() override;

        // Calling when deserializing
        void OnDeserialized(const DataValue& node) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::Text)
{
    BASE_CLASS(o2::IRectDrawable);
}
END_META;
CLASS_FIELDS_META(o2::Text)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(font);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(fontAsset);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(text);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(height);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(verAlign);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(horAlign);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(wordWrap);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(dotsEngings);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(symbolsDistanceCoef);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(linesDistanceCoef);
    FIELD().PROTECTED().DEFAULT_VALUE(4096).NAME(mMeshMaxPolyCount);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mText);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mFontAssetId);
    FIELD().PROTECTED().NAME(mFont);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHeight);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSymbolsDistCoef);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mLinesDistanceCoef);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mVerAlign);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHorAlign);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mWordWrap);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mDotsEndings);
    FIELD().PROTECTED().NAME(mMeshes);
    FIELD().PROTECTED().NAME(mLastTransform);
    FIELD().PROTECTED().NAME(mSymbolsSet);
    FIELD().PROTECTED().NAME(mUpdatingMesh);
}
END_META;
CLASS_METHODS_META(o2::Text)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(Ref<Font>);
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const UID&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const AssetRef<BitmapFontAsset>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const AssetRef<VectorFontAsset>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Text&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetFont, const Ref<Font>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const Ref<Font>&, GetFont);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetFontAsset, const AssetRef<FontAsset>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(AssetRef<FontAsset>, GetFontAsset);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHeight, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetFontHeight);
    FUNCTION().PUBLIC().SIGNATURE(void, SetText, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(const WString&, GetText);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHorAlign, HorAlign);
    FUNCTION().PUBLIC().SIGNATURE(HorAlign, GetHorAlign);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVerAlign, VerAlign);
    FUNCTION().PUBLIC().SIGNATURE(VerAlign, GetVerAlign);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWordWrap, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetWordWrap);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDotsEngings, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsDotsEngings);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSymbolsDistanceCoef, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSymbolsDistanceCoef);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLinesDistanceCoef, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLinesDistanceCoef);
    FUNCTION().PUBLIC().SIGNATURE(SymbolsSet&, GetSymbolsSet);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetRealSize);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(RectF, GetRealRect);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vec2F, GetTextSize, const WString&, const Ref<Font>&, int, const Vec2F&, HorAlign, VerAlign, bool, bool, float, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckCharactersAndRebuildMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, TransformMesh, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(void, PrepareMesh, int);
    FUNCTION().PROTECTED().SIGNATURE(Basis, CalculateTextBasis);
    FUNCTION().PROTECTED().SIGNATURE(void, OnColorChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, BasisChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;
// --- END META ---
