#pragma once

#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Function/Function.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"

using namespace o2;

namespace o2
{
    class EditBox;
    class HorizontalLayout;
    class Label;
    class ScrollArea;
    class Toggle;
    class VerticalLayout;
}

namespace Editor
{
    // ---------------------------------------------------------------------------
    // Node palette panel of the pipeline window: a search box, a tab per category
    // and the list of node types. Picking one calls onPick; the window puts the
    // node in the middle of the view. The right-click menu stays the way to place
    // a node at a particular spot
    // ---------------------------------------------------------------------------
    class PipelineNodePalette : public Widget
    {
    public:
        Function<void(const String&)> onPick; // Called with the node type picked in the list

    public:
        // Default constructor
        explicit PipelineNodePalette(RefCounter* refCounter);

        // Clears the search, shows all categories and focuses the search box
        void Reset();

        // Returns node types the list currently shows, in list order
        Vector<String> GetShownTypes() const;

        SERIALIZABLE(PipelineNodePalette);

    protected:
        Ref<EditBox>        mFilter;    // Search box filtering by name and description
        Ref<HorizontalLayout> mTabs;    // Row of category tabs
        Ref<ScrollArea>     mScroll;    // Scroll of the node list
        Ref<VerticalLayout> mList;      // List of node entries
        Ref<Label>          mEmptyHint; // Shown instead of the list when nothing matches

        Vector<Ref<Toggle>>          mTabToggles;    // Category tabs, in the order of mCategories
        Vector<PipelineNodeCategory> mCategories;    // Categories that have node types
        int                          mCategory = -1; // Open category, -1 for all of them

        Vector<String> mShownTypes; // Node types of the entries currently in the list

    protected:
        // Builds the search box, the category tabs and the empty list
        void InitializeControls();

        // Fills the list with the node types matching the query and the open category
        void RebuildList();

        // Returns true when the query is empty or is found in the type, the name or the description
        static bool Matches(const PipelineNodeSchema* schema, const String& query);
    };
}
// --- META ---

CLASS_BASES_META(Editor::PipelineNodePalette)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineNodePalette)
{
    FIELD().PUBLIC().NAME(onPick);
    FIELD().PROTECTED().NAME(mFilter);
    FIELD().PROTECTED().NAME(mTabs);
    FIELD().PROTECTED().NAME(mScroll);
    FIELD().PROTECTED().NAME(mList);
    FIELD().PROTECTED().NAME(mEmptyHint);
    FIELD().PROTECTED().NAME(mTabToggles);
    FIELD().PROTECTED().NAME(mCategories);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mCategory);
    FIELD().PROTECTED().NAME(mShownTypes);
}
END_META;
CLASS_METHODS_META(Editor::PipelineNodePalette)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Reset);
    FUNCTION().PUBLIC().SIGNATURE(Vector<String>, GetShownTypes);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildList);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(bool, Matches, const PipelineNodeSchema*, const String&);
}
END_META;
// --- END META ---
