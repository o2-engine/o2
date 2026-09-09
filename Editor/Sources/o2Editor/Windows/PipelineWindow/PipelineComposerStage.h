#pragma once

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/TextureRef.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Editor/FrameHandles.h"
#include "o2/Utils/Function/Function.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineGraph.h"

using namespace o2;

namespace o2
{
    class Bitmap;
    class Sprite;
    class Text;
}

namespace Editor
{
    // --------------------------------------------------------------------
    // One layer's placement on the work area, in work-area pixels (y down)
    // --------------------------------------------------------------------
    struct ComposerLayerPlacement
    {
        float x = 0, y = 0, w = 2, h = 2, rot = 0; // Left-top corner, size and rotation in degrees

        bool  hidden = false; // Skipped when drawing and rendering
        bool  flipH = false;  // Mirrored horizontally
        bool  flipV = false;  // Mirrored vertically
        float opacity = 1.0f; // Layer transparency, 0..1

        bool                        nine = false;      // Nine-slice scaling is enabled
        PipelineImageOps::NineSlice slice;             // Nine-slice borders in source pixels
        float                       sliceScale = 1.0f; // Scale of the nine-slice borders

        bool lockAspect = true; // Keeps the aspect ratio when resizing
        bool stored = false;    // True when read from the config, false for a default placement
    };

    // -----------------------------------------------------------------
    // Composer work area: draws the layers at their placement, lets the
    // user select, move, scale and rotate them with frame handles
    // -----------------------------------------------------------------
    class PipelineComposerStage : public Widget, public CursorAreaEventsListener
    {
    public:
        Function<void(const String&, bool)> onConfigChanged; // Called with the config key and the completed flag after a write
        Function<Ref<Bitmap>(const String&)> imageOfPort;    // Returns the upstream image of an input port

    public:
        // Default constructor
        explicit PipelineComposerStage(RefCounter* refCounter);

        // Sets the composer node and refreshes the layers
        void Init(const Ref<PipelineNode>& node);

        // Re-reads layers and their images, keeping the sprites of unchanged sources
        void Refresh();

        // Returns the layers resolved from the node inputs
        Vector<ComposerLayerRef> GetLayers() const;

        // Returns the placement stored in the config, or the default one when it is missing
        ComposerLayerPlacement GetPlacement(const ComposerLayerRef& layer) const;

        // Returns the placement centering the port image on the work area, scaled down to fit
        ComposerLayerPlacement DefaultPlacement(const String& portId) const;

        // Returns the size of the port image, zero when it is not loaded
        Vec2I GetNaturalSize(const String& portId) const;

        // Returns the upstream image of the port, null when it is not loaded
        Ref<Bitmap> GetLayerImage(const String& portId) const;

        // Returns the layer image resized, nine-sliced and flipped by its placement
        Ref<Bitmap> RenderLayer(const ComposerLayerRef& layer) const;

        // Stores the placement in the config "layers" object and notifies
        void WritePlacement(const String& layerId, const ComposerLayerPlacement& placement, bool completed);

        // Stores the selected layer id in the config and notifies when it changes
        void SelectLayer(const String& layerId);

        // Returns the selected layer id, empty when none
        String GetSelectedLayer() const;

        // Returns the work-area pixel to screen scale: fit to the widget times the config zoom
        float GetViewScale() const;

        // Updates the widget
        void Update(float dt) override;

        // Draws the background, the layers, the selection frame with the size badge and the hint
        void Draw() override;

        // Returns true when the point is inside the widget
        bool IsUnderPoint(const Vec2F& point) override;

        SERIALIZABLE(PipelineComposerStage);

    protected:
        // ------------------------------------------------------------
        // Layer view: layer reference, its source image and the sprite
        // ------------------------------------------------------------
        struct LayerView
        {
            ComposerLayerRef ref;      // Layer reference from the node
            Ref<Bitmap>      source;   // Upstream image
            Ref<Sprite>      sprite;   // Sprite drawn on the stage
            TextureRef       texture;  // Texture of the sprite
            String           cacheKey; // Nine-slice settings the sprite was built for

            // Returns true when the other view refers to the same layer
            bool operator==(const LayerView& other) const { return ref.id == other.ref.id; }
        };

        Ref<PipelineNode> mNode;          // Composer node
        Vector<LayerView> mLayers;        // Layer views in draw order
        Ref<Sprite>       mCheckerSprite; // Tiled checkerboard background
        Ref<Text>         mNameText;      // Text used for layer names, the size badge and the hint

        Ref<FrameHandles> mFrame;                 // Transform frame of the selected layer
        bool              mFrameSyncing = false;  // True while the frame is set from the placement
        bool              mFrameDragging = false; // True while the frame handles are dragged

        bool                   mMoving = false; // True while a layer is dragged with the cursor
        String                 mMovingId;       // Id of the dragged layer
        Vec2F                  mMoveStart;      // Cursor position in work-area pixels when the drag began
        ComposerLayerPlacement mMoveOrigin;     // Placement of the layer when the drag began

    protected:
        // Returns the work area width from the config, 16..8192
        int GetCanvasW() const;

        // Returns the work area height from the config, 16..8192
        int GetCanvasH() const;

        // Returns the work area rectangle on screen, centered in the widget
        RectF GetStageRect() const;

        // Converts a screen point to work-area pixels (y down)
        Vec2F ToWork(const Vec2F& canvasPoint) const;

        // Returns the screen basis of the placement, rotated around the layer center
        Basis PlacementBasis(const ComposerLayerPlacement& placement) const;

        // Returns true when the screen point is inside the placed layer
        bool IsPointInLayer(const ComposerLayerPlacement& placement, const Vec2F& canvasPoint) const;

        // Returns the view of the layer, null when it is missing
        LayerView* FindView(const String& layerId);

        // Creates the layer sprite, rebuilding it when the nine-slice settings change
        void EnsureLayerSprite(LayerView& view, const ComposerLayerPlacement& placement);

        // Sets the frame basis from the selected layer placement
        void SyncFrame();

        // Called when the frame is transformed; writes the placement of the selected layer
        void OnFrameTransformed(const Basis& basis);

        // Called when the frame transform is completed; writes the placement as completed
        void OnFrameCompleted();

        // Called when cursor pressed on this; selects the top layer under the cursor and starts moving it
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame; moves the layer, Shift locks one axis
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor released; writes the moved placement as completed
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken; completes the move
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        REF_COUNTERABLE_IMPL(Widget);
    };
}
// --- META ---

CLASS_BASES_META(Editor::PipelineComposerStage)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::CursorAreaEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineComposerStage)
{
    FIELD().PUBLIC().NAME(onConfigChanged);
    FIELD().PUBLIC().NAME(imageOfPort);
    FIELD().PROTECTED().NAME(mNode);
    FIELD().PROTECTED().NAME(mLayers);
    FIELD().PROTECTED().NAME(mCheckerSprite);
    FIELD().PROTECTED().NAME(mNameText);
    FIELD().PROTECTED().NAME(mFrame);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mFrameSyncing);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mFrameDragging);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mMoving);
    FIELD().PROTECTED().NAME(mMovingId);
    FIELD().PROTECTED().NAME(mMoveStart);
    FIELD().PROTECTED().NAME(mMoveOrigin);
}
END_META;
CLASS_METHODS_META(Editor::PipelineComposerStage)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Init, const Ref<PipelineNode>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PUBLIC().SIGNATURE(Vector<ComposerLayerRef>, GetLayers);
    FUNCTION().PUBLIC().SIGNATURE(ComposerLayerPlacement, GetPlacement, const ComposerLayerRef&);
    FUNCTION().PUBLIC().SIGNATURE(ComposerLayerPlacement, DefaultPlacement, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetNaturalSize, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Bitmap>, GetLayerImage, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Bitmap>, RenderLayer, const ComposerLayerRef&);
    FUNCTION().PUBLIC().SIGNATURE(void, WritePlacement, const String&, const ComposerLayerPlacement&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectLayer, const String&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetSelectedLayer);
    FUNCTION().PUBLIC().SIGNATURE(float, GetViewScale);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(int, GetCanvasW);
    FUNCTION().PROTECTED().SIGNATURE(int, GetCanvasH);
    FUNCTION().PROTECTED().SIGNATURE(RectF, GetStageRect);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, ToWork, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, PlacementBasis, const ComposerLayerPlacement&);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsPointInLayer, const ComposerLayerPlacement&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(LayerView*, FindView, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, EnsureLayerSprite, LayerView&, const ComposerLayerPlacement&);
    FUNCTION().PROTECTED().SIGNATURE(void, SyncFrame);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFrameTransformed, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFrameCompleted);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
}
END_META;
// --- END META ---
