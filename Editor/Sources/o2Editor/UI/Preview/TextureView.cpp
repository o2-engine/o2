#include "o2Editor/stdafx.h"
#include "TextureView.h"

#include "o2/Application/Input.h"
#include "o2/Assets/Assets.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"

namespace Editor
{
    TextureView::TextureView(RefCounter* refCounter):
        ScrollView(refCounter)
    {
        mTextureSprite = mmake<Sprite>();

        horGridEnabled = false;
        verGridEnabled = false;
    }

    TextureView::TextureView(RefCounter* refCounter, const TextureView& other):
        ScrollView(refCounter, other), mChannel(other.mChannel), mBackground(other.mBackground)
    {
        mTextureSprite = mmake<Sprite>();

        horGridEnabled = false;
        verGridEnabled = false;

        SetTexture(other.mTexture, other.mSourceRect);
    }

    TextureView& TextureView::operator=(const TextureView& other)
    {
        ScrollView::operator=(other);

        mChannel = other.mChannel;
        mBackground = other.mBackground;
        SetTexture(other.mTexture, other.mSourceRect);

        return *this;
    }

    void TextureView::SetTexture(const TextureRef& texture, const RectI& sourceRect /*= RectI()*/)
    {
        mTexture = texture;
        mSourceRect = sourceRect == RectI() && texture ? RectI(Vec2I(), texture->GetSize()) : sourceRect;
        mHighlights.Clear();
        mHovered = false;

        ReadPixels();
        UpdateShownTexture();

        mFitPending = true;
        FitToView();
    }

    const TextureRef& TextureView::GetTexture() const
    {
        return mTexture;
    }

    const RectI& TextureView::GetSourceRect() const
    {
        return mSourceRect;
    }

    void TextureView::Reread()
    {
        ReadPixels();
        UpdateShownTexture();
    }

    void TextureView::SetChannel(Channel channel)
    {
        if (mChannel == channel)
            return;

        mChannel = channel;
        UpdateShownTexture();
    }

    TextureView::Channel TextureView::GetChannel() const
    {
        return mChannel;
    }

    void TextureView::SetBackground(Background background)
    {
        mBackground = background;
    }

    TextureView::Background TextureView::GetBackground() const
    {
        return mBackground;
    }

    void TextureView::SetHighlights(const Vector<RectI>& rects)
    {
        mHighlights = rects;
    }

    const Vector<RectI>& TextureView::GetHighlights() const
    {
        return mHighlights;
    }

    void TextureView::FitToView()
    {
        Vec2F viewSize = layout->GetWorldRect().Size();
        Vec2F size = (Vec2F)GetSourceSize();
        if (viewSize.x < 1.0f || viewSize.y < 1.0f || size.x < 1.0f || size.y < 1.0f)
            return;

        mFitPending = false;

        float scale = Math::Max(size.x/(viewSize.x*0.9f), size.y/(viewSize.y*0.9f));
        Camera camera = mViewCamera;
        camera.SetPosition2D(size*0.5f);
        camera.SetScale2D(Vec2F(scale, scale));
        SetCamera(camera);
    }

    void TextureView::SetPixelZoom(float zoom)
    {
        if (zoom <= 0.0f)
            return;

        Camera camera = mViewCamera;
        camera.SetPosition2D(mViewCameraTargetPos);
        camera.SetScale2D(Vec2F(1.0f/zoom, 1.0f/zoom));
        SetCamera(camera);
    }

    float TextureView::GetPixelZoom() const
    {
        Vec2F scale = mViewCamera.GetScale2D();
        return scale.x > 0.0f ? 1.0f/scale.x : 1.0f;
    }

    bool TextureView::IsReadable() const
    {
        return mPixels != nullptr;
    }

    bool TextureView::GetHoveredPixel(Vec2I& pixel, Color4& color) const
    {
        if (!mHovered)
            return false;

        pixel = mHoveredPixel;
        color = GetPixel(pixel);
        return true;
    }

    Color4 TextureView::GetPixel(const Vec2I& pixel) const
    {
        if (!mPixels)
            return Color4(0, 0, 0, 0);

        Vec2I size = mPixels->GetSize();
        if (pixel.x < 0 || pixel.y < 0 || pixel.x >= size.x || pixel.y >= size.y)
            return Color4(0, 0, 0, 0);

        // A bitmap keeps its rows from the bottom
        const UInt8* data = mPixels->GetData() + ((size_t)(size.y - 1 - pixel.y)*size.x + pixel.x)*4;
        return Color4((int)data[0], (int)data[1], (int)data[2], (int)data[3]);
    }

    Ref<Bitmap> TextureView::MakeChannelBitmap(const Bitmap& source, Channel channel)
    {
        auto result = mmake<Bitmap>(PixelFormat::R8G8B8A8, source.GetSize());
        Vec2I size = source.GetSize();
        size_t count = (size_t)size.x*size.y;
        const UInt8* from = source.GetData();
        UInt8* to = result->GetData();

        for (size_t i = 0; i < count; i++, from += 4, to += 4)
        {
            switch (channel)
            {
            case Channel::RGBA:  to[0] = from[0]; to[1] = from[1]; to[2] = from[2]; to[3] = from[3]; break;
            case Channel::RGB:   to[0] = from[0]; to[1] = from[1]; to[2] = from[2]; to[3] = 255; break;
            case Channel::Red:   to[0] = to[1] = to[2] = from[0]; to[3] = 255; break;
            case Channel::Green: to[0] = to[1] = to[2] = from[1]; to[3] = 255; break;
            case Channel::Blue:  to[0] = to[1] = to[2] = from[2]; to[3] = 255; break;
            case Channel::Alpha: to[0] = to[1] = to[2] = from[3]; to[3] = 255; break;
            }
        }

        return result;
    }

    String TextureView::GetChannelName(Channel channel)
    {
        switch (channel)
        {
        case Channel::RGBA:  return "RGBA";
        case Channel::RGB:   return "RGB";
        case Channel::Red:   return "Red";
        case Channel::Green: return "Green";
        case Channel::Blue:  return "Blue";
        case Channel::Alpha: return "Alpha";
        }

        return "";
    }

    namespace
    {
        // A built file goes by its path in the assets of the platform, the words before the path stay
        String ShortenBuiltPath(const String& text)
        {
            int built = text.Find("BuiltAssets/");
            if (built < 0)
                return text;

            int pathStart = 0;
            for (int i = built - 1; i >= 0; i--)
            {
                if (text[i] == ' ')
                {
                    pathStart = i + 1;
                    break;
                }
            }

            String rest = text.SubStr(built + 12);
            int platformEnd = rest.Find("/");
            return text.SubStr(0, pathStart) + (platformEnd >= 0 ? rest.SubStr(platformEnd + 1) : rest);
        }
    }

    String TextureView::GetTextureName(const TextureRef& texture)
    {
        if (!texture)
            return "(none)";

        if (!texture->GetName().IsEmpty())
            return ShortenBuiltPath(texture->GetName());

        if (texture->IsAtlasPage())
        {
            String path = o2Assets.GetAssetPath(texture->GetAtlasAssetId());
            return (path.IsEmpty() ? String("Atlas") : path) + " page " + (String)texture->GetAtlasPage();
        }

        if (!texture->GetFileName().IsEmpty())
            return ShortenBuiltPath(texture->GetFileName());

        return texture->GetUsage() == Texture::Usage::RenderTarget ? "Render target" : "Texture from data";
    }

    UInt64 TextureView::GetTextureMemory(const TextureRef& texture)
    {
        if (!texture)
            return 0;

        Vec2I size = texture->GetSize();
        TextureFormat format = texture->GetFormat();
        if (Texture::IsFormatCompressed(format))
            return (UInt64)((size.x + 3)/4)*(UInt64)((size.y + 3)/4)*(UInt64)Texture::FormatBlockSize(format);

        int bytesPerPixel = format == TextureFormat::R16G16B16A16F ? 8 : 4;
        return (UInt64)size.x*(UInt64)size.y*(UInt64)bytesPerPixel;
    }

    String TextureView::GetFormatName(TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::R8G8B8A8:      return "RGBA8";
        case TextureFormat::DXT1:          return "DXT1";
        case TextureFormat::DXT5:          return "DXT5";
        case TextureFormat::BC7:           return "BC7";
        case TextureFormat::ASTC4x4:       return "ASTC 4x4";
        case TextureFormat::R16G16B16A16F: return "RGBA16F";
        }

        return "";
    }

    String TextureView::FormatBytes(UInt64 bytes)
    {
        if (bytes >= 1024*1024)
            return String::Format("%.1f MB", (double)bytes/(1024.0*1024.0));

        if (bytes >= 1024)
            return String::Format("%.1f KB", (double)bytes/1024.0);

        return (String)(int)bytes + " B";
    }

    namespace
    {
        // Rectangles of one color sent by one mesh
        struct QuadBatch
        {
            Mesh mesh = Mesh(TextureRef(), 4096, 2048);

            void Add(float x0, float y0, float x1, float y1, ULong color)
            {
                if (mesh.vertexCount + 4 > mesh.GetMaxVertexCount())
                    Flush();

                Vertex* vertices = mesh.GetVertices<Vertex>() + mesh.vertexCount;
                vertices[0].Set(Vec2F(x0, y1), color, 0.0f, 0.0f);
                vertices[1].Set(Vec2F(x1, y1), color, 1.0f, 0.0f);
                vertices[2].Set(Vec2F(x1, y0), color, 1.0f, 1.0f);
                vertices[3].Set(Vec2F(x0, y0), color, 0.0f, 1.0f);

                VertexIndex* indexes = mesh.GetIndexes() + mesh.polyCount*3;
                VertexIndex first = (VertexIndex)mesh.vertexCount;
                for (VertexIndex i : { 0, 1, 2, 0, 2, 3 })
                    *indexes++ = first + i;

                mesh.vertexCount += 4;
                mesh.polyCount += 2;
            }

            void Flush()
            {
                if (mesh.polyCount > 0)
                    mesh.Draw();

                mesh.vertexCount = 0;
                mesh.polyCount = 0;
            }
        };

        QuadBatch& Quads()
        {
            static QuadBatch batch;
            return batch;
        }
    }

    void TextureView::DrawChecker(const RectF& area, const RectF& visible, float cell)
    {
        float left = Math::Max(Math::Min(area.left, area.right), Math::Min(visible.left, visible.right));
        float right = Math::Min(Math::Max(area.left, area.right), Math::Max(visible.left, visible.right));
        float bottom = Math::Max(Math::Min(area.top, area.bottom), Math::Min(visible.top, visible.bottom));
        float top = Math::Min(Math::Max(area.top, area.bottom), Math::Max(visible.top, visible.bottom));
        if (left >= right || bottom >= top || cell <= 0.0f)
            return;

        auto& quads = Quads();
        quads.Add(left, bottom, right, top, Color4(255, 255, 255, 255).ABGR());

        // The cells keep their screen size at any zoom, counted from the area corner
        float originX = Math::Min(area.left, area.right), originY = Math::Min(area.top, area.bottom);
        int firstColumn = (int)Math::Floor((left - originX)/cell), lastColumn = (int)Math::Ceil((right - originX)/cell);
        int firstRow = (int)Math::Floor((bottom - originY)/cell), lastRow = (int)Math::Ceil((top - originY)/cell);

        ULong color = Color4(205, 205, 205, 255).ABGR();
        for (int row = firstRow; row < lastRow; row++)
        {
            for (int column = firstColumn; column < lastColumn; column++)
            {
                if (((row + column) & 1) == 0)
                    continue;

                quads.Add(Math::Max(left, originX + column*cell), Math::Max(bottom, originY + row*cell),
                          Math::Min(right, originX + (column + 1)*cell), Math::Min(top, originY + (row + 1)*cell), color);
            }
        }

        quads.Flush();
    }

    void TextureView::DrawFrame(const Vec2F& min, const Vec2F& max, float width, const Color4& color)
    {
        auto& quads = Quads();
        ULong abgr = color.ABGR();
        quads.Add(min.x, min.y, max.x, min.y + width, abgr);
        quads.Add(min.x, max.y - width, max.x, max.y, abgr);
        quads.Add(min.x, min.y + width, min.x + width, max.y - width, abgr);
        quads.Add(max.x - width, min.y + width, max.x, max.y - width, abgr);
        quads.Flush();
    }

    RectF TextureView::GetVisibleArea() const
    {
        RectF screen = layout->GetWorldRect();
        Vec2F a = screen.LeftBottom()*mScreenToLocalTransform, b = screen.RightTop()*mScreenToLocalTransform;
        return RectF(Math::Min(a.x, b.x), Math::Max(a.y, b.y), Math::Max(a.x, b.x), Math::Min(a.y, b.y));
    }

    void TextureView::RedrawContent()
    {
        Vec2F size = (Vec2F)GetSourceSize();
        if (!mTexture || size.x < 1.0f || size.y < 1.0f)
            return;

        Basis area(Vec2F(), Vec2F(size.x, 0.0f), Vec2F(0.0f, size.y));

        if (mBackground == Background::Checker)
            DrawChecker(RectF(0.0f, size.y, size.x, 0.0f), GetVisibleArea(), 8.0f/GetPixelZoom());
        else
        {
            Color4 color = mBackground == Background::Black ? Color4(0, 0, 0, 255) : Color4(255, 255, 255, 255);
            Quads().Add(0.0f, 0.0f, size.x, size.y, color.ABGR());
            Quads().Flush();
        }

        mTextureSprite->SetBasis(area);
        mTextureSprite->Draw();

        // Texels are squares big enough to see a grid between them
        float zoom = GetPixelZoom();
        if (zoom >= 12.0f)
        {
            RectF visible = GetVisibleArea();
            int left = Math::Max(0, (int)Math::Floor(visible.left)), right = Math::Min((int)size.x, (int)Math::Ceil(visible.right));
            int bottom = Math::Max(0, (int)Math::Floor(visible.bottom)), top = Math::Min((int)size.y, (int)Math::Ceil(visible.top));

            float line = 1.0f/zoom;
            ULong gridColor = Color4(40, 40, 40, 110).ABGR();
            auto& quads = Quads();
            for (int x = left; x <= right; x++)
                quads.Add((float)x - line*0.5f, (float)bottom, (float)x + line*0.5f, (float)top, gridColor);

            for (int y = bottom; y <= top; y++)
                quads.Add((float)left, (float)y - line*0.5f, (float)right, (float)y + line*0.5f, gridColor);

            quads.Flush();
        }

        float pixel = 1.0f/zoom;
        DrawFrame(Vec2F(-pixel, -pixel), size + Vec2F(pixel, pixel), pixel, Color4(128, 128, 128, 160));

        int sourceLeft = Math::Min(mSourceRect.left, mSourceRect.right), sourceTop = Math::Min(mSourceRect.top, mSourceRect.bottom);
        for (auto& rect : mHighlights)
        {
            float rowTop = (float)(Math::Min(rect.top, rect.bottom) - sourceTop), rowBottom = (float)(Math::Max(rect.top, rect.bottom) - sourceTop);
            Vec2F min((float)(Math::Min(rect.left, rect.right) - sourceLeft), size.y - rowBottom);
            Vec2F max((float)(Math::Max(rect.left, rect.right) - sourceLeft), size.y - rowTop);
            DrawFrame(min, max, Math::Min(2.0f*pixel, (max.x - min.x)*0.5f), Color4(255, 200, 0, 255));
        }

        if (mHovered && zoom >= 4.0f)
        {
            Vec2F min((float)mHoveredPixel.x, size.y - (float)mHoveredPixel.y - 1.0f);
            DrawFrame(min, min + Vec2F(1.0f, 1.0f), Math::Min(pixel, 0.25f), Color4(0, 170, 255, 255));
        }
    }

    void TextureView::OnTransformUpdated()
    {
        ScrollView::OnTransformUpdated();

        if (mFitPending)
            FitToView();
    }

    void TextureView::ReadPixels()
    {
        mPixels = nullptr;

        if (!mTexture || !mTexture->IsReady() || Texture::IsFormatCompressed(mTexture->GetFormat()))
            return;

        Vec2I textureSize = mTexture->GetSize();
        Vec2I size = GetSourceSize();
        int left = Math::Min(mSourceRect.left, mSourceRect.right), top = Math::Min(mSourceRect.top, mSourceRect.bottom);
        if (size.x <= 0 || size.y <= 0 || left < 0 || top < 0 || left + size.x > textureSize.x || top + size.y > textureSize.y)
            return;

        auto whole = mTexture->GetData();
        if (!whole || whole->GetSize() != textureSize || whole->GetFormat() != PixelFormat::R8G8B8A8)
            return;

        // The rectangle counts its rows from the top of the picture, the bitmap keeps them from the bottom
        int firstRow = textureSize.y - (top + size.y);
        mPixels = mmake<Bitmap>(PixelFormat::R8G8B8A8, size);
        for (int y = 0; y < size.y; y++)
        {
            memcpy(mPixels->GetData() + (size_t)y*size.x*4,
                   whole->GetData() + ((size_t)(firstRow + y)*textureSize.x + left)*4, (size_t)size.x*4);
        }
    }

    void TextureView::UpdateShownTexture()
    {
        if (!mPixels)
        {
            mChannelTexture = TextureRef();
            if (mTexture)
            {
                mTextureSprite->SetTexture(mTexture);
                mTextureSprite->SetTextureSrcRect(mSourceRect);
            }

            return;
        }

        mChannelTexture = TextureRef(*MakeChannelBitmap(*mPixels, mChannel));
        mChannelTexture->SetName("Editor texture view: " + GetTextureName(mTexture));
        mChannelTexture->SetFilter(Texture::Filter::Nearest);

        mTextureSprite->SetTexture(mChannelTexture);
        mTextureSprite->SetTextureSrcRect(RectI(Vec2I(), mChannelTexture->GetSize()));
    }

    Vec2I TextureView::GetSourceSize() const
    {
        return Vec2I(Math::Abs(mSourceRect.right - mSourceRect.left), Math::Abs(mSourceRect.bottom - mSourceRect.top));
    }

    bool TextureView::ScreenToPixel(const Vec2F& screenPoint, Vec2I& pixel) const
    {
        return LocalToPixel(screenPoint*mScreenToLocalTransform, pixel);
    }

    bool TextureView::LocalToPixel(const Vec2F& localPoint, Vec2I& pixel) const
    {
        Vec2I size = GetSourceSize();
        pixel = Vec2I((int)Math::Floor(localPoint.x), (int)Math::Floor((float)size.y - localPoint.y));
        return mTexture && pixel.x >= 0 && pixel.y >= 0 && pixel.x < size.x && pixel.y < size.y;
    }

    void TextureView::UpdateHoveredPixel(const Vec2F& localPoint)
    {
        Vec2I pixel;
        bool hovered = LocalToPixel(localPoint, pixel);

        if (hovered == mHovered && (!hovered || pixel == mHoveredPixel))
            return;

        mHovered = hovered;
        mHoveredPixel = pixel;
        onHoveredPixelChanged();
    }

    void TextureView::OnCursorPressed(const Input::Cursor& cursor)
    {
        mPanning = true;
    }

    void TextureView::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (mPanning)
            PanByCursor(cursor);
    }

    void TextureView::OnCursorRightMouseStayDown(const Input::Cursor& cursor)
    {
        PanByCursor(cursor);
    }

    void TextureView::PanByCursor(const Input::Cursor& cursor)
    {
        if (cursor.delta.Length() < Math::Epsilon)
            return;

        // The camera jumps to its target: a zoom still animating would otherwise slide under the cursor
        mViewCameraTargetPos -= cursor.delta;
        mViewCameraVelocity = Vec2F();
        mViewCamera.SetPosition2D(mViewCameraTargetPos);
        UpdateLocalScreenTransforms();
        mNeedRedraw = true;

        UpdateHoveredPixel(o2Input.GetCursorPos()*mScreenToLocalTransform);
    }

    void TextureView::OnCameraTransformChanged()
    {
        ScrollView::OnCameraTransformChanged();

        if (mCursorInside)
            UpdateHoveredPixel(o2Input.GetCursorPos()*mScreenToLocalTransform);
    }

    void TextureView::OnCursorEnter(const Input::Cursor& cursor)
    {
        mCursorInside = true;
        UpdateHoveredPixel(cursor.position);
    }

    void TextureView::OnCursorReleased(const Input::Cursor& cursor)
    {
        mPanning = false;
    }

    void TextureView::OnCursorMoved(const Input::Cursor& cursor)
    {
        mCursorInside = true;
        UpdateHoveredPixel(cursor.position);
    }

    void TextureView::OnCursorExit(const Input::Cursor& cursor)
    {
        mCursorInside = false;
        if (!mHovered)
            return;

        mHovered = false;
        onHoveredPixelChanged();
    }
}
// --- META ---

ENUM_META(Editor::TextureView::Channel, Editor__TextureView__Channel)
{
    ENUM_ENTRY(Alpha);
    ENUM_ENTRY(Blue);
    ENUM_ENTRY(Green);
    ENUM_ENTRY(RGB);
    ENUM_ENTRY(RGBA);
    ENUM_ENTRY(Red);
}
END_ENUM_META;

ENUM_META(Editor::TextureView::Background, Editor__TextureView__Background)
{
    ENUM_ENTRY(Black);
    ENUM_ENTRY(Checker);
    ENUM_ENTRY(White);
}
END_ENUM_META;

DECLARE_CLASS(Editor::TextureView, Editor__TextureView);
// --- END META ---
