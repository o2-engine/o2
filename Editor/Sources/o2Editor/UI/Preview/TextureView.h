#pragma once

#include "o2/Render/TextureRef.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2Editor/UI/ScrollView.h"

using namespace o2;

namespace o2
{
    class Sprite;
}

namespace Editor
{
    // ---------------------------------------------------------------------------------------------------
    // Texture view for debugging: the wheel zooms, the left or the right mouse button pans. The pixels are
    // read back and drawn without filtering, so a zoomed texel is a square; the channels are shown by the
    // read pixels too. A texture that can't be read (a compressed one) is drawn as it is, in RGBA only.
    // Rectangles of the texture are the source rectangles of the sprites and the atlases: their rows go down
    // from the top of the picture, as the pixel under the cursor is counted
    // ---------------------------------------------------------------------------------------------------
    class TextureView: public ScrollView
    {
    public:
        enum class Channel { RGBA, RGB, Red, Green, Blue, Alpha };
        enum class Background { Checker, Black, White };

    public:
        Function<void()> onHoveredPixelChanged; // Called when the pixel under the cursor changes

    public:
        // Default constructor
        TextureView(RefCounter* refCounter);

        // Copy-constructor
        TextureView(RefCounter* refCounter, const TextureView& other);

        // Copy-operator
        TextureView& operator=(const TextureView& other);

        // Shows the rectangle of the texture, the whole one when it is empty, and fits it into the view
        void SetTexture(const TextureRef& texture, const RectI& sourceRect = RectI());

        // Returns the shown texture
        const TextureRef& GetTexture() const;

        // Returns the shown rectangle of the texture
        const RectI& GetSourceRect() const;

        // Reads the pixels again: a render target or an atlas of glyphs changes after it is shown
        void Reread();

        // Sets the shown channel
        void SetChannel(Channel channel);

        // Returns the shown channel
        Channel GetChannel() const;

        // Sets the background under the transparent pixels
        void SetBackground(Background background);

        // Returns the background
        Background GetBackground() const;

        // Sets the rectangles drawn by frames over the texture, in its pixels as the source rectangle
        void SetHighlights(const Vector<RectI>& rects);

        // Returns the highlighted rectangles
        const Vector<RectI>& GetHighlights() const;

        // Fits the texture into the view
        void FitToView();

        // Sets screen pixels per texel, keeping the center of the view
        void SetPixelZoom(float zoom);

        // Returns screen pixels per texel
        float GetPixelZoom() const;

        // Returns true when the pixels are read: the channels and the pixel under the cursor work
        bool IsReadable() const;

        // Returns the pixel under the cursor and its color; false when the cursor is out of the texture
        bool GetHoveredPixel(Vec2I& pixel, Color4& color) const;

        // Returns the color of the read pixel, from the left top of the shown rectangle
        Color4 GetPixel(const Vec2I& pixel) const;

        // Returns the pixel at the point of the screen; false when the point is out of the texture
        bool ScreenToPixel(const Vec2F& screenPoint, Vec2I& pixel) const;

        // Returns the bitmap that shows a channel of the source: a color one as gray, RGB as opaque
        static Ref<Bitmap> MakeChannelBitmap(const Bitmap& source, Channel channel);

        // Returns name of a channel
        static String GetChannelName(Channel channel);

        // Returns the name of the texture for debugging: its name, its file, its atlas page or its kind
        static String GetTextureName(const TextureRef& texture);

        // Returns bytes the texture takes in the video memory
        static UInt64 GetTextureMemory(const TextureRef& texture);

        // Returns name of the texture format
        static String GetFormatName(TextureFormat format);

        // Returns bytes as a short text: B, KB or MB
        static String FormatBytes(UInt64 bytes);

        // Draws a checker of the cell size over the visible part of the area, in the space of the current camera
        static void DrawChecker(const RectF& area, const RectF& visible, float cell);

        // Draws a frame of the width inside the rectangle, in the space of the current camera
        static void DrawFrame(const Vec2F& min, const Vec2F& max, float width, const Color4& color);

        SERIALIZABLE(TextureView);
        CLONEABLE_REF(TextureView);

    protected:
        TextureRef    mTexture;                          // Shown texture
        RectI         mSourceRect;                       // Shown rectangle of the texture
        Ref<Bitmap>   mPixels;                           // Read pixels of the rectangle, empty when it can't be read
        TextureRef    mChannelTexture;                   // Read pixels of the shown channel, drawn without filtering
        Channel       mChannel = Channel::RGBA;          // Shown channel
        Background    mBackground = Background::Checker; // Background under the transparent pixels
        Vector<RectI> mHighlights;                       // Rectangles drawn by frames

        Ref<Sprite> mTextureSprite; // Texture or its channel

        bool  mFitPending = false;  // Is the texture fitted when the view gets its size
        bool  mPanning = false;      // Is the view panned by the left button
        bool  mCursorInside = false; // Is the cursor over the view
        Vec2I mHoveredPixel;         // Pixel under the cursor
        bool  mHovered = false;      // Is the cursor over the texture

    protected:
        // Draws the background, the texture, the pixels grid and the highlights
        void RedrawContent() override;

        // Called when transformation was changed and updated, fits the texture when it waits for the size
        void OnTransformUpdated() override;

        // Reads the pixels of the source rectangle
        void ReadPixels();

        // Makes the texture of the shown channel and sets the sprite to it
        void UpdateShownTexture();

        // Returns size of the shown rectangle
        Vec2I GetSourceSize() const;

        // Returns the part of the local space the view shows
        RectF GetVisibleArea() const;

        // Returns the pixel at the point of the texture space; false when the point is out of the texture
        bool LocalToPixel(const Vec2F& localPoint, Vec2I& pixel) const;

        // Updates the pixel under the cursor, the point is in the texture space
        void UpdateHoveredPixel(const Vec2F& localPoint);

        // Moves the camera so the texel under the cursor goes with it; the view gets the cursor in its local space
        void PanByCursor(const Input::Cursor& cursor);

        // Called when the camera moved or zoomed: the pixel under the still cursor changes
        void OnCameraTransformChanged() override;

        // Called when cursor pressed on this: starts panning
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame: pans the view
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor released: stops panning
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when right mouse button stay down on this: pans the view as the left one
        void OnCursorRightMouseStayDown(const Input::Cursor& cursor) override;

        // Called when cursor enters this
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor moved on this: updates the pixel under the cursor
        void OnCursorMoved(const Input::Cursor& cursor) override;

        // Called when cursor exits this: forgets the pixel under the cursor
        void OnCursorExit(const Input::Cursor& cursor) override;
    };
}
// --- META ---

PRE_ENUM_META(Editor::TextureView::Channel);

PRE_ENUM_META(Editor::TextureView::Background);

CLASS_BASES_META(Editor::TextureView)
{
    BASE_CLASS(Editor::ScrollView);
}
END_META;
CLASS_FIELDS_META(Editor::TextureView)
{
    FIELD().PUBLIC().NAME(onHoveredPixelChanged);
    FIELD().PROTECTED().NAME(mTexture);
    FIELD().PROTECTED().NAME(mSourceRect);
    FIELD().PROTECTED().NAME(mPixels);
    FIELD().PROTECTED().NAME(mChannelTexture);
    FIELD().PROTECTED().DEFAULT_VALUE(Channel::RGBA).NAME(mChannel);
    FIELD().PROTECTED().DEFAULT_VALUE(Background::Checker).NAME(mBackground);
    FIELD().PROTECTED().NAME(mHighlights);
    FIELD().PROTECTED().NAME(mTextureSprite);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mFitPending);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPanning);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mCursorInside);
    FIELD().PROTECTED().NAME(mHoveredPixel);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHovered);
}
END_META;
CLASS_METHODS_META(Editor::TextureView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const TextureView&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTexture, const TextureRef&, const RectI&);
    FUNCTION().PUBLIC().SIGNATURE(const TextureRef&, GetTexture);
    FUNCTION().PUBLIC().SIGNATURE(const RectI&, GetSourceRect);
    FUNCTION().PUBLIC().SIGNATURE(void, Reread);
    FUNCTION().PUBLIC().SIGNATURE(void, SetChannel, Channel);
    FUNCTION().PUBLIC().SIGNATURE(Channel, GetChannel);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBackground, Background);
    FUNCTION().PUBLIC().SIGNATURE(Background, GetBackground);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHighlights, const Vector<RectI>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<RectI>&, GetHighlights);
    FUNCTION().PUBLIC().SIGNATURE(void, FitToView);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPixelZoom, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetPixelZoom);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsReadable);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetHoveredPixel, Vec2I&, Color4&);
    FUNCTION().PUBLIC().SIGNATURE(Color4, GetPixel, const Vec2I&);
    FUNCTION().PUBLIC().SIGNATURE(bool, ScreenToPixel, const Vec2F&, Vec2I&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<Bitmap>, MakeChannelBitmap, const Bitmap&, Channel);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetChannelName, Channel);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetTextureName, const TextureRef&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(UInt64, GetTextureMemory, const TextureRef&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetFormatName, TextureFormat);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, FormatBytes, UInt64);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(void, DrawChecker, const RectF&, const RectF&, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(void, DrawFrame, const Vec2F&, const Vec2F&, float, const Color4&);
    FUNCTION().PROTECTED().SIGNATURE(void, RedrawContent);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, ReadPixels);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateShownTexture);
    FUNCTION().PROTECTED().SIGNATURE(Vec2I, GetSourceSize);
    FUNCTION().PROTECTED().SIGNATURE(RectF, GetVisibleArea);
    FUNCTION().PROTECTED().SIGNATURE(bool, LocalToPixel, const Vec2F&, Vec2I&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHoveredPixel, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, PanByCursor, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCameraTransformChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseStayDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
}
END_META;
// --- END META ---
