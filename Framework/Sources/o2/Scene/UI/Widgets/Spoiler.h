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
		Spoiler();

		// Copy-constructor
		Spoiler(const Spoiler& other);

		// Copy-operator
		Spoiler& operator=(const Spoiler& other);

		// Expands spoiler
		void Expand();

		// Collapses spoiler
		void Collapse();

		// Sets expanded state
		void SetExpanded(bool expand);

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
		Button* GetExpandButton() const;

		// Returns create menu group in editor
		static String GetCreateMenuGroup();

		SERIALIZABLE(Spoiler);

	protected:
		float mHeadHeight = 0.0f; // Spoiler head height @SERIALIZABLE

		WidgetState* mExpandState = nullptr; // Expanding state
		float        mExpandCoef = 0.0f;     // Expanding animation coefficient 0...1 
		float        mTargetHeight = 0.0f;   // target expanding height

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

CLASS_BASES_META(o2::Spoiler)
{
	BASE_CLASS(o2::VerticalLayout);
}
END_META;
CLASS_FIELDS_META(o2::Spoiler)
{
	FIELD().NAME(caption).PUBLIC();
	FIELD().NAME(headHeight).PUBLIC();
	FIELD().NAME(expanded).PUBLIC();
	FIELD().NAME(onExpand).PUBLIC();
	FIELD().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mHeadHeight).PROTECTED();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mExpandState).PROTECTED();
	FIELD().DEFAULT_VALUE(0.0f).NAME(mExpandCoef).PROTECTED();
	FIELD().DEFAULT_VALUE(0.0f).NAME(mTargetHeight).PROTECTED();
}
END_META;
CLASS_METHODS_META(o2::Spoiler)
{

	PUBLIC_FUNCTION(void, Expand);
	PUBLIC_FUNCTION(void, Collapse);
	PUBLIC_FUNCTION(void, SetExpanded, bool);
	PUBLIC_FUNCTION(bool, IsExpanded);
	PUBLIC_FUNCTION(void, Draw);
	PUBLIC_FUNCTION(void, SetCaption, const WString&);
	PUBLIC_FUNCTION(const WString&, GetCaption);
	PUBLIC_FUNCTION(void, SetHeadHeight, float);
	PUBLIC_FUNCTION(float, GetHeadHeight);
	PUBLIC_FUNCTION(Button*, GetExpandButton);
	PUBLIC_STATIC_FUNCTION(String, GetCreateMenuGroup);
	PROTECTED_FUNCTION(void, RearrangeChilds);
	PROTECTED_FUNCTION(float, GetMinHeightWithChildren);
	PROTECTED_FUNCTION(void, UpdateExpanding, float);
	PROTECTED_FUNCTION(void, CreateExpandAnimation);
	PROTECTED_FUNCTION(void, UpdateLayoutParametres);
	PROTECTED_FUNCTION(void, InitializeControls);
	PROTECTED_FUNCTION(bool, IsFullyExpanded);
	PROTECTED_FUNCTION(bool, IsFullyCollapsed);
}
END_META;
