#include "o2/stdafx.h"
#include "Text.h"

#include "o2/Application/Input.h"
#include "o2/Assets/Assets.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"

namespace o2
{
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
    }

    Text& Text::operator=(const Text& other)
    {
        IRectDrawable::operator=(other);

        if (mFont)
            mFont->onCharactersRebuilt -= ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        mText = other.mText;
        mFontAssetId = other.mFontAssetId;
        mFont = other.mFont;
        mSymbolsDistCoef = other.mSymbolsDistCoef;
        mLinesDistanceCoef = other.mLinesDistanceCoef;
        mVerAlign = other.mVerAlign;
        mHorAlign = other.mHorAlign;
        mWordWrap = other.mWordWrap;
        mDotsEndings = other.mDotsEndings;
        mHeight = other.mHeight;

        if (mFont)
            mFont->onCharactersRebuilt += ObjFunctionPtr<Text, void>(this, &Text::CheckCharactersAndRebuildMesh);

        UpdateMesh();

        return *this;
    }

    void Text::Draw()
    {
        if (!mEnabled)
            return;

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
                            float linesDistCoef /*= 1.0f*/)
    {
        SymbolsSet textSet;
        textSet.Initialize(font, text, height, Vec2F(), areaSize, horAlign, verAlign, wordWrap, dotsEngings, charsDistCoef,
                           linesDistCoef);

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

            mUpdatingMesh = false;

            return;
        }

        int textLen = mText.Length();
        if (mMeshes.Count() == 0 && textLen == 0)
        {
            mUpdatingMesh = false;
            return;
        }

        PrepareMesh(textLen);

        for (auto& mesh : mMeshes)
        {
            mesh->vertexCount = 0;
            mesh->polyCount = 0;
        }

        int currentMeshIdx = 0;
        Ref<Mesh> currentMesh = mMeshes[0];

        mSymbolsSet.Initialize(mFont, mText, mHeight, mTransform.origin.XY(), mSize.XY(), mHorAlign, mVerAlign, mWordWrap,
                               mDotsEndings, mSymbolsDistCoef, mLinesDistanceCoef);

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

    void Text::CheckCharactersAndRebuildMesh()
    {
        if (mFont)
        {
            mFont->CheckCharacters(mText, height);
            mFont->CheckCharacters(".", height);
        }

        UpdateMesh();
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
        }
    }

    void Text::OnDeserialized(const DataValue& node)
    {
        Transform::OnDeserialized(node);
        SetFontAsset(AssetRef<FontAsset>(mFontAssetId));
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

        mSymbolsSet.Move(bas.origin);
    }

    void Text::SymbolsSet::Initialize(const Ref<Font>& font, const WString& text, int height, const Vec2F& position, const Vec2F& areaSize,
                                      HorAlign horAlign, VerAlign verAlign, bool wordWrap, bool dotsEngings,
                                      float charsDistCoef, float linesDistCoef)
    {
        mFont = font;
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

        float linesDist = mFont->GetLineHeightPx(mHeight)*mLinesDistCoef;
        float fontHeight = mFont->GetHeightPx(mHeight);

        mLines.Add(Line());
        Line* curLine = &mLines.Last();
        curLine->mSize.y = fontHeight;

        float dotsSize = mFont->GetCharacter('.', mHeight).mAdvance*3.0f;

        Vec2F fullSize(0, fontHeight);
        bool checkAreaBounds = mWordWrap && mAreaSize.x > FLT_EPSILON;
        int wrapCharIdx = -1;
        for (int i = 0; i < textLen; i++)
        {
            const Font::Character& ch = mFont->GetCharacter(mText[i], mHeight);
            Vec2F chSize = ch.mSize;
            Vec2F chPos = Vec2F(curLine->mSize.x - ch.mOrigin.x, -ch.mOrigin.y);

            if (mDotsEndings && mText[i] != '\n' && curLine->mSize.x + ch.mAdvance*mSymbolsDistCoef > mAreaSize.x - dotsSize)
            {
                const Font::Character& dotCh = mFont->GetCharacter('.', mHeight);
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

            bool outOfBounds = checkAreaBounds ? curLine->mSize.x > mAreaSize.x && i > curLine->mLineBegSymbol : false;

            if (mText[i] == '\n' || outOfBounds)
            {
                if (outOfBounds)
                {
                    if (wrapCharIdx < 0 || wrapCharIdx == curLine->mLineBegSymbol)
                        wrapCharIdx = i;
                    else
                        curLine->mSpacesCount--;

                    int cutLen = wrapCharIdx - curLine->mLineBegSymbol;

                    curLine->mSymbols.RemoveRange(cutLen, curLine->mSymbols.Count());
                    curLine->mString.Erase(cutLen);


                    if (curLine->mSymbols.Count() > 0)
                        curLine->mSize.x = curLine->mSymbols.Last().mFrame.right;
                    else
                        curLine->mSize.x = 0;

                    i = wrapCharIdx - 1;
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
