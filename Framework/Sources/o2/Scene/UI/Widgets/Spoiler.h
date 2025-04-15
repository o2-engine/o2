#pragma once

#include "o2/Scene/UI/Widgets/VerticalLayout.h"

namespace o2
{
    class Button;

    // -------------------------------------
    // UI Spoiler. Can hide children widgets
    // -------------------------------------
    class Spoiler: public VerticalLayout
    {
    public:
        PROPERTIES(Spoiler);
        PROPERTY(WString, caption, SetCaption, GetCaption);        // Head caption property
        PROPERTY(float, headHeight, SetHeadHeight, GetHeadHeight); // Head height property
        PROPERTY(bool, expanded, SetExpanded, IsExpanded);         // Expanded state property

    public:
        Function<void()> onExpand;    // Expand starting event

    public:
        // Default constructor
        explicit Spoiler(RefCounter* refCounter);

        // Copy-constructor
        Spoiler(RefCounter* refCounter, const Spoiler& other);

        // Copy-operator
        Spoiler& operator=(const Spoiler& other);

        // Expands spoiler
        void Expand(bool forcible = false);

        // Collapses spoiler
        void Collapse(bool forcible = false);

        // Sets expanded state
        void SetExpanded(bool expand, bool forcible = false);

        // Returns is expanded
        bool IsExpanded() const;

        // Draws spoiler
        void Draw() override;

        // Sets caption, if text layer exist
        void SetCaption(const WString& caption);

        // Returns caption, if text layer exist
        const WString& GetCaption() const;

        // Sets head height
        void SetHeadHeight(float height);

        // Returns head's height
        float GetHeadHeight() const;

        // Searches expand button by name and type
        Ref<Button> GetExpandButton() const;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(Spoiler);
        CLONEABLE_REF(Spoiler);

    protected:
        float mHeadHeight = 0.0f; // Spoiler head height @SERIALIZABLE

        Ref<WidgetState> mExpandState;         // Expanding state
        float            mExpandCoef = 0.0f;   // Expanding animation coefficient 0...1 
        float            mTargetHeight = 0.0f; // target expanding height

    protected:
        // Invokes required function for childs arranging
        void RearrangeChilds() override;

        // Returns layout height
        float GetMinHeightWithChildren() const override;

        // Updates expanding
        void UpdateExpanding(float dt);

        // Creates expanding animation
        void CreateExpandAnimation();

        // Updates layout's weight and minimal size
        virtual void UpdateLayoutParametres() override;

        // Checks is expand button exist and sets click callback, searching caption layer
        void InitializeControls();

        // Returns is spoiler fully expanded and not animating
        bool IsFullyExpanded() const;

        // Returns is spoiler fully collapsed and not animating
        bool IsFullyCollapsed() const;
    };
}
// --- META ---

CLASS_BASES_META(o2::Spoiler)
{
    BASE_CLASS(o2::VerticalLayout);
}
END_META;
CLASS_FIELDS_META(o2::Spoiler)
{
    FIELD().PUBLIC().NAME(caption);
    FIELD().PUBLIC().NAME(headHeight);
    FIELD().PUBLIC().NAME(expanded);
    FIELD().PUBLIC().NAME(onExpand);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mHeadHeight);
    FIELD().PROTECTED().NAME(mExpandState);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mExpandCoef);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mTargetHeight);
}
END_META;
CLASS_METHODS_META(o2::Spoiler)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Spoiler&);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SetExpanded, bool, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsExpanded);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(const WString&, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHeadHeight, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetHeadHeight);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Button>, GetExpandButton);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, RearrangeChilds);
    FUNCTION().PROTECTED().SIGNATURE(float, GetMinHeightWithChildren);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateExpanding, float);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateExpandAnimation);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayoutParametres);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsFullyExpanded);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsFullyCollapsed);
}
END_META;
// --- END META ---
