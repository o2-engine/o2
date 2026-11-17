#pragma once

#include "o2/Scene/UI/Widget.h"

namespace o2
{
    // ------------------
    // Grid layout widget
    // ------------------
    class GridLayout: public Widget
    {
    public:
        PROPERTIES(GridLayout);
        PROPERTY(BaseCorner, baseCorner, SetBaseCorner, GetBaseCorner);                     // Base corder property
        PROPERTY(Vec2F, cellSize, SetCellSize, GetCellSize);                                // Grid cell size
        PROPERTY(int, arrangeAxisMaxCells, SetArrangeAxisMaxCells, GetArrangeAxisMaxCells); // Maximum cells at arranging axis
        PROPERTY(TwoDirection, arrangeAxis, SetArrangeAxis, GetArrangeAxis);                // Arrange axis

        PROPERTY(float, spacing, SetSpacing, GetSpacing);                // Space between widgets property
        PROPERTY(RectF, border, SetBorder, GetBorder);                   // Border property
        PROPERTY(float, borderLeft, SetBorderLeft, GetBorderLeft);       // Left border property
        PROPERTY(float, borderRight, SetBorderRight, GetBorderRight);    // Right border property
        PROPERTY(float, borderTop, SetBorderTop, GetBorderTop);          // Top border property
        PROPERTY(float, borderBottom, SetBorderBottom, GetBorderBottom); // Bottom border property

        PROPERTY(bool, fitByChildren, SetFitByChildren, IsFittingByChildren); // Fitting size by children property

    public:
        // Default constructor
        explicit GridLayout(RefCounter* refCounter);

        // Copy-constructor
        GridLayout(RefCounter* refCounter, const GridLayout& other);

        // Destructor
        ~GridLayout();

        // Copy operator
        GridLayout& operator=(const GridLayout& other);

        // Sets base corner
        void SetBaseCorner(BaseCorner baseCorner);

        // Returns base corner
        BaseCorner GetBaseCorner() const;

        // Sets space between widgets
        void SetSpacing(float spacing);

        // Returns space between widgets
        float GetSpacing() const;

        // Sets border
        void SetBorder(const RectF& border);

        // Returns border
        RectF GetBorder() const;

        // Sets left border
        void SetBorderLeft(float value);

        // Returns left border
        float GetBorderLeft() const;

        // Sets right border
        void SetBorderRight(float value);

        // Returns right border
        float GetBorderRight() const;

        // Sets top border
        void SetBorderTop(float value);

        // Returns top border
        float GetBorderTop() const;

        // Sets bottom border
        void SetBorderBottom(float value);

        // Returns bottom border
        float GetBorderBottom() const;

        // Sets cell size
        void SetCellSize(const Vec2F& size);

        // Returns cell size
        Vec2F GetCellSize() const;

        // Sets arrange type
        void SetArrangeAxis(TwoDirection type);

        // Returns arrange type
        TwoDirection GetArrangeAxis() const;

        // Sets arrange axis maximum cells count
        void SetArrangeAxisMaxCells(int count);

        // Returns arrange axis maximum cells count
        int GetArrangeAxisMaxCells() const;

        // Sets fitting size by children
        void SetFitByChildren(bool fit);

        // Returns fitting by children
        bool IsFittingByChildren() const;

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        // Returns the icon of the type in the editor
        static String GetEditorIcon();

        SERIALIZABLE(GridLayout);
        CLONEABLE_REF(GridLayout);

    protected:
        BaseCorner   mBaseCorner = BaseCorner::Left;          // Base corner of widgets arranging @SERIALIZABLE
        float        mSpacing = 0.0f;                         // Space between widgets @SERIALIZABLE
        Vec2F        mCellSize = Vec2F(50, 50);               // Grid cell size @SERIALIZABLE
        TwoDirection mArrangeAxis = TwoDirection::Horizontal; // Cells arrange axis @SERIALIZABLE
        int          mArrangeAxisMaxCells = 5;                // Arrange axis maximum cells count @SERIALIZABLE
        RectF        mBorder;                                 // Border @SERIALIZABLE
        bool         mFitByChildren = false;                  // Fitting by children @SERIALIZABLE

    protected:
        // Called when child widget was added
        void OnChildAdded(const Ref<Widget>& child) override;

        // Called when child widget was removed
        void OnChildRemoved(const Ref<Widget>& child) override;

        // Invokes required function for childs arranging
        void RearrangeChilds();

        // Arrange childs from left top
        void ArrangeFromLeftTop();

        // Arrange childs from top
        void ArrangeFromTop();

        // Arrange childs from right top
        void ArrangeFromRightTop();

        // Arrange childs from left
        void ArrangeFromLeft();

        // Arrange childs from center
        void ArrangeFromCenter();

        // Arrange childs from right
        void ArrangeFromRight();

        // Arrange childs from left bottom
        void ArrangeFromLeftBottom();

        // Arrange childs from bottom
        void ArrangeFromBottom();

        // Arrange childs from right bottom
        void ArrangeFromRightBottom();

        // Expands size by children
        void ExpandSizeByChilds();
    };
}
// --- META ---

CLASS_BASES_META(o2::GridLayout)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(o2::GridLayout)
{
    FIELD().PUBLIC().NAME(baseCorner);
    FIELD().PUBLIC().NAME(cellSize);
    FIELD().PUBLIC().NAME(arrangeAxisMaxCells);
    FIELD().PUBLIC().NAME(arrangeAxis);
    FIELD().PUBLIC().NAME(spacing);
    FIELD().PUBLIC().NAME(border);
    FIELD().PUBLIC().NAME(borderLeft);
    FIELD().PUBLIC().NAME(borderRight);
    FIELD().PUBLIC().NAME(borderTop);
    FIELD().PUBLIC().NAME(borderBottom);
    FIELD().PUBLIC().NAME(fitByChildren);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(BaseCorner::Left).NAME(mBaseCorner);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mSpacing);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Vec2F(50, 50)).NAME(mCellSize);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(TwoDirection::Horizontal).NAME(mArrangeAxis);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(5).NAME(mArrangeAxisMaxCells);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mBorder);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mFitByChildren);
}
END_META;
CLASS_METHODS_META(o2::GridLayout)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const GridLayout&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBaseCorner, BaseCorner);
    FUNCTION().PUBLIC().SIGNATURE(BaseCorner, GetBaseCorner);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSpacing, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSpacing);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBorder, const RectF&);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetBorder);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBorderLeft, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetBorderLeft);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBorderRight, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetBorderRight);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBorderTop, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetBorderTop);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBorderBottom, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetBorderBottom);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCellSize, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetCellSize);
    FUNCTION().PUBLIC().SIGNATURE(void, SetArrangeAxis, TwoDirection);
    FUNCTION().PUBLIC().SIGNATURE(TwoDirection, GetArrangeAxis);
    FUNCTION().PUBLIC().SIGNATURE(void, SetArrangeAxisMaxCells, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetArrangeAxisMaxCells);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFitByChildren, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFittingByChildren);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildAdded, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildRemoved, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, RearrangeChilds);
    FUNCTION().PROTECTED().SIGNATURE(void, ArrangeFromLeftTop);
    FUNCTION().PROTECTED().SIGNATURE(void, ArrangeFromTop);
    FUNCTION().PROTECTED().SIGNATURE(void, ArrangeFromRightTop);
    FUNCTION().PROTECTED().SIGNATURE(void, ArrangeFromLeft);
    FUNCTION().PROTECTED().SIGNATURE(void, ArrangeFromCenter);
    FUNCTION().PROTECTED().SIGNATURE(void, ArrangeFromRight);
    FUNCTION().PROTECTED().SIGNATURE(void, ArrangeFromLeftBottom);
    FUNCTION().PROTECTED().SIGNATURE(void, ArrangeFromBottom);
    FUNCTION().PROTECTED().SIGNATURE(void, ArrangeFromRightBottom);
    FUNCTION().PROTECTED().SIGNATURE(void, ExpandSizeByChilds);
}
END_META;
// --- END META ---
