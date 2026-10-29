#pragma once

#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2Editor/UI/ScrollView.h"

using namespace o2;

namespace o2
{
    class Sprite;
    class VectorSprite;
}

namespace Editor
{
    // ------------------------------------------------------------------------------------------------
    // View of a vector image: the wheel zooms, a mouse button pans. Image draws it as a sprite of the view
    // scale, so it stays sharp at any zoom; Mesh draws the triangles the sprite takes at the zoom over it
    // ------------------------------------------------------------------------------------------------
    class VectorImageView: public ScrollView
    {
    public:
        enum class Mode { Image, Mesh };

    public:
        // Default constructor
        VectorImageView(RefCounter* refCounter);

        // Copy-constructor
        VectorImageView(RefCounter* refCounter, const VectorImageView& other);

        // Copy-operator
        VectorImageView& operator=(const VectorImageView& other);

        // Shows the image and fits it into the view
        void SetImage(const AssetRef<VectorImageAsset>& image);

        // Returns the shown image
        const AssetRef<VectorImageAsset>& GetImage() const;

        // Sets how the image is drawn
        void SetMode(Mode mode);

        // Returns how the image is drawn
        Mode GetMode() const;

        // Fits the image into the view
        void FitToView();

        // Returns the sprite drawing the image
        const Ref<VectorSprite>& GetSprite() const;

        // Returns the triangles of the last drawing of the image by its mesh
        UInt GetMeshTrianglesCount() const;

        SERIALIZABLE(VectorImageView);
        CLONEABLE_REF(VectorImageView);

    protected:
        AssetRef<VectorImageAsset> mImage;               // Shown image
        Mode                       mMode = Mode::Image; // How the image is drawn
        Ref<VectorSprite>          mSprite;             // Sprite of the image
        UInt                       mMeshTriangles = 0;  // Triangles of the last drawing by the mesh
        bool                       mFitPending = false; // Is the image fitted when the view gets its size
        bool                       mPanning = false;    // Is the view panned by the left button

    protected:
        // Draws the checker, the image and its triangles
        void RedrawContent() override;

        // Called when transformation was changed and updated, fits the image when it waits for the size
        void OnTransformUpdated() override;

        // Called when cursor pressed on this: starts panning
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame: pans the view
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor released: stops panning
        void OnCursorReleased(const Input::Cursor& cursor) override;
    };
}
// --- META ---

PRE_ENUM_META(Editor::VectorImageView::Mode);

CLASS_BASES_META(Editor::VectorImageView)
{
    BASE_CLASS(Editor::ScrollView);
}
END_META;
CLASS_FIELDS_META(Editor::VectorImageView)
{
    FIELD().PROTECTED().NAME(mImage);
    FIELD().PROTECTED().DEFAULT_VALUE(Mode::Image).NAME(mMode);
    FIELD().PROTECTED().NAME(mSprite);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mMeshTriangles);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mFitPending);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPanning);
}
END_META;
CLASS_METHODS_META(Editor::VectorImageView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const VectorImageView&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetImage, const AssetRef<VectorImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<VectorImageAsset>&, GetImage);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMode, Mode);
    FUNCTION().PUBLIC().SIGNATURE(Mode, GetMode);
    FUNCTION().PUBLIC().SIGNATURE(void, FitToView);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<VectorSprite>&, GetSprite);
    FUNCTION().PUBLIC().SIGNATURE(UInt, GetMeshTrianglesCount);
    FUNCTION().PROTECTED().SIGNATURE(void, RedrawContent);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
}
END_META;
// --- END META ---
