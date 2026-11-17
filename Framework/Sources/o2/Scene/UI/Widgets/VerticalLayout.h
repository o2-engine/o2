#pragma once

#include "o2/Scene/UI/Widget.h"

namespace o2
{
    // ----------------------
    // Vertical layout widget
    // ----------------------
    class VerticalLayout: public Widget
    {
    public:
        PROPERTIES(VerticalLayout);
        PROPERTY(BaseCorner, baseCorner, SetBaseCorner, GetBaseCorner); // Base corder property

        PROPERTY(float, spacing, SetSpacing, GetSpacing); // Space between widgets property

        PROPERTY(BorderF, border, SetBorder, GetBorder);                 // Border property
        PROPERTY(float, borderLeft, SetBorderLeft, GetBorderLeft);       // Left border property
        PROPERTY(float, borderRight, SetBorderRight, GetBorderRight);    // Right border property
        PROPERTY(float, borderTop, SetBorderTop, GetBorderTop);          // Top border property
        PROPERTY(float, borderBottom, SetBorderBottom, GetBorderBottom); // Bottom border property

        PROPERTY(bool, expandWidth, SetWidthExpand, IsWidthExpand);    // Expand children by width property
        PROPERTY(bool, expandHeight, SetHeightExpand, IsHeightExpand); // Expand children by height property

        PROPERTY(bool, fitByChildren, SetFitByChildren, IsFittingByChildren); // Fitting size by children property

    public:        
        // Default constructor
        explicit VerticalLayout(RefCounter* refCounter);

        // Copy-constructor
        VerticalLayout(RefCounter* refCounter, const VerticalLayout& other);

        // Destructor
        ~VerticalLayout();

        // Copy operator
        VerticalLayout& operator=(const VerticalLayout& other);

        // Sets base corner
        void SetBaseCorner(BaseCorner baseCorner);

        // Returns base corner
        BaseCorner GetBaseCorner() const;

        // Sets space between widgets
        void SetSpacing(float spacing);

        // Returns space between widgets
        float GetSpacing() const;

        // Sets border
        void SetBorder(const BorderF& border);

        // Returns border
        BorderF GetBorder() const;

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

        // Sets expanding by width
        void SetWidthExpand(bool expand);

        // Returns expanding by width
        bool IsWidthExpand() const;

        // Sets expanding by height
        void SetHeightExpand(bool expand);

        // Returns height expand
        bool IsHeightExpand() const;

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

        SERIALIZABLE(VerticalLayout);
        CLONEABLE_REF(VerticalLayout);

    protected:
        BaseCorner mBaseCorner = BaseCorner::Top;  // Base corner of widgets arranging @SERIALIZABLE
        float      mSpacing = 0.0f;                // Space between widgets @SERIALIZABLE
        BorderF    mBorder;                        // Border @SERIALIZABLE
        bool       mExpandWidth = true;            // Expanding by width @SERIALIZABLE
        bool       mExpandHeight = true;           // Expanding by height @SERIALIZABLE
        bool       mFitByChildren = false;         // Fitting by children @SERIALIZABLE

    protected:
        // Returns layout width with children
        float GetMinWidthWithChildren() const override;

        // Returns layout height
        float GetMinHeightWithChildren() const override;

        // Returns layout height weight with children
        float GetHeightWeightWithChildren() const override;

        // Called when child widget was added
        void OnChildAdded(const Ref<Widget>& child) override;

        // Called when child widget was removed
        void OnChildRemoved(const Ref<Widget>& child) override;

        // Invokes required function for childs arranging
        virtual void RearrangeChilds();

        // Arranging child from left to right by bottom, middle and top
        void ArrangeFromTopToBottom();

        // Arranging child from right to left by bottom, middle and top
        void ArrangeFromBottomToTop();

        // Arranging child from center by bottom, middle and top
        void ArrangeFromCenter();

        // Expands size by children
        virtual void ExpandSizeByChilds();

        // Aligns widget by height with base corner
        void AlignWidgetByWidth(const Ref<Widget>& child, float heightAnchor);

        // Updates layout's weight and minimal size
        virtual void UpdateLayoutParametres();
    };
}
// --- META ---

CLASS_BASES_META(o2::VerticalLayout)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(o2::VerticalLayout)
{
    FIELD().PUBLIC().NAME(baseCorner);
    FIELD().PUBLIC().NAME(spacing);
    FIELD().PUBLIC().NAME(border);
    FIELD().PUBLIC().NAME(borderLeft);
    FIELD().PUBLIC().NAME(borderRight);
    FIELD().PUBLIC().NAME(borderTop);
    FIELD().PUBLIC().NAME(borderBottom);
    FIELD().PUBLIC().NAME(expandWidth);
    FIELD().PUBLIC().NAME(expandHeight);
    FIELD().PUBLIC().NAME(fitByChildren);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(BaseCorner::Top).NAME(mBaseCorner);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mSpacing);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mBorder);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mExpandWidth);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mExpandHeight);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mFitByChildren);
}
END_META;
CLASS_METHODS_META(o2::VerticalLayout)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const VerticalLayout&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBaseCorner, BaseCorner);
    FUNCTION().PUBLIC().SIGNATURE(BaseCorner, GetBaseCorner);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSpacing, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSpacing);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBorder, const BorderF&);
    FUNCTION().PUBLIC().SIGNATURE(BorderF, GetBorder);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBorderLeft, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetBorderLeft);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBorderRight, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetBorderRight);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBorderTop, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetBorderTop);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBorderBottom, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetBorderBottom);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWidthExpand, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsWidthExpand);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHeightExpand, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsHeightExpand);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFitByChildren, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFittingByChildren);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PROTECTED().SIGNATURE(float, GetMinWidthWithChildren);
    FUNCTION().PROTECTED().SIGNATURE(float, GetMinHeightWithChildren);
    FUNCTION().PROTECTED().SIGNATURE(float, GetHeightWeightWithChildren);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildAdded, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildRemoved, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, RearrangeChilds);
    FUNCTION().PROTECTED().SIGNATURE(void, ArrangeFromTopToBottom);
    FUNCTION().PROTECTED().SIGNATURE(void, ArrangeFromBottomToTop);
    FUNCTION().PROTECTED().SIGNATURE(void, ArrangeFromCenter);
    FUNCTION().PROTECTED().SIGNATURE(void, ExpandSizeByChilds);
    FUNCTION().PROTECTED().SIGNATURE(void, AlignWidgetByWidth, const Ref<Widget>&, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayoutParametres);
}
END_META;
// --- END META ---
