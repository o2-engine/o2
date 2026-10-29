#pragma once

#include "o2/Render/TextureRef.h"
#include "o2/Utils/Singleton.h"

using namespace o2;

namespace o2
{
    class EditBox;
    class Label;
    class LongList;
    class Toggle;
    class Widget;
    class WidgetDragHandle;
    class Window;
}

namespace Editor
{
    FORWARD_CLASS_REF(TextureInspector);

    // ---------------------------------------------------------------------------------------------
    // Debug window of every texture alive in the engine: the files, the atlas pages and the ones made
    // from data (render targets, glyph atlases, vector rasters), filtered by name, sorted, with the
    // inspector; the list is as wide as the separator between them is dragged
    // ---------------------------------------------------------------------------------------------
    class TexturesWindow: public Singleton<TexturesWindow>
    {
    public:
        enum class SortBy { Order, Name, Size, Memory, Format };

    public:
        // Constructor
        TexturesWindow(RefCounter* refCounter);

        // Destructor
        ~TexturesWindow();

        // Shows the window with the textures alive now
        static void Show();

        // Lists the textures alive now
        void Refresh();

        // Selects the listed texture and shows it in the inspector
        void Select(int index);

        // Returns count of the listed textures, filtered
        int GetListedCount() const;

        // Returns listed texture by index; empty when it is gone
        TextureRef GetListed(int index) const;

        // Sets the filter of the names
        void SetFilter(const WString& filter);

        // Sorts the list; the order is the one the render keeps the textures in
        void SetSort(SortBy sortBy, bool descending = false);

        // Returns the key of the sorting
        SortBy GetSortBy() const;

        // Returns is the list sorted from the biggest
        bool IsSortDescending() const;

        // Sets the width of the list, kept in the window
        void SetListWidth(float width);

        // Returns the width of the list
        float GetListWidth() const;

        // Returns the sort toggles, in the order of the keys
        const Vector<Ref<Toggle>>& GetSortToggles() const;

        // Returns the line of the totals: count and memory
        WString GetSummaryText() const;

        // Returns the inspector
        const Ref<TextureInspector>& GetInspector() const;

        // Returns the window
        const Ref<o2::Window>& GetWindow() const;

    protected:
        Ref<o2::Window>         mWindow;      // Window
        Ref<Widget>             mListPanel;   // Filter, sorting, summary and list
        Ref<EditBox>            mFilter;      // Names filter
        Vector<Ref<Toggle>>     mSortToggles; // Sort keys, in the order of the keys
        Ref<Label>              mSummary;     // Count and memory of the listed textures
        Ref<LongList>           mList;        // Textures list
        Ref<WidgetDragHandle>   mSeparator;   // Separator between the list and the inspector
        Ref<TextureInspector>   mInspector;   // Selected texture

        Vector<WeakRef<Texture>> mTextures;   // Alive textures at the last refresh
        Vector<WeakRef<Texture>> mListed;     // Textures passed the filter, sorted
        WString                  mFilterText; // Names filter

        SortBy mSortBy = SortBy::Order; // Key of the sorting
        bool   mSortDescending = false; // Is the list sorted from the biggest
        float  mListWidth = 400.0f;     // Width of the list

    protected:
        // Builds the window
        void InitializeWindow();

        // Lists the textures that pass the filter, sorted
        void ApplyFilter();

        // Sorts the listed textures
        void SortListed();

        // Shows the direction of the sorting on the toggle of its key
        void UpdateSortCaptions();

        // Lays the list and the inspector out by the width of the list
        void UpdateListWidth();

        // Returns the line of a listed texture
        static WString GetLine(const TextureRef& texture);

        // Returns count of the list items
        int GetItemsCount();

        // Returns the list items in the range
        Vector<void*> GetItemsRange(int start, int end);

        // Sets up the list item widget
        void SetupItem(const Ref<Widget>& item, void* object);
    };
}
// --- META ---

PRE_ENUM_META(Editor::TexturesWindow::SortBy);
// --- END META ---
