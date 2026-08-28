#pragma once

#include "o2/Scene/ActorTransform.h"

namespace o2
{
    class Widget;

    // ----------------------------------------------------------------------------------------------------------------
    // Widget layout. Represents the Widget transformation layout. The position and size sets by anchors and offsets. 
    // Anchor is a vector value from (0, 0) to (1, 1), where (0, 0) is left bottom corner of parent and (1, 1) is right 
    // top corner. Offsets represents the position and size relative to anchor points. 
    // Also layout has minimal, maximum size and layout weight, used in grouped layouts.
    // ----------------------------------------------------------------------------------------------------------------
    class WidgetLayout: public ActorTransform
    {
    public:
        PROPERTIES(WidgetLayout);
        PROPERTY(Vec2F, anchorMin, SetAnchorMin, GetAnchorMin); // Left bottom anchor property
        PROPERTY(Vec2F, anchorMax, SetAnchorMax,GetAnchorMax);  // Right top anchor property

        PROPERTY(Vec2F, offsetMin, SetOffsetMin, GetOffsetMin); // Left bottom corner offset property
        PROPERTY(Vec2F, offsetMax, SetOffsetMax, GetOffsetMax); // Right top corner offset property

        PROPERTY(float, anchorLeft, SetAnchorLeft, GetAnchorLeft);       // Left anchor property
        PROPERTY(float, anchorRight, SetAnchorRight, GetAnchorRight);    // Right anchor property
        PROPERTY(float, anchorBottom, SetAnchorBottom, GetAnchorBottom); // Bottom anchor property
        PROPERTY(float, anchorTop, SetAnchorTop, GetAnchorTop);          // Top anchor property

        PROPERTY(float, offsetLeft, SetOffsetLeft, GetOffsetLeft);       // Left offset property
        PROPERTY(float, offsetRight, SetoffsetRight, GetoffsetRight);    // Right offset property
        PROPERTY(float, offsetBottom, SetOffsetBottom, GetOffsetBottom); // Bottom offset property
        PROPERTY(float, offsetTop, SetOffsetTop, GetOffsetTop);          // Top offset property

        PROPERTY(Vec2F, minSize, SetMinimalSize, GetMinimalSize);      // Minimal size property
        PROPERTY(float, minWidth, SetMinimalWidth, GetMinWidth);   // Minimal width property
        PROPERTY(float, minHeight,SetMinimalHeight, GetMinHeight); // Minimal height property

        PROPERTY(Vec2F, maxSize, SetMaximalSize, GetMaximalSize);       // Maximum size property
        PROPERTY(float, maxWidth, SetMaximalWidth, GetMaxWidth);    // Maximum width property
        PROPERTY(float, maxHeight, SetMaximalHeight, GetMaxHeight); // Maximum height property

        PROPERTY(Vec2F, weight, SetWeight, GetWeight);                   // Layout weight property
        PROPERTY(float, widthWeight, SetWidthWeight, GetWidthWeight);    // Width layout weight property
        PROPERTY(float, heightWeight, SetHeightWeight, GetHeightWeight); // Height layout weight property

    public:
        // Default constructor, creates both stretching layout  
        WidgetLayout();

        // Constructor with parameters
        WidgetLayout(const Vec2F& anchorMin, const Vec2F& anchorMax, const Vec2F& offsetMin, const Vec2F& offsetMax);

        // Constructor with parameters
        WidgetLayout(float anchorLeft, float anchorTop, float anchorRight, float anchorBottom,
                     float offsetLeft, float offsetTop, float offsetRight, float offsetBottom);

        // Copy-constructor
        WidgetLayout(const WidgetLayout& other);

        // Copy-operator
        WidgetLayout& operator=(const WidgetLayout& other);

        // Equals operator
        bool operator==(const WidgetLayout& other) const;

        // Updates layout and transformation
        void Update() override;

        // Sets transform dirty and needed to update. Checks is driven by parent and marks parent as dirty too @SCRIPTABLE
        void SetDirty(bool fromParent = false) override;

        // Copies data parameters from other layout @SCRIPTABLE
        void CopyFrom(const ActorTransform& other) override;

        // Sets 2D position
        void SetPosition2D(const Vec2F& position) override;

        // Sets 2D size
        void SetSize2D(const Vec2F& size) override;

        // Sets width
        void SetWidth(float value) override;

        // Sets height
        void SetHeight(float value) override;

        // Return 2D size
        Vec2F GetSize2D() const override;

        // Return width
        float GetWidth() const override;

        // Return height
        float GetHeight() const override;

        // Sets 2D pivot, in local space, where (0, 0) - left down corner, (1; 1) - right top
        void SetPivot2D(const Vec2F& pivot) override;

        // Sets basis
        void SetBasis(const Basis& basis) override;

        // Sets basis without size
        void SetNonSizedBasis(const Basis& basis) override;

        // Returns rect
        RectF GetRect() const override;

        // Sets rect
        void SetRect(const RectF& rect) override;

        // Return rectangle for children in worlds space 
        RectF GetChildrenWorldRect() const;

        // Sets direction aligned rectangle transformation
        void SetAxisAlignedRect(const RectF& rect) override;

        // Sets left bottom anchor. (0, 0) - left bottom of parent, (1, 1) - right top @SCRIPTABLE
        void SetAnchorMin(const Vec2F& min);

        // Returns left bottom anchor. (0, 0) - left bottom of parent, (1, 1) - right top @SCRIPTABLE
        Vec2F GetAnchorMin() const;

        // Sets right top anchor. (0, 0) - left bottom of parent, (1, 1) - right top @SCRIPTABLE
        void SetAnchorMax(const Vec2F& max);

        // Returns right top anchor. (0, 0) - left bottom of parent, (1, 1) - right top @SCRIPTABLE
        Vec2F GetAnchorMax() const;

        // Sets left anchor. 0 - left parent border, 1 - right @SCRIPTABLE
        void SetAnchorLeft(float value);

        // Returns left anchor. 0 - left parent border, 1 - right @SCRIPTABLE
        float GetAnchorLeft() const;

        // Sets right anchor. 0 - left parent border, 1 - right @SCRIPTABLE
        void SetAnchorRight(float value);

        // Returns right anchor. 0 - left parent border, 1 - right @SCRIPTABLE
        float GetAnchorRight() const;

        // Sets bottom anchor. 0 - bottom parent border, 1 - top @SCRIPTABLE
        void SetAnchorBottom(float value);

        // Returns bottom anchor. 0 - bottom parent border, 1 - top @SCRIPTABLE
        float GetAnchorBottom() const;

        // Sets top anchor. 0 - bottom parent border, 1 - top @SCRIPTABLE
        void SetAnchorTop(float value);

        // Returns top anchor. 0 - bottom parent border, 1 - top @SCRIPTABLE
        float GetAnchorTop() const;

        // Sets left bottom offset @SCRIPTABLE
        void SetOffsetMin(const Vec2F& min);

        // Returns left bottom offset @SCRIPTABLE
        Vec2F GetOffsetMin() const;

        // Sets right top offset @SCRIPTABLE
        void SetOffsetMax(const Vec2F& max);

        // Returns right top offset @SCRIPTABLE
        Vec2F GetOffsetMax() const;

        // Sets left offset @SCRIPTABLE
        void SetOffsetLeft(float value);

        // Returns left offset @SCRIPTABLE
        float GetOffsetLeft() const;

        // Sets right offset @SCRIPTABLE
        void SetoffsetRight(float value);

        // Returns right offset @SCRIPTABLE
        float GetoffsetRight() const;

        // Sets bottom offset @SCRIPTABLE
        void SetOffsetBottom(float value);

        // Returns bottom offset @SCRIPTABLE
        float GetOffsetBottom() const;

        // Sets top offset @SCRIPTABLE
        void SetOffsetTop(float value);

        // Returns top offset @SCRIPTABLE
        float GetOffsetTop() const;

        // Sets minimal size, enables size check @SCRIPTABLE
        void SetMinimalSize(const Vec2F& minSize);

        // Returns minimal size @SCRIPTABLE
        Vec2F GetMinimalSize() const;

        // Sets minimal width, enables size check @SCRIPTABLE
        void SetMinimalWidth(float value);

        // Returns minimal width @SCRIPTABLE
        float GetMinWidth() const;

        // Sets minimal height, enables size check @SCRIPTABLE
        void SetMinimalHeight(float value);

        // Returns minimal height @SCRIPTABLE
        float GetMinHeight() const;

        // Sets maximal size, enables size check @SCRIPTABLE
        void SetMaximalSize(const Vec2F& maxSize);

        // Returns maximal size @SCRIPTABLE
        Vec2F GetMaximalSize() const;

        // Sets maximal width, enables size check @SCRIPTABLE
        void SetMaximalWidth(float value);

        // Returns maximal width @SCRIPTABLE
        float GetMaxWidth() const;

        // Sets maximal height, enables size check @SCRIPTABLE
        void SetMaximalHeight(float value);

        // Returns maximal height @SCRIPTABLE
        float GetMaxHeight() const;

        // Disables size checking
        void DisableSizeChecks();

        // Enables size checking
        void EnableSizeChecks();

        // Sets layout weight @SCRIPTABLE
        void SetWeight(const Vec2F& weight);

        // Returns layout height @SCRIPTABLE
        Vec2F GetWeight() const;

        // Sets width layout weight @SCRIPTABLE
        void SetWidthWeight(float widthWeigth);

        // Returns width layout weight  @SCRIPTABLE
        float GetWidthWeight();

        // Sets height layout weight @SCRIPTABLE
        void SetHeightWeight(float heigthWeigth);

        // Returns height layout weight @SCRIPTABLE
        float GetHeightWeight();

        // Returns transform from the axis-aligned layout rect (GetWorldRect) to the world: rotation and scale
        // of this widget and its parents applied. Layers and children are placed through it
        Basis GetLayoutToWorldBasis() const;

        // Returns true when the point is inside the layout rect, honoring the widget's world rotation and scale
        bool IsPointInside(const Vec2F& point) const;

        // Returns world point in the axis-aligned layout space (inverse of GetLayoutToWorldBasis)
        Vec2F WorldToLayoutPoint(const Vec2F& worldPoint) const;

        // Returns true when the world transform is the plain layout rect: no rotation, scale or shear in the chain
        bool IsWorldTransformPlain() const;

        // Enables rounding of scene widgets layouts to whole units (pixel-perfect UI). The editor turns it off
        // while editing so frame handles move smoothly at any zoom
        static void SetSceneLayoutsRounding(bool enabled);

        // Returns is scene widgets layouts rounding enabled
        static bool IsSceneLayoutsRounding();

        // Returns both axis stretching by parent layout with border offsets @SCRIPTABLE
        static WidgetLayout BothStretch(float borderLeft = 0, float borderBottom = 0, float borderRight = 0, float borderTop = 0);

        // Returns fixed by size layout, positioned relative to corner with offset  @SCRIPTABLE
        static WidgetLayout Based(BaseCorner corner, const Vec2F& size, const Vec2F& offset = Vec2F());

        // Returns horizontal stretching by parent layout, anchored by height with offsets and left and right border @SCRIPTABLE
        static WidgetLayout HorStretch(VerAlign align, float left, float right, float height, float offsY = 0.0f);

        // Returns vertical stretching by parent layout, anchored by width with offsets and left and right border @SCRIPTABLE
        static WidgetLayout VerStretch(HorAlign align, float top, float bottom, float width, float offsX = 0.0f);

        SERIALIZABLE(WidgetLayout);

    protected:
        void(WidgetLayout::*mCheckMinMaxFunc)(); // Check minimum and maximum of layout delegate

        Vec2F mAnchorMin = Vec2F(0, 0); // Left bottom anchor @DELTA_SEARCH
        Vec2F mAnchorMax = Vec2F(0, 0); // Right top anchor @DELTA_SEARCH

        Vec2F mOffsetMin = Vec2F(0, 0);   // Left bottom offset @DELTA_SEARCH
        Vec2F mOffsetMax = Vec2F(10, 10); // Right top offset @DELTA_SEARCH

        Vec2F mMinSize = Vec2F(0, 0);         // Minimal size @DELTA_SEARCH
        Vec2F mMaxSize = Vec2F(10000, 10000); // Maximum size @DELTA_SEARCH

        Vec2F mWeight = Vec2F(1, 1); // Layout weight @DELTA_SEARCH

        RectF mChildrenWorldRect; // World rectangle for children arranging

        bool mDrivenByParent = false; // Is layout controlling by parent

        Widget* mOwnerWidget = nullptr; // owner widget pointer

    protected:
        // Returns is serialize enabled; used to turn off fields serialization
        bool IsSerializeEnabled() const override;

        // Sets owner and updates transform
        void SetOwner(const Ref<Actor>& actor) override;

        // Returns parent rectange, or zero when no parent
        RectF GetParentRectangle() const override;

        // Floors all local rectangle properties
        static bool mSceneLayoutsRounding; // Scene widgets layouts are rounded to whole units

        void FloorRectangle();

        // Updates offsets to match existing rectangle to offsets and anchors rectangle
        void UpdateOffsetsByCurrentTransform();

        // Checks minimum and maximum size
        void CheckMinMax();

        // Doesn't checks size, dummy function
        void DontCheckMinMax();

        // Beginning serialization callback, writes layout fields
        void OnSerialize(DataValue& node) const override;

        // Called when object was deserialized, reads layout fields
        void OnDeserialized(const DataValue& node) override;

        // Beginning serialization delta callback
        void OnSerializeDelta(DataValue& node, const IObject& origin) const override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

#if IS_SCRIPTING_SUPPORTED
    public:
        // Sets layout data? for scripting @SCRIPTABLE
        void Set(const WidgetLayout& other);
#endif

        friend class ContextMenu;
        friend class CustomDropDown;
        friend class GridLayout;
        friend class HorizontalLayout;
        friend class Label;
        friend class ScrollArea;
        friend class Spoiler;
        friend class Tree;
        friend class VerticalLayout;
        friend class Widget;
        friend class WidgetLayer;
        friend class Window;
    };

    // Calculates children widths or heights by weights and min/max sizes
    Vector<float> CalculateExpandedSize(Vector<Ref<Widget>>& widgets, bool horizontal, float availableWidth, float spacing);
}
// --- META ---

CLASS_BASES_META(o2::WidgetLayout)
{
    BASE_CLASS(o2::ActorTransform);
}
END_META;
CLASS_FIELDS_META(o2::WidgetLayout)
{
    FIELD().PUBLIC().NAME(anchorMin);
    FIELD().PUBLIC().NAME(anchorMax);
    FIELD().PUBLIC().NAME(offsetMin);
    FIELD().PUBLIC().NAME(offsetMax);
    FIELD().PUBLIC().NAME(anchorLeft);
    FIELD().PUBLIC().NAME(anchorRight);
    FIELD().PUBLIC().NAME(anchorBottom);
    FIELD().PUBLIC().NAME(anchorTop);
    FIELD().PUBLIC().NAME(offsetLeft);
    FIELD().PUBLIC().NAME(offsetRight);
    FIELD().PUBLIC().NAME(offsetBottom);
    FIELD().PUBLIC().NAME(offsetTop);
    FIELD().PUBLIC().NAME(minSize);
    FIELD().PUBLIC().NAME(minWidth);
    FIELD().PUBLIC().NAME(minHeight);
    FIELD().PUBLIC().NAME(maxSize);
    FIELD().PUBLIC().NAME(maxWidth);
    FIELD().PUBLIC().NAME(maxHeight);
    FIELD().PUBLIC().NAME(weight);
    FIELD().PUBLIC().NAME(widthWeight);
    FIELD().PUBLIC().NAME(heightWeight);
    FIELD().PROTECTED().DELTA_SEARCH_ATTRIBUTE().DEFAULT_VALUE(Vec2F(0, 0)).NAME(mAnchorMin);
    FIELD().PROTECTED().DELTA_SEARCH_ATTRIBUTE().DEFAULT_VALUE(Vec2F(0, 0)).NAME(mAnchorMax);
    FIELD().PROTECTED().DELTA_SEARCH_ATTRIBUTE().DEFAULT_VALUE(Vec2F(0, 0)).NAME(mOffsetMin);
    FIELD().PROTECTED().DELTA_SEARCH_ATTRIBUTE().DEFAULT_VALUE(Vec2F(10, 10)).NAME(mOffsetMax);
    FIELD().PROTECTED().DELTA_SEARCH_ATTRIBUTE().DEFAULT_VALUE(Vec2F(0, 0)).NAME(mMinSize);
    FIELD().PROTECTED().DELTA_SEARCH_ATTRIBUTE().DEFAULT_VALUE(Vec2F(10000, 10000)).NAME(mMaxSize);
    FIELD().PROTECTED().DELTA_SEARCH_ATTRIBUTE().DEFAULT_VALUE(Vec2F(1, 1)).NAME(mWeight);
    FIELD().PROTECTED().NAME(mChildrenWorldRect);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mDrivenByParent);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mOwnerWidget);
}
END_META;
CLASS_METHODS_META(o2::WidgetLayout)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vec2F&, const Vec2F&, const Vec2F&, const Vec2F&);
    FUNCTION().PUBLIC().CONSTRUCTOR(float, float, float, float, float, float, float, float);
    FUNCTION().PUBLIC().CONSTRUCTOR(const WidgetLayout&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetDirty, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, CopyFrom, const ActorTransform&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPosition2D, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSize2D, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWidth, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHeight, float);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetSize2D);
    FUNCTION().PUBLIC().SIGNATURE(float, GetWidth);
    FUNCTION().PUBLIC().SIGNATURE(float, GetHeight);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPivot2D, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBasis, const Basis&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetNonSizedBasis, const Basis&);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetRect);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRect, const RectF&);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetChildrenWorldRect);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAxisAlignedRect, const RectF&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetAnchorMin, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetAnchorMin);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetAnchorMax, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetAnchorMax);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetAnchorLeft, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetAnchorLeft);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetAnchorRight, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetAnchorRight);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetAnchorBottom, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetAnchorBottom);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetAnchorTop, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetAnchorTop);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetOffsetMin, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetOffsetMin);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetOffsetMax, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetOffsetMax);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetOffsetLeft, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetOffsetLeft);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetoffsetRight, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetoffsetRight);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetOffsetBottom, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetOffsetBottom);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetOffsetTop, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetOffsetTop);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetMinimalSize, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetMinimalSize);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetMinimalWidth, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetMinWidth);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetMinimalHeight, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetMinHeight);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetMaximalSize, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetMaximalSize);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetMaximalWidth, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetMaxWidth);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetMaximalHeight, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetMaxHeight);
    FUNCTION().PUBLIC().SIGNATURE(void, DisableSizeChecks);
    FUNCTION().PUBLIC().SIGNATURE(void, EnableSizeChecks);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWeight, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetWeight);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWidthWeight, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetWidthWeight);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetHeightWeight, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetHeightWeight);
    FUNCTION().PUBLIC().SIGNATURE(Basis, GetLayoutToWorldBasis);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPointInside, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, WorldToLayoutPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsWorldTransformPlain);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(void, SetSceneLayoutsRounding, bool);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsSceneLayoutsRounding);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE_STATIC(WidgetLayout, BothStretch, float, float, float, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE_STATIC(WidgetLayout, Based, BaseCorner, const Vec2F&, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE_STATIC(WidgetLayout, HorStretch, VerAlign, float, float, float, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE_STATIC(WidgetLayout, VerStretch, HorAlign, float, float, float, float);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsSerializeEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, SetOwner, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(RectF, GetParentRectangle);
    FUNCTION().PROTECTED().SIGNATURE(void, FloorRectangle);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateOffsetsByCurrentTransform);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckMinMax);
    FUNCTION().PROTECTED().SIGNATURE(void, DontCheckMinMax);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerializeDelta, DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
#if  IS_SCRIPTING_SUPPORTED
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Set, const WidgetLayout&);
#endif
}
END_META;
// --- END META ---
