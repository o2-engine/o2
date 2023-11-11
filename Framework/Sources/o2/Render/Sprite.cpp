#include "o2/stdafx.h"
#include "Sprite.h"

#include "o2/Application/Input.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    Sprite::Sprite():
        mMeshBuildFunc(&Sprite::BuildDefaultMesh), mMesh(NoTexture(), 16, 18)
    {
        for (int i = 0; i < 4; i++)
            mCornersColors[i] = Color4::White();

        UpdateMesh();

        Render::OnSpriteCreated(this);
    }

    Sprite::Sprite(const ImageAssetRef& image):
        mMeshBuildFunc(&Sprite::BuildDefaultMesh), mMesh(NoTexture(), 16, 18)
    {
        for (int i = 0; i < 4; i++)
            mCornersColors[i] = Color4::White();

        LoadFromImage(image);

        Render::OnSpriteCreated(this);
    }

    Sprite::Sprite(const String& imagePath):
        mMeshBuildFunc(&Sprite::BuildDefaultMesh), mMesh(NoTexture(), 16, 18)
    {
        for (int i = 0; i < 4; i++)
            mCornersColors[i] = Color4::White();

        LoadFromImage(imagePath);

        Render::OnSpriteCreated(this);
    }

    Sprite::Sprite(UID imageId):
        mMeshBuildFunc(&Sprite::BuildDefaultMesh), mMesh(NoTexture(), 16, 18)
    {
        for (int i = 0; i < 4; i++)
            mCornersColors[i] = Color4::White();

        LoadFromImage(imageId);

        Render::OnSpriteCreated(this);
    }

    Sprite::Sprite(TextureRef texture, const RectI& srcRect /*= RectI()*/):
        mTextureSrcRect(srcRect), mMeshBuildFunc(&Sprite::BuildDefaultMesh), mMesh(texture, 16, 18)
    {
        if (srcRect == RectI())
            mTextureSrcRect = RectI(Vec2I(), texture->GetSize());

        for (int i = 0; i < 4; i++)
            mCornersColors[i] = Color4::White();

        UpdateMesh();

        Render::OnSpriteCreated(this);
    }

    Sprite::Sprite(const Color4& color):
        mMeshBuildFunc(&Sprite::BuildDefaultMesh), mMesh(NoTexture(), 16, 18)
    {
        for (int i = 0; i < 4; i++)
            mCornersColors[i] = Color4::White();

        LoadMonoColor(color);

        Render::OnSpriteCreated(this);
    }

    Sprite::Sprite(Bitmap* bitmap):
        mMeshBuildFunc(&Sprite::BuildDefaultMesh), mMesh(NoTexture(), 16, 18)
    {
        for (int i = 0; i < 4; i++)
            mCornersColors[i] = Color4::White();

        LoadFromBitmap(bitmap);

        Render::OnSpriteCreated(this);
    }

    Sprite::Sprite(const Sprite& other):
        mImageAsset(other.mImageAsset), mTextureSrcRect(other.mTextureSrcRect), IRectDrawable(other), 
        mMesh(other.mMesh), mMode(other.mMode), mFill(other.mFill), mSlices(other.mSlices),
        mMeshBuildFunc(other.mMeshBuildFunc), mTileScale(other.mTileScale),
        texture(this), textureSrcRect(this), image(this), imageName(this), leftTopColor(this), rightTopColor(this),
        leftBottomColor(this), rightBottomColor(this), mode(this), fill(this), tileScale(this), sliceBorder(this), 
        bitmap(this)
    {
        for (int i = 0; i < 4; i++)
            mCornersColors[i] = other.mCornersColors[i];

        Render::OnSpriteCreated(this);
    }

    Sprite::~Sprite()
    {
        Render::OnSpriteDestroyed(this);
    }

    Sprite& Sprite::operator=(const Sprite& other)
    {
        mMesh           = other.mMesh;
        mTextureSrcRect = other.mTextureSrcRect;
        mImageAsset     = other.mImageAsset;
        mMode           = other.mMode;
        mFill           = other.mFill;
        mSlices         = other.mSlices;
        mTileScale      = other.mTileScale;
        mMeshBuildFunc  = other.mMeshBuildFunc;
        IRectDrawable::operator=(other);

        return *this;
    }

    void Sprite::Draw()
    {
        //PROFILE_SAMPLE_FUNC();

        if (!mEnabled)
            return;

        if (IsRenderDrawCallsDebugEnabled())
            o2Render.mLog->Out("- Draw sprite: " + mImageAsset->GetPath());

        mMesh.Draw();
        OnDrawn();

        if (o2Input.IsKeyDown(VK_F3))
            o2Render.DrawMeshWire(&mMesh, Color4(0, 0, 0, 100));

//         o2Render.DrawBasis(mTransform);
    }

    void Sprite::SetTexture(TextureRef texture)
    {
        mMesh.SetTexture(texture);
        mImageAsset = ImageAssetRef();
    }

    TextureRef Sprite::GetTexture() const
    {
        return mMesh.GetTexture();
    }

    void Sprite::SetTextureSrcRect(const RectI& rect)
    {
        mTextureSrcRect = rect;
    }

    RectI Sprite::GetTextureSrcRect() const
    {
        return mTextureSrcRect;
    }

    Vec2I Sprite::GetOriginalSize() const
    {
        return mTextureSrcRect.Size();
    }

    void Sprite::SetCornerColor(Corner corner, const Color4& color)
    {
        mCornersColors[(int)corner] = color;
    }

    Color4 Sprite::GetCornerColor(Corner corner) const
    {
        return mCornersColors[(int)corner];
    }

    void Sprite::SetLeftTopColor(const Color4& color)
    {
        mCornersColors[(int)Corner::LeftTop] = color;
    }

    Color4 Sprite::GetLeftTopCorner() const
    {
        return mCornersColors[(int)Corner::LeftTop];
    }

    void Sprite::SetRightTopColor(const Color4& color)
    {
        mCornersColors[(int)Corner::RightTop] = color;
    }

    Color4 Sprite::GetRightTopCorner() const
    {
        return mCornersColors[(int)Corner::RightTop];
    }

    void Sprite::SetRightBottomColor(const Color4& color)
    {
        mCornersColors[(int)Corner::RightBottom] = color;
    }

    Color4 Sprite::GetRightBottomCorner() const
    {
        return mCornersColors[(int)Corner::RightBottom];
    }

    void Sprite::SetLeftBottomColor(const Color4& color)
    {
        mCornersColors[(int)Corner::LeftBottom] = color;
    }

    Color4 Sprite::GetLeftBottomCorner() const
    {
        return mCornersColors[(int)Corner::LeftBottom];
    }

    void Sprite::SetFill(float fill)
    {
        if (Math::Equals(mFill, fill))
            return;

        mFill = Math::Clamp01(fill);
        UpdateMesh();
    }

    float Sprite::GetFill() const
    {
        return mFill;
    }

    void Sprite::SetTileScale(float scale)
    {
        mTileScale = Math::Abs(scale);
        UpdateMesh();
    }

    float Sprite::GetTileScale() const
    {
        return mTileScale;
    }

    void Sprite::SetMode(SpriteMode mode)
    {
        if (mode == mMode)
            return;

        mMode = mode;

        switch (mode)
        {
            case SpriteMode::Default:         mMeshBuildFunc = &Sprite::BuildDefaultMesh; break;
            case SpriteMode::Sliced:          mMeshBuildFunc = &Sprite::BuildSlicedMesh; break;
            case SpriteMode::Tiled:           mMeshBuildFunc = &Sprite::BuildTiledMesh; break;
            case SpriteMode::FixedAspect:     mMeshBuildFunc = &Sprite::BuildFixedAspectMesh; break;
            case SpriteMode::FillLeftToRight: mMeshBuildFunc = &Sprite::BuildFillLeftToRightMesh; break;
            case SpriteMode::FillRightToLeft: mMeshBuildFunc = &Sprite::BuildFillRightToLeftMesh; break;
            case SpriteMode::FillUpToDown:    mMeshBuildFunc = &Sprite::BuildFillUpToDownMesh; break;
            case SpriteMode::FillDownToUp:    mMeshBuildFunc = &Sprite::BuildFillDownToUpMesh; break;
            case SpriteMode::Fill360CW:       mMeshBuildFunc = &Sprite::BuildFill360CWMesh; break;
            case SpriteMode::Fill360CCW:      mMeshBuildFunc = &Sprite::BuildFill360CCWMesh; break;
            default:                          mMeshBuildFunc = &Sprite::BuildDefaultMesh; break;
        }

        UpdateMesh();
    }

    SpriteMode Sprite::GetMode() const
    {
        return mMode;
    }

    void Sprite::SetSliceBorder(const BorderI& border)
    {
        if (mSlices == border)
            return;

        mSlices = border;
        UpdateMesh();
    }

    BorderI Sprite::GetSliceBorder() const
    {
        return mSlices;
    }

    bool Sprite::operator==(const Sprite& other) const
    {
        return IRectDrawable::operator==(other) && mTextureSrcRect == other.mTextureSrcRect && 
            mCornersColors[0] == other.mCornersColors[0] && mCornersColors[1] == other.mCornersColors[1] &&
            mCornersColors[2] == other.mCornersColors[2] && mCornersColors[3] == other.mCornersColors[3] &&
            mImageAsset == other.mImageAsset && mMode == other.mMode && mSlices == other.mSlices &&
            Math::Equals(mTileScale, other.mTileScale) && Math::Equals(mFill, other.mFill);
    }

    bool Sprite::operator!=(const Sprite& other) const
    {
        return !operator==(other);
    }

    void Sprite::LoadFromImage(const ImageAssetRef& image, bool setSizeByImage /*= true*/)
    {
        if (!image)
        {
            LoadMonoColor(Color4::White());
            return;
        }

        mImageAsset = image;

        InitializeTexture();

        mSlices = image->GetMeta()->sliceBorder;

        SetMode(image->GetMeta()->defaultMode);

        if (setSizeByImage)
            SetSize(mTextureSrcRect.Size());
        else
            UpdateMesh();
    }

    void Sprite::LoadFromImage(const String& imagePath, bool setSizeByImage /*= true*/)
    {
        ImageAssetRef assetRef = o2Assets.GetAssetRef(imagePath);
        if (assetRef)
            LoadFromImage(assetRef, setSizeByImage);
        else 
            o2Debug.LogWarning("Can't load sprite from image by path (" + imagePath + "): image isn't exist");
    }

    void Sprite::LoadFromImage(UID imageId, bool setSizeByImage /*= true*/)
    {
        ImageAssetRef assetRef = o2Assets.GetAssetRef(imageId);
        if (assetRef)
            LoadFromImage(assetRef, setSizeByImage);
        else 
            o2Debug.LogWarning("Can't create sprite from image by id (" + imageId.ToString() + "): image isn't exist");
    }

    void Sprite::LoadMonoColor(const Color4& color)
    {
        mImageAsset = ImageAssetRef();
        mMesh.mTexture = TextureRef();
        mColor = color;
        mCornersColors[0] = Color4::White();
        mCornersColors[1] = Color4::White();
        mCornersColors[2] = Color4::White();
        mCornersColors[3] = Color4::White();

        UpdateMesh();
    }

    void Sprite::LoadFromBitmap(Bitmap* bitmap, bool setSizeByImage /*= true*/)
    {
        if (bitmap)
        {
            mImageAsset = ImageAssetRef();
            mMesh.mTexture = TextureRef(bitmap);
            mTextureSrcRect.Set(Vec2F(), mMesh.mTexture->GetSize());

            if (setSizeByImage)
                SetSize(mMesh.mTexture->GetSize());
        }
        else o2Debug.LogWarningStr("Can't create sprite from bitmap: bitmap is null");
    }

    void Sprite::SetImageAsset(const ImageAssetRef& asset)
    {
        LoadFromImage(asset, false);
    }

    ImageAssetRef Sprite::GetImageAsset() const
    {
        return mImageAsset;
    }

    const String& Sprite::GetImageName() const
    {
        if (mImageAsset)
            return mImageAsset->GetPath();

        return String::empty;
    }

    UID Sprite::GetAtlasAssetId() const
    {
        if (mImageAsset)
            return mImageAsset->GetAtlasUID();

        return 0;
    }

    void Sprite::NormalizeSize()
    {
        SetSize(mTextureSrcRect.Size());
    }

    void Sprite::NormalizeAspectByWidth()
    {
        float aspect = (float)mTextureSrcRect.Width()/(float)mTextureSrcRect.Height();
        SetSize(Vec2F(mSize.x, mSize.x/aspect));
    }

    void Sprite::NormalizeAspectByHeight()
    {
        float aspect = (float)mTextureSrcRect.Width()/(float)mTextureSrcRect.Height();
        SetSize(Vec2F(mSize.y*aspect, mSize.y));
    }

    void Sprite::NormalizeAspect()
    {
        float aspect = (float)mTextureSrcRect.Width()/(float)mTextureSrcRect.Height();

        if (aspect > 1.0f)
            SetSize(Vec2F(mSize.y*aspect, mSize.y));
        else
            SetSize(Vec2F(mSize.x, mSize.x/aspect));
    }

    void Sprite::BasisChanged()
    {
        UpdateMesh();
    }

    void Sprite::ColorChanged()
    {
        UpdateMesh();
    }

    void Sprite::UpdateMesh()
    {
        (this->*mMeshBuildFunc)();
    }

    void Sprite::BuildDefaultMesh()
    {
        Vec2F invTexSize(1.0f, 1.0f);
        if (mMesh.mTexture)
            invTexSize.Set(1.0f/mMesh.mTexture->GetSize().x, 1.0f/mMesh.mTexture->GetSize().y);

        ULong rcc[4];
        for (int i = 0; i < 4; i++)
            rcc[i] = (mColor*mCornersColors[i]).ABGR();

        static VertexIndex indexes[] ={ 0, 1, 2, 0, 2, 3 };

        float uvLeft = mTextureSrcRect.left*invTexSize.x;
        float uvRight = mTextureSrcRect.right*invTexSize.x;

        float uvUp = 1.0f - mTextureSrcRect.bottom*invTexSize.y;
        float uvDown = 1.0f - mTextureSrcRect.top*invTexSize.y;

        mMesh.vertices[0].Set(mTransform.origin + mTransform.yv, rcc[0], uvLeft, uvUp);
        mMesh.vertices[1].Set(mTransform.origin + mTransform.yv + mTransform.xv, rcc[1], uvRight, uvUp);
        mMesh.vertices[2].Set(mTransform.origin + mTransform.xv, rcc[2], uvRight, uvDown);
        mMesh.vertices[3].Set(mTransform.origin, rcc[3], uvLeft, uvDown);

        memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*6);

        mMesh.vertexCount = 4;
        mMesh.polyCount = 2;
    }

    void Sprite::BuildSlicedMesh()
    {
        Vec2F lastTransformXv = mTransform.xv;
        float lastSizeX = mSize.x;

        mTransform.xv *= mFill;
        mSize.x *= mFill;

        Vec2F invTexSize(1.0f, 1.0f);
        if (mMesh.mTexture)
            invTexSize.Set(1.0f/mMesh.mTexture->GetSize().x, 1.0f/mMesh.mTexture->GetSize().y);

        ULong rcc[4];
        for (int i = 0; i < 4; i++)
            rcc[i] = (mColor*mCornersColors[i]).ABGR();

        static VertexIndex indexes[] ={
            0, 1, 5,    0, 5, 4,    1, 2, 6,    1, 6, 5,    2, 3, 7,    2, 7, 6,
            4, 5, 9,    4, 9, 8,    5, 6, 10,   5, 10, 9,   6, 7, 11,   6, 11, 10,
            8, 9, 13,   8, 13, 12,  9, 10, 14,  9, 14, 13,  10, 11, 15, 10, 15, 14
        };

        Vec2F texSrcSize = mTextureSrcRect.Size();
        Vec2F invTexSrcSize = Vec2F::One()/texSrcSize;
        Vec2F sz = mSize*mScale;

        float u0 = mTextureSrcRect.left*invTexSize.x;
        float u3 = mTextureSrcRect.right*invTexSize.x;
        float u1 = Math::Lerp(u0, u3, (float)mSlices.left*invTexSrcSize.x);
        float u2 = Math::Lerp(u0, u3, (texSrcSize.x - (float)mSlices.right)*invTexSrcSize.x);

        float v0 = 1.0f - mTextureSrcRect.top*invTexSize.y;
        float v3 = 1.0f - mTextureSrcRect.bottom*invTexSize.y;
        float v1 = Math::Lerp(v0, v3, (float)mSlices.bottom*invTexSrcSize.y);
        float v2 = Math::Lerp(v0, v3, (texSrcSize.y - (float)mSlices.top)*invTexSrcSize.y);

        Vec2F xv = mTransform.xv/sz.x;
        Vec2F yv = mTransform.yv/sz.y;

        Vec2F o = mTransform.origin;

        Vec2F r1 = xv*(float)mSlices.left;
        Vec2F r2 = xv*(sz.x - (float)mSlices.right);
        Vec2F r3 = mTransform.xv;

        Vec2F t1 = yv*(float)mSlices.bottom;
        Vec2F t2 = yv*(sz.y - (float)mSlices.top);
        Vec2F t3 = mTransform.yv;

        if (mSlices.left + mSlices.right > sz.x)
        {
            float d = (mSlices.left + mSlices.right - sz.x)*0.5f;
            r1 -= xv*d;
            r2 += xv*d;

            u1 = Math::Lerp(u0, u3, (mSlices.left - d)*invTexSrcSize.x);
            u2 = Math::Lerp(u0, u3, (texSrcSize.x - mSlices.right + d)*invTexSrcSize.x);
        }

        if (mSlices.top + mSlices.bottom > sz.y)
        {
            float d = (mSlices.top + mSlices.bottom - sz.y)*0.5f;
            t1 -= yv*d;
            t2 += yv*d;

            v1 = Math::Lerp(v0, v3, (mSlices.bottom - d)*invTexSrcSize.y);
            v2 = Math::Lerp(v0, v3, (texSrcSize.y - mSlices.top + d)*invTexSrcSize.y);
        }

        mMesh.vertices[0].Set(o      + t3, rcc[0], u0, v3);
        mMesh.vertices[1].Set(o + r1 + t3, rcc[0], u1, v3);
        mMesh.vertices[2].Set(o + r2 + t3, rcc[1], u2, v3);
        mMesh.vertices[3].Set(o + r3 + t3, rcc[1], u3, v3);

        mMesh.vertices[4].Set(o      + t2, rcc[0], u0, v2);
        mMesh.vertices[5].Set(o + r1 + t2, rcc[0], u1, v2);
        mMesh.vertices[6].Set(o + r2 + t2, rcc[1], u2, v2);
        mMesh.vertices[7].Set(o + r3 + t2, rcc[1], u3, v2);

        mMesh.vertices[8].Set(o      + t1, rcc[3], u0, v1);
        mMesh.vertices[9].Set(o + r1 + t1, rcc[3], u1, v1);
        mMesh.vertices[10].Set(o + r2 + t1, rcc[2], u2, v1);
        mMesh.vertices[11].Set(o + r3 + t1, rcc[2], u3, v1);

        mMesh.vertices[12].Set(o + Vec2F(), rcc[3], u0, v0);
        mMesh.vertices[13].Set(o      + r1, rcc[3], u1, v0);
        mMesh.vertices[14].Set(o      + r2, rcc[2], u2, v0);
        mMesh.vertices[15].Set(o      + r3, rcc[2], u3, v0);

        memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*18*3);

        mMesh.vertexCount = 16;
        mMesh.polyCount = 18;

        mTransform.xv = lastTransformXv;
        mSize.x = lastSizeX;
    }

    void Sprite::BuildTiledMesh()
    {
        Vec2F invTexSize(1.0f, 1.0f);
        if (mMesh.mTexture)
            invTexSize.Set(1.0f/mMesh.mTexture->GetSize().x, 1.0f/mMesh.mTexture->GetSize().y);

        ULong rcc[4];
        for (int i = 0; i < 4; i++)
            rcc[i] = (mColor*mCornersColors[i]).ABGR();

        float uvLeft = mTextureSrcRect.left*invTexSize.x;
        float uvRight = mTextureSrcRect.right*invTexSize.x;

        float uvUp = 1.0f - mTextureSrcRect.bottom*invTexSize.y;
        float uvDown = 1.0f - mTextureSrcRect.top*invTexSize.y;

        Vec2F tileSize = (Vec2F)(mTextureSrcRect.Size())*mTileScale;
        Vec2F sz = mSize*mScale;

        Vec2I tilesCount(Math::CeilToInt(sz.x/tileSize.x), Math::CeilToInt(sz.y/tileSize.y));

        UInt requiredPolygons = tilesCount.x*tilesCount.y*2;
        UInt requiredVecticiesCount = requiredPolygons*2;

        if (mMesh.GetMaxVertexCount() < requiredVecticiesCount || mMesh.GetMaxPolyCount() < requiredPolygons)
            mMesh.Resize(requiredVecticiesCount, requiredPolygons);

        Vec2F xv = mTransform.xv/sz.x;
        Vec2F yv = mTransform.yv/sz.y;

        Vec2F o = mTransform.origin;

        int vi = 0, pi = 0;

        float px0 = 0;
        for (int x = 0; x < tilesCount.x; x++)
        {
            float px = (float)(x + 1)*tileSize.x;
            float u = uvRight;

            if (px > sz.x)
            {
                u = Math::Lerp(uvRight, uvLeft, (px - sz.x)/tileSize.x);
                px = sz.x;
            }

            float py0 = 0;
            for (int y = 0; y < tilesCount.y; y++)
            {
                float py = (float)(y + 1)*tileSize.y;
                float v = uvUp;

                if (py > sz.y)
                {
                    v = Math::Lerp(uvUp, uvDown, (py - sz.y)/tileSize.y);
                    py = sz.y;
                }

                int vii = vi;
                mMesh.vertices[vi++].Set(o + xv*px0 + yv*py, rcc[0], uvLeft, v);
                mMesh.vertices[vi++].Set(o + xv*px  + yv*py, rcc[1], u, v);
                mMesh.vertices[vi++].Set(o + xv*px  + yv*py0, rcc[2], u, uvDown);
                mMesh.vertices[vi++].Set(o + xv*px0 + yv*py0, rcc[3], uvLeft, uvDown);

                mMesh.indexes[pi++] = vii; mMesh.indexes[pi++] = vii + 1; mMesh.indexes[pi++] = vii + 2;
                mMesh.indexes[pi++] = vii; mMesh.indexes[pi++] = vii + 2; mMesh.indexes[pi++] = vii + 3;

                py0 = py;
            }

            px0 = px;
        }

        mMesh.vertexCount = vi;
        mMesh.polyCount = pi/3;
    }

    void Sprite::BuildFixedAspectMesh()
    {
        Vec2F invTexSize(1.0f, 1.0f);
        Vec2F texSize(1.0f, 1.0f);

        if (mMesh.mTexture)
        {
            texSize = mMesh.mTexture->GetSize();
            invTexSize.Set(1.0f/texSize.x, 1.0f/texSize.y);
        }

        ULong rcc[4];
        for (int i = 0; i < 4; i++)
            rcc[i] = (mColor*mCornersColors[i]).ABGR();

        static VertexIndex indexes[] = { 0, 1, 2, 0, 2, 3 };

        float uvLeft = mTextureSrcRect.left*invTexSize.x;
        float uvRight = mTextureSrcRect.right*invTexSize.x;

        float uvUp = 1.0f - mTextureSrcRect.bottom*invTexSize.y;
        float uvDown = 1.0f - mTextureSrcRect.top*invTexSize.y;

        Vec2 srcRectSize = mTextureSrcRect.Size();
        float fy = mSize.x/srcRectSize.x*srcRectSize.y;
        if (fy > mSize.y)
        {
            float fx = mSize.y/srcRectSize.y*srcRectSize.x;
            float off = (mSize.x - fx)*0.5f;
            Vec2F offx = mNonSizedTransform.xv*off;

            mMesh.vertices[0].Set(mTransform.origin + mTransform.yv + offx, rcc[0], uvLeft, uvUp);
            mMesh.vertices[1].Set(mTransform.origin + mTransform.yv + mTransform.xv - offx, rcc[1], uvRight, uvUp);
            mMesh.vertices[2].Set(mTransform.origin + mTransform.xv - offx, rcc[2], uvRight, uvDown);
            mMesh.vertices[3].Set(mTransform.origin + offx, rcc[3], uvLeft, uvDown);
        }
        else
        {
            float off = (mSize.y - fy)*0.5f;
            Vec2F offy = mNonSizedTransform.yv*off;

            mMesh.vertices[0].Set(mTransform.origin + mTransform.yv - offy, rcc[0], uvLeft, uvUp);
            mMesh.vertices[1].Set(mTransform.origin + mTransform.yv + mTransform.xv - offy, rcc[1], uvRight, uvUp);
            mMesh.vertices[2].Set(mTransform.origin + mTransform.xv + offy, rcc[2], uvRight, uvDown);
            mMesh.vertices[3].Set(mTransform.origin + offy, rcc[3], uvLeft, uvDown);
        }

        memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*6);

        mMesh.vertexCount = 4;
        mMesh.polyCount = 2;
    }

    void Sprite::BuildFillLeftToRightMesh()
    {
        float coef = Math::Clamp01(mFill);

        Vec2F invTexSize(1.0f, 1.0f);
        if (mMesh.mTexture)
            invTexSize.Set(1.0f/mMesh.mTexture->GetSize().x, 1.0f/mMesh.mTexture->GetSize().y);

        ULong rcc[4];
        rcc[0] = (mColor*mCornersColors[0]).ABGR();
        rcc[1] = (mColor*Math::Lerp(mCornersColors[0], mCornersColors[1], coef)).ABGR();
        rcc[2] = (mColor*Math::Lerp(mCornersColors[3], mCornersColors[2], coef)).ABGR();
        rcc[3] = (mColor*mCornersColors[3]).ABGR();

        static VertexIndex indexes[] ={ 0, 1, 2, 0, 2, 3 };

        float uvLeft = mTextureSrcRect.left*invTexSize.x;
        float uvRight = Math::Lerp((float)mTextureSrcRect.left, (float)mTextureSrcRect.right, coef)*invTexSize.x;

        float uvUp = 1.0f - mTextureSrcRect.bottom*invTexSize.y;
        float uvDown = 1.0f - mTextureSrcRect.top*invTexSize.y;

        mMesh.vertices[0].Set(mTransform.origin + mTransform.yv, rcc[0], uvLeft, uvUp);
        mMesh.vertices[1].Set(mTransform.origin + mTransform.yv + mTransform.xv*coef, rcc[1], uvRight, uvUp);
        mMesh.vertices[2].Set(mTransform.origin + mTransform.xv*coef, rcc[2], uvRight, uvDown);
        mMesh.vertices[3].Set(mTransform.origin, rcc[3], uvLeft, uvDown);

        memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*6);

        mMesh.vertexCount = 4;
        mMesh.polyCount = 2;
    }

    void Sprite::BuildFillRightToLeftMesh()
    {
        float coef = Math::Clamp01(mFill);
        float invCoef = 1.0f - coef;

        Vec2F invTexSize(1.0f, 1.0f);
        if (mMesh.mTexture)
            invTexSize.Set(1.0f/mMesh.mTexture->GetSize().x, 1.0f/mMesh.mTexture->GetSize().y);

        ULong rcc[4];
        rcc[0] = (mColor*Math::Lerp(mCornersColors[1], mCornersColors[0], coef)).ABGR();
        rcc[1] = (mColor*mCornersColors[1]).ABGR();
        rcc[2] = (mColor*mCornersColors[2]).ABGR();
        rcc[3] = (mColor*Math::Lerp(mCornersColors[2], mCornersColors[3], coef)).ABGR();

        static VertexIndex indexes[] ={ 0, 1, 2, 0, 2, 3 };

        float uvLeft = Math::Lerp((float)mTextureSrcRect.right, (float)mTextureSrcRect.left, coef)*invTexSize.x;
        float uvRight = mTextureSrcRect.right*invTexSize.x;

        float uvUp = 1.0f - mTextureSrcRect.bottom*invTexSize.y;
        float uvDown = 1.0f - mTextureSrcRect.top*invTexSize.y;

        mMesh.vertices[0].Set(mTransform.origin + mTransform.yv + mTransform.xv*invCoef, rcc[0], uvLeft, uvUp);
        mMesh.vertices[1].Set(mTransform.origin + mTransform.yv + mTransform.xv, rcc[1], uvRight, uvUp);
        mMesh.vertices[2].Set(mTransform.origin + mTransform.xv, rcc[2], uvRight, uvDown);
        mMesh.vertices[3].Set(mTransform.origin + mTransform.xv*invCoef, rcc[3], uvLeft, uvDown);

        memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*6);

        mMesh.vertexCount = 4;
        mMesh.polyCount = 2;
    }

    void Sprite::BuildFillUpToDownMesh()
    {
        float coef = Math::Clamp01(mFill);
        float invCoef = 1.0f - coef;

        Vec2F invTexSize(1.0f, 1.0f);
        if (mMesh.mTexture)
            invTexSize.Set(1.0f/mMesh.mTexture->GetSize().x, 1.0f/mMesh.mTexture->GetSize().y);

        ULong rcc[4];
        rcc[0] = (mColor*mCornersColors[0]).ABGR();
        rcc[1] = (mColor*mCornersColors[1]).ABGR();
        rcc[2] = (mColor*Math::Lerp(mCornersColors[1], mCornersColors[2], coef)).ABGR();
        rcc[3] = (mColor*Math::Lerp(mCornersColors[0], mCornersColors[3], coef)).ABGR();

        static VertexIndex indexes[] ={ 0, 1, 2, 0, 2, 3 };

        float uvLeft = mTextureSrcRect.left*invTexSize.x;
        float uvRight = mTextureSrcRect.right*invTexSize.x;

        float uvUp = 1.0f - mTextureSrcRect.bottom*invTexSize.y;
        float uvDown = 1.0f - Math::Lerp((float)mTextureSrcRect.bottom, (float)mTextureSrcRect.top, coef)*invTexSize.y;

        mMesh.vertices[0].Set(mTransform.origin + mTransform.yv, rcc[0], uvLeft, uvUp);
        mMesh.vertices[1].Set(mTransform.origin + mTransform.yv + mTransform.xv, rcc[1], uvRight, uvUp);
        mMesh.vertices[2].Set(mTransform.origin + mTransform.xv + mTransform.yv*invCoef, rcc[2], uvRight, uvDown);
        mMesh.vertices[3].Set(mTransform.origin + mTransform.yv*invCoef, rcc[3], uvLeft, uvDown);

        memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*6);

        mMesh.vertexCount = 4;
        mMesh.polyCount = 2;
    }

    void Sprite::BuildFillDownToUpMesh()
    {
        float coef = Math::Clamp01(mFill);

        Vec2F invTexSize(1.0f, 1.0f);
        if (mMesh.mTexture)
            invTexSize.Set(1.0f/mMesh.mTexture->GetSize().x, 1.0f/mMesh.mTexture->GetSize().y);

        ULong rcc[4];
        rcc[0] = (mColor*Math::Lerp(mCornersColors[3], mCornersColors[0], coef)).ABGR();
        rcc[1] = (mColor*Math::Lerp(mCornersColors[2], mCornersColors[1], coef)).ABGR();
        rcc[2] = (mColor*mCornersColors[2]).ABGR();
        rcc[3] = (mColor*mCornersColors[3]).ABGR();

        static VertexIndex indexes[] ={ 0, 1, 2, 0, 2, 3 };

        float uvLeft = mTextureSrcRect.left*invTexSize.x;
        float uvRight = mTextureSrcRect.right*invTexSize.x;

        float uvUp = 1.0f - Math::Lerp((float)mTextureSrcRect.top, (float)mTextureSrcRect.bottom, coef)*invTexSize.y;
        float uvDown = 1.0f - mTextureSrcRect.top*invTexSize.y;

        mMesh.vertices[0].Set(mTransform.origin + mTransform.yv*coef, rcc[0], uvLeft, uvUp);
        mMesh.vertices[1].Set(mTransform.origin + mTransform.yv*coef + mTransform.xv, rcc[1], uvRight, uvUp);
        mMesh.vertices[2].Set(mTransform.origin + mTransform.xv, rcc[2], uvRight, uvDown);
        mMesh.vertices[3].Set(mTransform.origin, rcc[3], uvLeft, uvDown);

        memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*6);

        mMesh.vertexCount = 4;
        mMesh.polyCount = 2;
    }

    void Sprite::BuildFill360CWMesh()
    {
        float coef = Math::Clamp01(mFill);
        float angle = 360.0f*coef;

        Vec2F invTexSize(1.0f, 1.0f);
        if (mMesh.mTexture)
            invTexSize.Set(1.0f/mMesh.mTexture->GetSize().x, 1.0f/mMesh.mTexture->GetSize().y);

        ULong cornerResColr[4];
        for (int i = 0; i < 4; i++)
            cornerResColr[i] = (mColor*mCornersColors[i]).ABGR();

        float uLeft = mTextureSrcRect.left*invTexSize.x;
        float uRight = mTextureSrcRect.right*invTexSize.x;

        float vUp = 1.0f - mTextureSrcRect.bottom*invTexSize.y;
        float vDown = 1.0f - mTextureSrcRect.top*invTexSize.y;

        Vec2F zeroPos    = mTransform.origin + mTransform.xv*0.5f + mTransform.yv;
        Vec2F centerPos  = mTransform.origin + mTransform.xv*0.5f + mTransform.yv*0.5f;

        ULong centerResColr = (mColor*((mCornersColors[0] + mCornersColors[1] + mCornersColors[2] + mCornersColors[3])*0.25f)).ABGR();
        ULong zeroResColor = (mColor*((mCornersColors[0] + mCornersColors[1])*0.5f)).ABGR();
        float uZero = (uLeft + uRight)*0.5f;
        float uCenter = uZero;
        float vCenter = (vUp + vDown)*0.5f;

        Vec2F dir = Vec2F::Rotated(Math::Deg2rad(-angle + 90.0f));
        if (angle < 45.0f)
        {
            float dirCoef = 0.5f + dir.x/dir.y*0.5f;
            Vec2F dirPoint = mTransform.origin + mTransform.xv*dirCoef + mTransform.yv;
            ULong dirColor = (mColor*Math::Lerp(mCornersColors[0], mCornersColors[1], dirCoef)).ABGR();
            float uDir = Math::Lerp(uLeft, uRight, dirCoef);

            mMesh.vertices[0].Set(zeroPos, zeroResColor, uZero, vUp);
            mMesh.vertices[1].Set(dirPoint, dirColor, uDir, vUp);
            mMesh.vertices[2].Set(centerPos, centerResColr, uCenter, vCenter);

            static VertexIndex indexes[] ={ 0, 1, 2 };
            memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*3);

            mMesh.vertexCount = 3;
            mMesh.polyCount = 1;
        }
        else if (angle < 135.0f)
        {
            float dirCoef = 0.5f + dir.y/dir.x*0.5f;
            Vec2F dirPoint = mTransform.origin + mTransform.xv + mTransform.yv*dirCoef;
            ULong dirColor = (mColor*Math::Lerp(mCornersColors[2], mCornersColors[1], dirCoef)).ABGR();
            float vDir = Math::Lerp(vDown, vUp, dirCoef);

            Vec2F cornerPos1 = mTransform.origin + mTransform.yv + mTransform.xv;

            mMesh.vertices[0].Set(zeroPos, zeroResColor, uZero, vUp);
            mMesh.vertices[1].Set(cornerPos1, cornerResColr[1], uRight, vUp);
            mMesh.vertices[2].Set(dirPoint, dirColor, uRight, vDir);
            mMesh.vertices[3].Set(centerPos, centerResColr, uCenter, vCenter);

            static VertexIndex indexes[] ={ 0, 1, 3, 1, 2, 3 };
            memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*3*2);

            mMesh.vertexCount = 4;
            mMesh.polyCount = 2;
        }
        else if (angle < 225.0f)
        {
            float dirCoef = 0.5f - dir.x/dir.y*0.5f;
            Vec2F dirPoint = mTransform.origin + mTransform.xv*dirCoef;
            ULong dirColor = (mColor*Math::Lerp(mCornersColors[3], mCornersColors[2], dirCoef)).ABGR();
            float uDir = Math::Lerp(uLeft, uRight, dirCoef);

            Vec2F cornerPos1 = mTransform.origin + mTransform.yv + mTransform.xv;
            Vec2F cornerPos2 = mTransform.origin + mTransform.xv;

            mMesh.vertices[0].Set(zeroPos, zeroResColor, uZero, vUp);
            mMesh.vertices[1].Set(cornerPos1, cornerResColr[1], uRight, vUp);
            mMesh.vertices[2].Set(cornerPos2, cornerResColr[2], uRight, vDown);
            mMesh.vertices[3].Set(dirPoint, dirColor, uDir, vDown);
            mMesh.vertices[4].Set(centerPos, centerResColr, uCenter, vCenter);

            static VertexIndex indexes[] ={ 0, 1, 4, 1, 2, 4, 2, 3, 4 };
            memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*3*3);

            mMesh.vertexCount = 5;
            mMesh.polyCount = 3;
        }
        else if (angle < 315.0f)
        {
            float dirCoef = 0.5f - dir.y/dir.x*0.5f;
            Vec2F dirPoint = mTransform.origin + mTransform.yv*dirCoef;
            ULong dirColor = (mColor*Math::Lerp(mCornersColors[3], mCornersColors[0], dirCoef)).ABGR();
            float vDir = Math::Lerp(vDown, vUp, dirCoef);

            Vec2F cornerPos1 = mTransform.origin + mTransform.yv + mTransform.xv;
            Vec2F cornerPos2 = mTransform.origin + mTransform.xv;
            Vec2F cornerPos3 = mTransform.origin;

            mMesh.vertices[0].Set(zeroPos, zeroResColor, uZero, vUp);
            mMesh.vertices[1].Set(cornerPos1, cornerResColr[1], uRight, vUp);
            mMesh.vertices[2].Set(cornerPos2, cornerResColr[2], uRight, vDown);
            mMesh.vertices[3].Set(cornerPos3, cornerResColr[3], uLeft, vDown);
            mMesh.vertices[4].Set(dirPoint, dirColor, uLeft, vDir);
            mMesh.vertices[5].Set(centerPos, centerResColr, uCenter, vCenter);

            static VertexIndex indexes[] ={ 0, 1, 5, 1, 2, 5, 2, 3, 5, 3, 4, 5 };
            memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*3*4);

            mMesh.vertexCount = 6;
            mMesh.polyCount = 4;
        }
        else
        {
            float dirCoef = 0.5f + dir.x/dir.y*0.5f;
            Vec2F dirPoint = mTransform.origin + mTransform.xv*dirCoef + mTransform.yv;
            ULong dirColor = (mColor*Math::Lerp(mCornersColors[0], mCornersColors[1], dirCoef)).ABGR();
            float uDir = Math::Lerp(uLeft, uRight, dirCoef);

            Vec2F cornerPos0 = mTransform.origin + mTransform.yv;
            Vec2F cornerPos1 = mTransform.origin + mTransform.yv + mTransform.xv;
            Vec2F cornerPos2 = mTransform.origin + mTransform.xv;
            Vec2F cornerPos3 = mTransform.origin;

            mMesh.vertices[0].Set(zeroPos, zeroResColor, uZero, vUp);
            mMesh.vertices[1].Set(cornerPos1, cornerResColr[0], uRight, vUp);
            mMesh.vertices[2].Set(cornerPos2, cornerResColr[1], uRight, vDown);
            mMesh.vertices[3].Set(cornerPos3, cornerResColr[2], uLeft, vDown);
            mMesh.vertices[4].Set(cornerPos0, cornerResColr[3], uLeft, vUp);
            mMesh.vertices[5].Set(dirPoint, dirColor, uDir, vUp);
            mMesh.vertices[6].Set(centerPos, centerResColr, uCenter, vCenter);

            static VertexIndex indexes[] ={ 0, 1, 6, 1, 2, 6, 2, 3, 6, 3, 4, 6, 4, 5, 6 };
            memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*3*5);

            mMesh.vertexCount = 7;
            mMesh.polyCount = 5;
        }
    }

    void Sprite::BuildFill360CCWMesh()
    {
        float coef = Math::Clamp01(mFill);
        float angle = 360.0f*coef;

        Vec2F invTexSize(1.0f, 1.0f);
        if (mMesh.mTexture)
            invTexSize.Set(1.0f/mMesh.mTexture->GetSize().x, 1.0f/mMesh.mTexture->GetSize().y);

        ULong cornerResColr[4];
        for (int i = 0; i < 4; i++)
            cornerResColr[i] = (mColor*mCornersColors[i]).ABGR();

        float uLeft = mTextureSrcRect.left*invTexSize.x;
        float uRight = mTextureSrcRect.right*invTexSize.x;

        float vUp = 1.0f - mTextureSrcRect.bottom*invTexSize.y;
        float vDown = 1.0f - mTextureSrcRect.top*invTexSize.y;

        Vec2F zeroPos   = mTransform.origin + mTransform.xv*0.5f + mTransform.yv;
        Vec2F centerPos = mTransform.origin + mTransform.xv*0.5f + mTransform.yv*0.5f;

        ULong centerResColr = (mColor*((mCornersColors[0] + mCornersColors[1] + mCornersColors[2] + mCornersColors[3])*0.25f)).ABGR();
        ULong zeroResColor = (mColor*((mCornersColors[0] + mCornersColors[1])*0.5f)).ABGR();
        float uZero = (uLeft + uRight)*0.5f;
        float uCenter = uZero;
        float vCenter = (vUp + vDown)*0.5f;

        Vec2F dir = Vec2F::Rotated(Math::Deg2rad(angle + 90.0f));
        if (angle < 45.0f)
        {
            float dirCoef = 0.5f + dir.x/dir.y*0.5f;
            Vec2F dirPoint = mTransform.origin + mTransform.xv*dirCoef + mTransform.yv;
            ULong dirColor = (mColor*Math::Lerp(mCornersColors[0], mCornersColors[1], dirCoef)).ABGR();
            float uDir = Math::Lerp(uLeft, uRight, dirCoef);

            mMesh.vertices[0].Set(zeroPos, zeroResColor, uZero, vUp);
            mMesh.vertices[1].Set(dirPoint, dirColor, uDir, vUp);
            mMesh.vertices[2].Set(centerPos, centerResColr, uCenter, vCenter);

            static VertexIndex indexes[] ={ 1, 0, 2 };
            memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*3);

            mMesh.vertexCount = 3;
            mMesh.polyCount = 1;
        }
        else if (angle < 135.0f)
        {
            float dirCoef = 0.5f - dir.y/dir.x*0.5f;
            Vec2F dirPoint = mTransform.origin + mTransform.yv*dirCoef;
            ULong dirColor = (mColor*Math::Lerp(mCornersColors[2], mCornersColors[1], dirCoef)).ABGR();
            float vDir = Math::Lerp(vDown, vUp, dirCoef);

            Vec2F cornerPos0 = mTransform.origin + mTransform.yv;

            mMesh.vertices[0].Set(zeroPos, zeroResColor, uZero, vUp);
            mMesh.vertices[1].Set(cornerPos0, cornerResColr[0], uLeft, vUp);
            mMesh.vertices[2].Set(dirPoint, dirColor, uLeft, vDir);
            mMesh.vertices[3].Set(centerPos, centerResColr, uCenter, vCenter);

            static VertexIndex indexes[] ={ 1, 0, 3, 2, 1, 3 };
            memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*3*2);

            mMesh.vertexCount = 4;
            mMesh.polyCount = 2;
        }
        else if (angle < 225.0f)
        {
            float dirCoef = 0.5f - dir.x/dir.y*0.5f;
            Vec2F dirPoint = mTransform.origin + mTransform.xv*dirCoef;
            ULong dirColor = (mColor*Math::Lerp(mCornersColors[3], mCornersColors[2], dirCoef)).ABGR();
            float uDir = Math::Lerp(uLeft, uRight, dirCoef);

            Vec2F cornerPos0 = mTransform.origin + mTransform.yv;
            Vec2F cornerPos3 = mTransform.origin;

            mMesh.vertices[0].Set(zeroPos, zeroResColor, uZero, vUp);
            mMesh.vertices[1].Set(cornerPos0, cornerResColr[0], uLeft, vUp);
            mMesh.vertices[2].Set(cornerPos3, cornerResColr[3], uLeft, vDown);
            mMesh.vertices[3].Set(dirPoint, dirColor, uDir, vDown);
            mMesh.vertices[4].Set(centerPos, centerResColr, uCenter, vCenter);

            static VertexIndex indexes[] ={ 1, 0, 4, 2, 1, 4, 3, 2, 4 };
            memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*3*3);

            mMesh.vertexCount = 5;
            mMesh.polyCount = 3;
        }
        else if (angle < 315.0f)
        {
            float dirCoef = 0.5f + dir.y/dir.x*0.5f;
            Vec2F dirPoint = mTransform.origin + mTransform.yv*dirCoef + mTransform.xv;
            ULong dirColor = (mColor*Math::Lerp(mCornersColors[3], mCornersColors[0], dirCoef)).ABGR();
            float vDir = Math::Lerp(vDown, vUp, dirCoef);

            Vec2F cornerPos0 = mTransform.origin + mTransform.yv;
            Vec2F cornerPos3 = mTransform.origin;
            Vec2F cornerPos2 = mTransform.origin + mTransform.xv;

            mMesh.vertices[0].Set(zeroPos, zeroResColor, uZero, vUp);
            mMesh.vertices[1].Set(cornerPos0, cornerResColr[0], uLeft, vUp);
            mMesh.vertices[2].Set(cornerPos3, cornerResColr[3], uLeft, vDown);
            mMesh.vertices[3].Set(cornerPos2, cornerResColr[2], uRight, vDown);
            mMesh.vertices[4].Set(dirPoint, dirColor, uRight, vDir);
            mMesh.vertices[5].Set(centerPos, centerResColr, uCenter, vCenter);

            static VertexIndex indexes[] ={ 1, 0, 5, 2, 1, 5, 3, 2, 5, 4, 3, 5 };
            memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*3*4);

            mMesh.vertexCount = 6;
            mMesh.polyCount = 4;
        }
        else
        {
            float dirCoef = 0.5f + dir.x/dir.y*0.5f;
            Vec2F dirPoint = mTransform.origin + mTransform.xv*dirCoef + mTransform.yv;
            ULong dirColor = (mColor*Math::Lerp(mCornersColors[0], mCornersColors[1], dirCoef)).ABGR();
            float uDir = Math::Lerp(uLeft, uRight, dirCoef);

            Vec2F cornerPos0 = mTransform.origin + mTransform.yv;
            Vec2F cornerPos3 = mTransform.origin;
            Vec2F cornerPos2 = mTransform.origin + mTransform.xv;
            Vec2F cornerPos1 = mTransform.origin + mTransform.yv + mTransform.xv;

            mMesh.vertices[0].Set(zeroPos, zeroResColor, uZero, vUp);
            mMesh.vertices[1].Set(cornerPos0, cornerResColr[0], uLeft, vUp);
            mMesh.vertices[2].Set(cornerPos3, cornerResColr[3], uLeft, vDown);
            mMesh.vertices[3].Set(cornerPos2, cornerResColr[2], uRight, vDown);
            mMesh.vertices[4].Set(cornerPos1, cornerResColr[1], uRight, vUp);
            mMesh.vertices[5].Set(dirPoint, dirColor, uDir, vUp);
            mMesh.vertices[6].Set(centerPos, centerResColr, uCenter, vCenter);

            static VertexIndex indexes[] ={ 1, 0, 6, 2, 1, 6, 3, 2, 6, 4, 3, 6, 5, 4, 6 };
            memcpy(mMesh.indexes, indexes, sizeof(VertexIndex)*3*5);

            mMesh.vertexCount = 7;
            mMesh.polyCount = 5;
        }
    }

    void Sprite::OnSerialize(DataValue& node) const
    {
        if (!mImageAsset)
        {
            node["mTextureSrcRect"] = mTextureSrcRect;

            if (mMesh.GetTexture())
                node["textureFileName"] = mMesh.GetTexture()->GetFileName();
        }
    }

    void Sprite::OnDeserialized(const DataValue& node)
    {
        if (mImageAsset)
            InitializeTexture();

        SpriteMode mode = mMode;
        mMode = (SpriteMode)((int)mode + 1);
        SetMode(mode);
    }

    void Sprite::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        OnDeserialized(node);
    }

    void Sprite::ReloadImage()
    {
        if (mImageAsset)
        {
            InitializeTexture();
            UpdateMesh();
        }
    }

    void Sprite::InitializeTexture()
    {
        auto atlasSpriteSource = mImageAsset->GetTextureSource();
        mMesh.mTexture = atlasSpriteSource.texture;
        mTextureSrcRect = atlasSpriteSource.sourceRect;
    }
}
// --- META ---

DECLARE_CLASS(o2::Sprite, o2__Sprite);
// --- END META ---
