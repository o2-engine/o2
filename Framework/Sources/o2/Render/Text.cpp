#include "o2/stdafx.h"
#include "Text.h"

#include <cmath>

#include "o2/Application/Input.h"
#include "o2/Assets/Assets.h"
#include "o2/Integration.h"
#include "o2/Render/FontStyle.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Render/VectorGraphics/VectorMesh.h"

namespace o2
{
    namespace
    {
        // Part of the scale the fringe of the glyph meshes is kept within
        const float glyphsFringeScaleTolerance = 0.01f;

        // Vertices of a glyph mesh at the fringe scale, for the text geometry being built
        Vector<Vec2F> scaledGlyphPositions;
        Vector<Color32Bit> scaledGlyphColors;

        // Triangles of the glyphs of a text
        struct GlyphsIndexes: public RefCounterable
        {
            Vector<VertexIndex> indexes;
        };
    }

    bool Text::mGlyphMeshesEnabled = false;

    Text::Text():
        mFont(nullptr), mSymbolsDistCoef(1), mLinesDistanceCoef(1), mVerAlign(VerAlign::Top),
        mHorAlign(HorAlign::Left), mWordWrap(false), IRectDrawable(), mUpdatingMesh(false), mFontAssetId(0),
        mDotsEndings(false), mHeight(11)
    {}

    Text::Text(Ref<Font> font):
        mFont(font), mSymbolsDistCoef(1), mLinesDistanceCoef(1), mVerAlign(VerAlign::Top),
        mHorAlign(HorAlign::Left), mWordWrap(false), IRectDrawable(), mUpdatingMesh(false),
        mFontAssetId(0), mDotsEndings(false), mHeight(11)
    {
        if (mFont)
            mFont->onCharactersRebuilt += ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);
    }

    Text::Text(const Text& text):
        IRectDrawable(text), mUpdatingMesh(false)
    {
        mText = text.mText;
        mFont = text.mFont;
        mFontStyle = text.mFontStyle;
        mFontStyleAsset = text.mFontStyleAsset;
        mSymbolsDistCoef = text.mSymbolsDistCoef;
        mLinesDistanceCoef = text.mLinesDistanceCoef;
        mVerAlign = text.mVerAlign;
        mHorAlign = text.mHorAlign;
        mWordWrap = text.mWordWrap;
        mFontAssetId = text.mFontAssetId;
        mDotsEndings = text.mDotsEndings;
        mHeight = text.mHeight;

        if (mFont)
            mFont->onCharactersRebuilt += ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        if (mFontStyle)
            mFontStyle->onChanged += ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);
    }

    Text::Text(const AssetRef<BitmapFontAsset>& fontAsset):
        Text(fontAsset->GetFont())
    {
        mFontAssetId = fontAsset->GetUID();
    }

    Text::Text(const AssetRef<VectorFontAsset>& fontAsset):
        Text(fontAsset->GetFont())
    {
        mFontAssetId = fontAsset->GetUID();
    }

    Text::Text(const String& fontFileName):
        Text(o2Assets.GetAssetId(fontFileName))
    {}

    Text::Text(const UID& fontAssetId) :
        mSymbolsDistCoef(1), mLinesDistanceCoef(1), mVerAlign(VerAlign::Top),
        mHorAlign(HorAlign::Left), mWordWrap(false), IRectDrawable(), mDotsEndings(false), mHeight(11),
        mUpdatingMesh(false)
    {
        SetFontAsset(AssetRef<FontAsset>(fontAssetId));
    }

    Text::~Text()
    {
        if (mFont)
            mFont->onCharactersRebuilt -= ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        if (mFontStyle)
            mFontStyle->onChanged -= ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);
    }

    Text& Text::operator=(const Text& other)
    {
        IRectDrawable::operator=(other);

        if (mFont)
            mFont->onCharactersRebuilt -= ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        if (mFontStyle)
            mFontStyle->onChanged -= ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        mText = other.mText;
        mFontAssetId = other.mFontAssetId;
        mFont = other.mFont;
        mFontStyle = other.mFontStyle;
        mFontStyleAsset = other.mFontStyleAsset;
        mSymbolsDistCoef = other.mSymbolsDistCoef;
        mLinesDistanceCoef = other.mLinesDistanceCoef;
        mVerAlign = other.mVerAlign;
        mHorAlign = other.mHorAlign;
        mWordWrap = other.mWordWrap;
        mDotsEndings = other.mDotsEndings;
        mHeight = other.mHeight;

        if (mFont)
            mFont->onCharactersRebuilt += ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        if (mFontStyle)
            mFontStyle->onChanged += ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        UpdateMesh();

        return *this;
    }

    void Text::Draw()
    {
        if (!mEnabled)
            return;

        // The view scale has the camera zoom and the screen density: glyphs are rendered for the pixels they cover
        Vec2F viewPixelScale = o2Render.GetViewPixelScale();
        if (IsDrawnByGlyphMeshes())
        {
            // The glyph meshes stay, only their fringe is put for the new scale
            float fringeScale = GetGlyphsFringeScale(viewPixelScale);
            if (!mGlyphsMesh || Math::Abs(fringeScale - mGlyphsFringeScale) > mGlyphsFringeScale*glyphsFringeScaleTolerance)
            {
                mGlyphsFringeScale = fringeScale;
                UpdateMesh();
            }
        }
        else
        {
            float pixelDensity = QuantizePixelDensity(Math::Max(Math::Abs(viewPixelScale.x), Math::Abs(viewPixelScale.y)));
            if (pixelDensity != GetPixelDensity() || mGlyphsMesh)
            {
                mPixelDensity = pixelDensity;
                CheckCharactersAndRebuildMesh();
            }
        }

        if (mGlyphsMesh && mGlyphsGeometry && mResultColor.a != 0)
            o2Render.DrawGeometry(mGlyphsGeometry, GetMaterial());

        for (auto& mesh : mMeshes)
        {
            mesh->Draw();

            if (o2Input.IsKeyDown(VK_F3))
                o2Render.DrawMeshWire(mesh.Get(), Color4(0, 255, 0, 150));
        }

        OnDrawn();
    }

    void Text::SetFont(const Ref<Font>& font)
    {
        if (mFont)
            mFont->onCharactersRebuilt -= ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        mFont = font;

        if (mFont)
            mFont->onCharactersRebuilt += ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        UpdateMesh();
    }

    const Ref<Font>& Text::GetFont() const
    {
        return mFont;
    }

    void Text::SetFontAsset(const AssetRef<FontAsset>& asset)
    {
        if (mFont)
            mFont->onCharactersRebuilt -= ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        if (asset.IsValid())
        {
            mFont = asset->GetFont();
            mFontAssetId = asset->GetUID();
        }
        else
            mFont = Ref<Font>();

        if (mFont)
            mFont->onCharactersRebuilt += ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        CheckCharactersAndRebuildMesh();
    }

    AssetRef<FontAsset> Text::GetFontAsset() const
    {
        auto& fontAssetInfo = o2Assets.GetAssetInfo(mFontAssetId);
        if (fontAssetInfo.meta->GetAssetType() == &TypeOf(BitmapFontAsset))
            return AssetRef<BitmapFontAsset>(mFontAssetId);

        return AssetRef<VectorFontAsset>(mFontAssetId);
    }

    void Text::SetFontStyle(const Ref<FontStyle>& style)
    {
        if (mFontStyle == style)
            return;

        if (mFontStyle)
            mFontStyle->onChanged -= ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        mFontStyle = style;
        mFontStyleAsset = AssetRef<FontStyleAsset>(dynamic_cast<FontStyleAsset*>(style.Get()));

        if (mFontStyle)
            mFontStyle->onChanged += ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        CheckCharactersAndRebuildMesh();
    }

    const Ref<FontStyle>& Text::GetFontStyle() const
    {
        return mFontStyle;
    }

    void Text::SetFontStyleAsset(const AssetRef<FontStyleAsset>& asset)
    {
        if (mFontStyle)
            mFontStyle->onChanged -= ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        mFontStyleAsset = asset;
        mFontStyle = asset.GetRef();

        if (mFontStyle)
            mFontStyle->onChanged += ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        CheckCharactersAndRebuildMesh();
    }

    const AssetRef<FontStyleAsset>& Text::GetFontStyleAsset() const
    {
        return mFontStyleAsset;
    }

    void Text::SetHeight(int height)
    {
        mHeight = height;
        CheckCharactersAndRebuildMesh();
    }

    int Text::GetFontHeight() const
    {
        return mHeight;
    }

    void Text::SetText(const WString& text)
    {
        if (text == mText)
			return;

        mText = text;
        CheckCharactersAndRebuildMesh();
    }

    const WString& Text::GetText() const
    {
        return mText;
    }

    void Text::SetHorAlign(HorAlign align)
    {
        mHorAlign = align;
        UpdateMesh();
    }

    HorAlign Text::GetHorAlign() const
    {
        return mHorAlign;
    }

    void Text::SetVerAlign(VerAlign align)
    {
        mVerAlign = align;
        UpdateMesh();
    }

    VerAlign Text::GetVerAlign() const
    {
        return mVerAlign;
    }

    void Text::SetWordWrap(bool flag)
    {
        mWordWrap = flag;
        UpdateMesh();
    }

    bool Text::GetWordWrap() const
    {
        return mWordWrap;
    }

    void Text::SetDotsEngings(bool flag)
    {
        mDotsEndings = flag;
        UpdateMesh();
    }

    bool Text::IsDotsEngings() const
    {
        return mDotsEndings;
    }

    void Text::SetSymbolsDistanceCoef(float coef)
    {
        mSymbolsDistCoef = coef;
        UpdateMesh();
    }

    float Text::GetSymbolsDistanceCoef() const
    {
        return mSymbolsDistCoef;
    }

    void Text::SetLinesDistanceCoef(float coef /*= 1*/)
    {
        mLinesDistanceCoef = coef;
        UpdateMesh();
    }

    float Text::GetLinesDistanceCoef() const
    {
        return mLinesDistanceCoef;
    }

    Text::SymbolsSet& Text::GetSymbolsSet()
    {
        return mSymbolsSet;
    }

    Vec2F Text::GetRealSize()
    {
        return mSymbolsSet.mRealSize;
    }

    RectF Text::GetRealRect()
    {
        return RectF(mTransform.origin.XY(), mTransform.origin.XY() + mSymbolsSet.mRealSize);
    }

    Vec2F Text::GetTextSize(const WString& text, const Ref<Font>& font, int height /*= 11*/, const Vec2F& areaSize /*= Vec2F()*/,
                            HorAlign horAlign /*= HorAlign::Left*/, VerAlign verAlign /*= VerAlign::Top*/,
                            bool wordWrap /*= true*/, bool dotsEngings /*= false*/, float charsDistCoef /*= 1.0f*/,
                            float linesDistCoef /*= 1.0f*/, const Ref<FontStyle>& style /*= nullptr*/)
    {
        // The metrics come from the rendered characters: a text not drawn yet at this height would measure short
        if (font)
        {
            if (style)
                font->CheckCharacters(text, height, style);
            else
                font->CheckCharacters(text, height);
        }

        SymbolsSet textSet;
        textSet.Initialize(font, style, text, height, Vec2F(), areaSize, horAlign, verAlign, wordWrap, dotsEngings,
                           charsDistCoef, linesDistCoef);

        return textSet.mRealSize;
    }

    void Text::UpdateMesh()
    {
        if (mUpdatingMesh)
            return;

        mUpdatingMesh = true;

        if (!mFont)
        {
            for (auto& mesh : mMeshes)
            {
                mesh->vertexCount = 0;
                mesh->polyCount = 0;
            }

            mGlyphsGeometry = nullptr;
            mUpdatingMesh = false;

            return;
        }

        int textLen = mText.Length();
        if (mMeshes.Count() == 0 && textLen == 0 && !mGlyphsMesh)
        {
            mUpdatingMesh = false;
            return;
        }

        if (IsDrawnByGlyphMeshes())
        {
            // Glyph bitmaps of the single density give the layout only
            mSymbolsSet.Initialize(mFont, mFontStyle, mText, mHeight, mTransform.origin.XY(), mSize.XY(), mHorAlign,
                                   mVerAlign, mWordWrap, mDotsEndings, mSymbolsDistCoef, mLinesDistanceCoef, 1.0f);

            mLastTransform = CalculateTextBasis();
            UpdateGlyphsMesh(mLastTransform);

            mUpdatingMesh = false;
            return;
        }

        if (mGlyphsMesh)
        {
            mGlyphsGeometry = nullptr;
            mGlyphsCoverage.Clear();
            mGlyphsMesh = false;
        }

        PrepareMesh(textLen);

        for (auto& mesh : mMeshes)
        {
            mesh->vertexCount = 0;
            mesh->polyCount = 0;
        }

        int currentMeshIdx = 0;
        Ref<Mesh> currentMesh = mMeshes[0];

        mSymbolsSet.Initialize(mFont, mFontStyle, mText, mHeight, mTransform.origin.XY(), mSize.XY(), mHorAlign, mVerAlign,
                               mWordWrap, mDotsEndings, mSymbolsDistCoef, mLinesDistanceCoef, GetPixelDensity());

        Basis transf = CalculateTextBasis();
        mLastTransform = transf;

        for (auto& line : mSymbolsSet.mLines)
        {
            for (auto& symb : line.mSymbols)
            {
                if (currentMesh->polyCount + 2 >= currentMesh->GetMaxPolyCount())
                    currentMesh = mMeshes[++currentMeshIdx];

                unsigned long color = mResultColor.ABGR();
                Vec2F points[4] =
                {
                    transf.Transform(symb.mFrame.LeftTop() - mSymbolsSet.mPosition),
                    transf.Transform(symb.mFrame.RightTop() - mSymbolsSet.mPosition),
                    transf.Transform(symb.mFrame.RightBottom() - mSymbolsSet.mPosition),
                    transf.Transform(symb.mFrame.LeftBottom() - mSymbolsSet.mPosition)
                };

                Vertex* verts = currentMesh->GetVertices<Vertex>();
                verts[currentMesh->vertexCount++] = Vertex(points[0], color, symb.mTexSrc.left, 1.0f - symb.mTexSrc.top);
                verts[currentMesh->vertexCount++] = Vertex(points[1], color, symb.mTexSrc.right, 1.0f - symb.mTexSrc.top);
                verts[currentMesh->vertexCount++] = Vertex(points[2], color, symb.mTexSrc.right, 1.0f - symb.mTexSrc.bottom);
                verts[currentMesh->vertexCount++] = Vertex(points[3], color, symb.mTexSrc.left, 1.0f - symb.mTexSrc.bottom);

                VertexIndex* idx = currentMesh->GetIndexes();
                int pp = currentMesh->polyCount*3;
                idx[pp] = currentMesh->vertexCount - 4;
                idx[pp + 1] = currentMesh->vertexCount - 3;
                idx[pp + 2] = currentMesh->vertexCount - 2;
                currentMesh->polyCount++;

                pp += 3;
                idx[pp] = currentMesh->vertexCount - 4;
                idx[pp + 1] = currentMesh->vertexCount - 2;
                idx[pp + 2] = currentMesh->vertexCount - 1;
                currentMesh->polyCount++;
            }
        }

        currentMesh->SetTexture(mFont->mTexture);

        Vec2F tangent = mTransform.xv.XY().Normalized();
        float z = mTransform.origin.z;
        for (auto& mesh : mMeshes)
        {
            Vertex* verts = mesh->GetVertices<Vertex>();
            for (UInt i = 0; i < mesh->vertexCount; i++)
            {
                verts[i].SetNormal(tangent);
                verts[i].z = z;
            }
        }

        mUpdatingMesh = false;
    }

    void Text::SetGlyphMeshesEnabled(bool enabled)
    {
        mGlyphMeshesEnabled = enabled;
    }

    bool Text::IsGlyphMeshesEnabled()
    {
        return mGlyphMeshesEnabled;
    }

    bool Text::IsDrawnByGlyphMeshes() const
    {
        return mGlyphMeshesEnabled && mFont && (!mFontStyle || mFontStyle->GetEffects().IsEmpty()) &&
            mFont->GetGlyphMesh(L' ') != nullptr;
    }

    float Text::GetGlyphsFringeScale(const Vec2F& viewPixelScale) const
    {
        Basis transform = CalculateTextBasis();
        float textScale = Math::Sqrt(Math::Abs(transform.xv.x*transform.yv.y - transform.xv.y*transform.yv.x));
        float viewScale = Math::Max(Math::Abs(viewPixelScale.x), Math::Abs(viewPixelScale.y));

        // A text of no size has no scale of its own
        float scale = viewScale*textScale*(float)mHeight/(float)Font::glyphMeshHeight;
        if (!std::isfinite(scale) || textScale <= 0.0f)
            scale = viewScale*(float)mHeight/(float)Font::glyphMeshHeight;

        return std::isfinite(scale) ? Math::Max(scale, 1e-3f) : 1.0f;
    }

    void Text::UpdateGlyphsMesh(const Basis& transform)
    {
        if (mGlyphsFringeScale <= 0.0f)
        {
            float graphicsScale = Integration::IsSingletonInitialzed() ? o2Integration.GetGraphicsScale() : 1.0f;
            mGlyphsFringeScale = GetGlyphsFringeScale(Vec2F(graphicsScale, graphicsScale));
        }

        UInt verticesCount = 0, indexesCount = 0;
        for (auto& line : mSymbolsSet.mLines)
        {
            for (auto& symbol : line.mSymbols)
            {
                const VectorMesh* glyph = mFont->GetGlyphMesh(symbol.mCharId);
                verticesCount += (UInt)glyph->positions.Count();
                indexesCount += (UInt)glyph->indexes.Count();
            }
        }

        mMeshes.Clear();
        mGlyphsMesh = true;

        auto glyphsIndexes = mmake<GlyphsIndexes>();
        glyphsIndexes->indexes.resize(indexesCount);

        RenderGeometry& geometry = GetChangingGlyphsGeometry();
        geometry.vertices.resize(verticesCount);
        mGlyphsCoverage.resize(verticesCount);
        geometry.indexes = glyphsIndexes->indexes.Data();
        geometry.trianglesCount = indexesCount/3;
        geometry.indexesOwner = glyphsIndexes;

        float unit = (float)mHeight/(float)Font::glyphMeshHeight;
        Vec2F fringeScale(mGlyphsFringeScale, mGlyphsFringeScale);
        Vec2F tangent = mTransform.xv.XY().Normalized();
        float z = mTransform.origin.z;

        UInt textAlpha = (UInt)mResultColor.a;
        Color32Bit opaqueColor = mResultColor.ABGR() & 0x00FFFFFF;
        mGlyphsColor = mResultColor;

        Vertex* vertices = geometry.vertices.Data();
        VertexIndex* indexes = glyphsIndexes->indexes.Data();
        UInt vertexIdx = 0, indexIdx = 0;

        for (auto& line : mSymbolsSet.mLines)
        {
            for (auto& symbol : line.mSymbols)
            {
                const VectorMesh& glyph = *mFont->GetGlyphMesh(symbol.mCharId);
                int count = glyph.positions.Count();
                if (count == 0)
                    continue;

                scaledGlyphPositions.resize(count);
                scaledGlyphColors.resize(count);
                glyph.FillScaled(fringeScale, scaledGlyphPositions.Data(), scaledGlyphColors.Data());

                Vec2F pen = symbol.mFrame.LeftBottom() + symbol.mOrigin - mSymbolsSet.mPosition;
                UInt base = vertexIdx;
                for (int i = 0; i < count; i++)
                {
                    UInt coverage = scaledGlyphColors[i] >> 24;
                    UInt alpha = coverage*textAlpha/255;
                    mGlyphsCoverage[vertexIdx] = (UInt8)coverage;
                    Vertex& vertex = vertices[vertexIdx++];
                    vertex.Set(transform.Transform(pen + scaledGlyphPositions[i]*unit), opaqueColor | (alpha << 24), 0.0f, 0.0f);
                    vertex.SetNormal(tangent);
                    vertex.z = z;
                }

                for (int i = 0; i < glyph.indexes.Count(); i++)
                    indexes[indexIdx++] = base + glyph.indexes[i];
            }
        }
    }

    RenderGeometry& Text::GetChangingGlyphsGeometry()
    {
        if (!mGlyphsGeometry)
            mGlyphsGeometry = mmake<RenderGeometry>();
        else if (mGlyphsGeometry->IsQueued())
        {
            auto geometry = mmake<RenderGeometry>();
            geometry->vertices = mGlyphsGeometry->vertices;
            geometry->indexes = mGlyphsGeometry->indexes;
            geometry->trianglesCount = mGlyphsGeometry->trianglesCount;
            geometry->indexesOwner = mGlyphsGeometry->indexesOwner;
            mGlyphsGeometry = geometry;
        }

        mGlyphsGeometry->OnChanged();
        return *mGlyphsGeometry;
    }

    void Text::CheckCharactersAndRebuildMesh()
    {
        if (mFont)
        {
            mFont->CheckCharacters(mText, height, mFontStyle, GetPixelDensity());
            mFont->CheckCharacters(".", height, mFontStyle, GetPixelDensity());
        }

        UpdateMesh();
    }

    float Text::QuantizePixelDensity(float pixelDensity)
    {
        const float minDensity = 0.5f, maxDensity = 4.0f, stepsPerOctave = 2.0f;

        if (!(pixelDensity > minDensity))
            return minDensity;

        if (pixelDensity > maxDensity)
            return maxDensity;

        // The nearest step up: glyphs are shown a bit smaller than rendered, never stretched
        return std::pow(2.0f, Math::Ceil(std::log2(pixelDensity)*stepsPerOctave - 0.01f)/stepsPerOctave);
    }

    float Text::GetPixelDensity() const
    {
        if (mPixelDensity > 0.0f)
            return mPixelDensity;

        return Integration::IsSingletonInitialzed() ? QuantizePixelDensity(o2Integration.GetGraphicsScale()) : 1.0f;
    }

    void Text::PrepareMesh(int charactersCount)
    {
        int needPolygons = charactersCount*2 + 15; // 15 for dots endings
        for (auto& mesh : mMeshes)
            needPolygons -= mesh->GetMaxPolyCount();

        if (needPolygons <= 0)
            return;

        if (mMeshes.Count() > 0 &&
            needPolygons + mMeshes.Last()->GetMaxPolyCount() < mMeshMaxPolyCount)
        {
            mMeshes.Last()->Resize(mMeshes.Last()->GetMaxVertexCount() + (UInt)needPolygons*2,
                                   mMeshes.Last()->GetMaxPolyCount() + (UInt)needPolygons);
            return;
        }

        while (needPolygons > 0)
        {
            int polyCount = Math::Min<int>(needPolygons, mMeshMaxPolyCount);
            needPolygons -= polyCount;
            mMeshes.Add(mmake<Mesh>(mFont->mTexture, polyCount * 2, polyCount));
        }
    }

    Basis Text::CalculateTextBasis() const
    {
        Basis transf;
        if (mSize.XY() == Vec2F())
        {
            transf = Basis::Build(mPosition.XY(), mScale.XY(), mEulerAngles.z, mShear.x);
        }
        else
        {
            transf = mTransform.ToBasis();
            transf.xv /= mSize.x;
            transf.yv /= mSize.y;
        }

        return transf;
    }

    void Text::OnColorChanged()
    {
        // Vertices of the glyph meshes have the coverage in their alpha
        if (mGlyphsMesh)
        {
            if (!mGlyphsGeometry || mGlyphsGeometry->vertices.IsEmpty() || mResultColor == mGlyphsColor)
                return;

            mGlyphsColor = mResultColor;

            // Alpha of the vertices is the coverage of the glyph by the alpha of the text
            UInt textAlpha = (UInt)mResultColor.a;
            Color32Bit opaqueColor = mResultColor.ABGR() & 0x00FFFFFF;

            Vertex* vertices = GetChangingGlyphsGeometry().vertices.Data();
            for (int i = 0; i < mGlyphsCoverage.Count(); i++)
                vertices[i].color = opaqueColor | (((UInt)mGlyphsCoverage[i]*textAlpha/255) << 24);

            return;
        }

        ULong dcolor = mResultColor.ABGR();
        for (auto& mesh : mMeshes)
        {
            Vertex* verts = mesh->GetVertices<Vertex>();
            for (int i = 0; i < (int)mesh->vertexCount; i++)
                verts[i].color = dcolor;
        }
    }


	void Text::OnMaterialChanged()
	{
        for (auto& mesh : mMeshes)
			mesh->SetMaterial(GetMaterial());

		IRectDrawable::OnMaterialChanged();
	}

	void Text::BasisChanged()
    {
        if (mSymbolsSet.mAreaSize != mSize.XY())
            CheckCharactersAndRebuildMesh();
        else
        {
            Basis transform = CalculateTextBasis();
            Basis meshTransform = mLastTransform.Inverted()*transform;

            meshTransform.origin.x = Math::Round(meshTransform.origin.x);
            meshTransform.origin.y = Math::Round(meshTransform.origin.y);

            TransformMesh(mLastTransform.Inverted()*transform);
            mLastTransform = transform;

            float z = mTransform.origin.z;
            for (auto& mesh : mMeshes)
            {
                Vertex* verts = mesh->GetVertices<Vertex>();
                for (UInt i = 0; i < mesh->vertexCount; i++)
                    verts[i].z = z;
            }

            if (mGlyphsMesh && mGlyphsGeometry && !mGlyphsGeometry->vertices.IsEmpty() &&
                mGlyphsGeometry->vertices[0].z != z)
            {
                for (Vertex& vertex : GetChangingGlyphsGeometry().vertices)
                    vertex.z = z;
            }
        }
    }

    void Text::OnDeserialized(const DataValue& node)
    {
        Transform::OnDeserialized(node);
        SetFontAsset(AssetRef<FontAsset>(mFontAssetId));
        SetFontStyleAsset(mFontStyleAsset);
    }

    void Text::TransformMesh(const Basis& bas)
    {
        if (bas == Basis::Identity())
            return;

        for (auto& mesh : mMeshes)
        {
            Vertex* verts = mesh->GetVertices<Vertex>();
            for (unsigned int i = 0; i < mesh->vertexCount; i++)
            {
                Vertex* vx = &verts[i];
                bas.Transform(vx->x, vx->y);
            }
        }

        if (mGlyphsMesh && mGlyphsGeometry)
        {
            for (Vertex& vertex : GetChangingGlyphsGeometry().vertices)
                bas.Transform(vertex.x, vertex.y);
        }

        mSymbolsSet.Move(bas.origin);
    }

    void Text::SymbolsSet::Initialize(const Ref<Font>& font, const Ref<FontStyle>& style, const WString& text, int height,
                                      const Vec2F& position, const Vec2F& areaSize,
                                      HorAlign horAlign, VerAlign verAlign, bool wordWrap, bool dotsEngings,
                                      float charsDistCoef, float linesDistCoef, float pixelDensity /*= 1.0f*/)
    {
        mFont = font;
        mStyle = style;
        mText = text;
        mHeight = height;
        mPosition = Vec2F(Math::Round(position.x), Math::Round(position.y));
        mAreaSize = areaSize;
        mRealSize = Vec2F();
        mHorAlign = horAlign;
        mVerAlign = verAlign;
        mWordWrap = wordWrap;
        mSymbolsDistCoef = charsDistCoef;
        mLinesDistCoef = linesDistCoef;
        mDotsEndings = dotsEngings;

        mLines.Clear();
        int textLen = mText.Length();

        if (textLen == 0)
            return;

        mFont->CheckCharacters(mText, mHeight, mStyle, pixelDensity);
        mFont->CheckCharacters(".", mHeight, mStyle, pixelDensity);

        float linesDist = mFont->GetLineHeightPx(mHeight)*mLinesDistCoef;
        float fontHeight = mFont->GetHeightPx(mHeight);

        mLines.Add(Line());
        Line* curLine = &mLines.Last();
        curLine->mSize.y = fontHeight;

        float dotsSize = mFont->GetCharacter('.', mHeight, mStyle, pixelDensity).mAdvance*3.0f;

        // The dots end a line that doesn't fit: the room for them is kept only then
        auto restOfLineFits = [&](int from) {
            float width = curLine->mSize.x;
            for (int j = from; j < textLen && mText[j] != '\n'; j++)
                width += mFont->GetCharacter(mText[j], mHeight, mStyle, pixelDensity).mAdvance*mSymbolsDistCoef;

            return width <= mAreaSize.x;
        };

        Vec2F fullSize(0, fontHeight);
        bool checkAreaBounds = mWordWrap && mAreaSize.x > FLT_EPSILON;
        int wrapCharIdx = -1;
        int fittingLine = -1;
        for (int i = 0; i < textLen; i++)
        {
            const Font::Character& ch = mFont->GetCharacter(mText[i], mHeight, mStyle, pixelDensity);
            Vec2F chSize = ch.mSize;
            Vec2F chPos = Vec2F(curLine->mSize.x - ch.mOrigin.x, -ch.mOrigin.y);

            bool dotsRoomTaken = mDotsEndings && mText[i] != '\n' && fittingLine != mLines.Count() - 1 &&
                curLine->mSize.x + ch.mAdvance*mSymbolsDistCoef > mAreaSize.x - dotsSize;

            if (dotsRoomTaken && restOfLineFits(i))
            {
                fittingLine = mLines.Count() - 1;
                dotsRoomTaken = false;
            }

            if (dotsRoomTaken)
            {
                const Font::Character& dotCh = mFont->GetCharacter('.', mHeight, mStyle, pixelDensity);
                Vec2F dotChSize = dotCh.mSize;

                for (int j = 0; j < 3; j++)
                {
                    Vec2F dotChPos = Vec2F(curLine->mSize.x - dotCh.mOrigin.x, -dotCh.mOrigin.y);
                    curLine->mSymbols.Add(Symbol(dotChPos, dotChSize, dotCh.mTexSrc, dotCh.mId, dotCh.mOrigin, dotCh.mAdvance));
                    curLine->mString += '.';
                    curLine->mSize.x += dotCh.mAdvance*mSymbolsDistCoef;
                }

                for (; i < textLen - 1; i++)
                {
                    if (mText[i + 1] == '\n')
                        break;
                }

                continue;
            }

            curLine->mSymbols.Add(Symbol(chPos, chSize, ch.mTexSrc, ch.mId, ch.mOrigin, ch.mAdvance));

            if (mText[i] != '\n')
                curLine->mSize.x += ch.mAdvance*mSymbolsDistCoef;

            curLine->mString += mText[i];

            // A space may hang past the edge: the break comes at the next word
            bool outOfBounds = checkAreaBounds ? curLine->mSize.x > mAreaSize.x && i > curLine->mLineBegSymbol && mText[i] != ' ' : false;

            if (mText[i] == '\n' || outOfBounds)
            {
                if (outOfBounds)
                {
                    // A line broken at a space keeps the space at its end, as the following line starts with the next word
                    bool atSpace = wrapCharIdx >= 0 && wrapCharIdx != curLine->mLineBegSymbol;
                    if (!atSpace)
                        wrapCharIdx = i;
                    else
                        curLine->mSpacesCount--;

                    int cutLen = wrapCharIdx - curLine->mLineBegSymbol + (atSpace ? 1 : 0);
                    float width = atSpace && cutLen > 1 ? curLine->mSymbols[cutLen - 2].mFrame.right : 0.0f;

                    curLine->mSymbols.RemoveRange(cutLen, curLine->mSymbols.Count());
                    curLine->mString.Erase(cutLen);

                    if (atSpace)
                        curLine->mSize.x = width;
                    else if (curLine->mSymbols.Count() > 0)
                        curLine->mSize.x = curLine->mSymbols.Last().mFrame.right;
                    else
                        curLine->mSize.x = 0;

                    i = atSpace ? wrapCharIdx : wrapCharIdx - 1;
                    wrapCharIdx = -1;
                }
                else
                {
                    curLine->mSymbols.PopBack();
                    curLine->mString.PopBack();
                    curLine->mEndedNewLine = true;
                }

                fullSize.x = Math::Max(fullSize.x, curLine->mSize.x);
                fullSize.y += linesDist;

                mLines.Add(Line());
                curLine = &mLines.Last();
                curLine->mSize.y = linesDist;
                curLine->mLineBegSymbol = i + 1;
                wrapCharIdx = -1;
            }
            else if (mText[i] == ' '/* || mFont->mAllSymbolReturn*/)
            {
                curLine->mSpacesCount++;
                wrapCharIdx = i;
            }
        }

        fullSize.x = Math::Max(fullSize.x, curLine->mSize.x);

        float lineHeight = linesDist;
        float yOffset = mAreaSize.y - mLines[0].mSize.y;

        if (mVerAlign == VerAlign::Both)
            lineHeight = Math::Max(fontHeight, (mAreaSize.y - fontHeight)/(float)(mLines.Count() - 1));
        else if (mVerAlign == VerAlign::Bottom)
            yOffset = fullSize.y - mLines[0].mSize.y;
        else if (mVerAlign == VerAlign::Middle)
            yOffset -= mAreaSize.y*0.5f - fullSize.y*0.5f;

        yOffset += mPosition.y;

        for (Vector<Line>::Iterator it = mLines.begin(); it != mLines.end(); ++it)
        {
            Line* line = &(*it);

            float xOffset = 0;
            float additiveSpaceOffs = 0;

            if (mHorAlign == HorAlign::Middle)
                xOffset = (mAreaSize.x - line->mSize.x)*0.5f;
            else if (mHorAlign == HorAlign::Right)
                xOffset = mAreaSize.x - line->mSize.x;
            else if (mHorAlign == HorAlign::Both)
                additiveSpaceOffs = Math::Max(0.0f, (mAreaSize.x - line->mSize.x)/(float)line->mSpacesCount);

            xOffset += mPosition.x;

            Vec2F locOrigin((float)(int)xOffset, (float)(int)yOffset);
            line->mPosition = locOrigin;
            yOffset -= lineHeight;

            for (Vector<Symbol>::Iterator jt = line->mSymbols.begin(); jt != line->mSymbols.end(); ++jt)
            {
                if (jt->mCharId == ' ')
                    locOrigin.x += additiveSpaceOffs;

                jt->mFrame = jt->mFrame + locOrigin;
            }
        }

        mRealSize = fullSize;
    }

    void Text::SymbolsSet::Move(const Vec2F& offs)
    {
        for (auto& line : mLines)
        {
            for (auto& sm : line.mSymbols)
                sm.mFrame += offs;

            line.mPosition += offs;
        }
    }

    Text::SymbolsSet::Symbol::Symbol()
    {}

    Text::SymbolsSet::Symbol::Symbol(const Vec2F& position, const Vec2F& size, const RectF& texSrc,
                                     UInt16 charId, const Vec2F& origin, float advance):
        mFrame(position, position + size), mTexSrc(texSrc), mCharId(charId), mOrigin(origin), mAdvance(advance)
    {}

    bool Text::SymbolsSet::Symbol::operator==(const Symbol& other) const
    {
        return mCharId == other.mCharId && mFrame == other.mFrame;
    }

    Text::SymbolsSet::Line::Line():
        mLineBegSymbol(0), mSpacesCount(0), mEndedNewLine(false)
    {}

    bool Text::SymbolsSet::Line::operator==(const Line& other) const
    {
        return false;
    }
}
// --- META ---

DECLARE_CLASS(o2::Text, o2__Text);
// --- END META ---
