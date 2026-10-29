#include "o2Editor/stdafx.h"
#include "TexturesWindow.h"

#include <algorithm>

#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/LongList.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2Editor/UI/Preview/PreviewToolbar.h"
#include "o2Editor/UI/Preview/TextureInspector.h"
#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2Editor/UIRoot.h"

DECLARE_SINGLETON(Editor::TexturesWindow);

namespace Editor
{
    namespace
    {
        const float minListWidth = 220.0f;
        const float minInspectorWidth = 300.0f;
        const float summaryHeight = 18.0f;
        const char* sortNames[] = { "Order", "Name", "Size", "Memory", "Format" };
    }

    TexturesWindow::TexturesWindow(RefCounter* refCounter):
        Singleton<TexturesWindow>(refCounter)
    {
        InitializeWindow();
    }

    TexturesWindow::~TexturesWindow()
    {
        if (mInstance == this)
            mInstance = nullptr;
    }

    void TexturesWindow::Show()
    {
        PushEditorScopeOnStack scope;

        if (!IsSingletonInitialzed())
            InitializeSingleton();

        auto& self = Instance();
        self.mWindow->Show();
        self.Refresh();
    }

    void TexturesWindow::InitializeWindow()
    {
        PushEditorScopeOnStack scope;

        mWindow = DynamicCast<o2::Window>(EditorUIRoot.AddWidget(o2UI.CreateWindow("Textures")));
        mWindow->name = "textures window";
        mWindow->SetClippingLayout(Layout::BothStretch(-1, -2, 0, 19));
        mWindow->SetViewLayout(Layout::BothStretch(-2, -2, 0, 20));
        mWindow->SetIcon(mmake<VectorSprite>("ui/UI4_image_component.svg"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 1)));

        mListPanel = mmake<Widget>();
        mListPanel->name = "list panel";
        mWindow->AddChild(mListPanel);

        auto toolbar = PreviewToolbar::Create();
        mListPanel->AddChild(toolbar);

        PreviewToolbar::AddLabel(toolbar, "Filter", 40.0f);

        mFilter = o2UI.CreateWidget<EditBox>("singleline");
        mFilter->name = "filter";
        mFilter->layout->minWidth = 60.0f;
        mFilter->layout->minHeight = PreviewToolbar::height;
        mFilter->onChanged = [this](const WString& text) { SetFilter(text); };
        toolbar->AddChild(mFilter);

        PreviewToolbar::AddButton(toolbar, "Refresh", 64.0f, [this]() { Refresh(); });

        auto sortBar = PreviewToolbar::Create("sort");
        *sortBar->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, PreviewToolbar::height, PreviewToolbar::height);
        mListPanel->AddChild(sortBar);

        PreviewToolbar::AddLabel(sortBar, "Sort", 40.0f);
        // The toggles are as wide as their captions with the direction mark
        Vector<WString> sortCaptions;
        for (auto name : sortNames)
            sortCaptions.Add(WString(name) + L" \u02C5");

        // A click on the key sorted by turns the direction
        mSortToggles = PreviewToolbar::AddChoice(sortBar, sortCaptions, { 0.0f }, [this](int position) {
            SortBy sortBy = (SortBy)position;
            SetSort(sortBy, sortBy == mSortBy ? !mSortDescending : sortBy == SortBy::Size || sortBy == SortBy::Memory);
        });
        PreviewToolbar::AddLabel(sortBar, "");

        mSummary = o2UI.CreateLabel("");
        mSummary->name = "summary";
        mSummary->horAlign = HorAlign::Left;
        mSummary->horOverflow = Label::HorOverflow::Cut;
        *mSummary->layout = WidgetLayout::HorStretch(VerAlign::Top, 4, 4, summaryHeight, PreviewToolbar::height*2.0f);
        mListPanel->AddChild(mSummary);

        mList = o2UI.CreateWidget<LongList>();
        mList->name = "textures list";
        *mList->layout = WidgetLayout::BothStretch(0, 0, 0, PreviewToolbar::height*2.0f + summaryHeight);
        mList->SetViewLayout(Layout::BothStretch());
        mList->getItemsCountFunc = [this]() { return GetItemsCount(); };
        mList->getItemsRangeFunc = [this](int start, int end) { return GetItemsRange(start, end); };
        mList->setupItemFunc = [this](const Ref<Widget>& item, void* object) { SetupItem(item, object); };
        mList->onFocused = [this](int index) { Select(index); };

        auto& palette = EditorTheme::Get();
        auto sample = mmake<Widget>();
        sample->layout->minHeight = 20;
        auto caption = mmake<Text>("stdFont.ttf");
        caption->text = "texture";
        caption->horAlign = HorAlign::Left;
        caption->verAlign = VerAlign::Middle;
        caption->wordWrap = false;
        caption->color = palette.text;
        sample->AddLayer("caption", caption, Layout::BothStretch(6, 0, 4, 0));
        mList->SetItemSample(sample);
        mListPanel->AddChild(mList);

        mInspector = mmake<TextureInspector>();
        mInspector->name = "inspector";
        mWindow->AddChild(mInspector);

        mSeparator = mmake<WidgetDragHandle>(mmake<VectorSprite>("ui/UI4_Ver_separator.svg"));
        mSeparator->name = "separator";
        mSeparator->GetRegularDrawable()->pivot = Vec2F(0.5f, 0.5f);
        mSeparator->GetRegularDrawable()->szPivot = Vec2F(4, mSeparator->GetRegularDrawable()->szPivot.Get().y);
        mSeparator->cursorType = CursorType::SizeWE;
        mSeparator->checkPositionFunc = [this](const Vec2F& point) {
            return Vec2F(point.x, mWindow->layout->GetHeight()*0.5f);
        };
        mSeparator->onChangedPos = [this](const Vec2F& point) { SetListWidth(point.x); };
        mSeparator->onLayoutUpdated = [this]() {
            mSeparator->SetDrawablesSize(Vec2F(5.0f, mWindow->layout->GetHeight() + 50.0f));
            mSeparator->SetPosition(Vec2F(mListWidth + 2.0f, mWindow->layout->GetHeight()*0.5f));
        };
        mWindow->AddChild(mSeparator);

        UpdateListWidth();
        UpdateSortCaptions();

        mWindow->Hide(true);
        mWindow->layout->size2D = Vec2F(1300, 820);
    }

    void TexturesWindow::Refresh()
    {
        mTextures.Clear();
        for (auto& texture : o2Render.GetTextures())
            mTextures.Add(WeakRef<Texture>(texture.Get()));

        ApplyFilter();
    }

    void TexturesWindow::SetFilter(const WString& filter)
    {
        mFilterText = filter;
        ApplyFilter();
    }

    void TexturesWindow::ApplyFilter()
    {
        PushEditorScopeOnStack scope;

        mListed.Clear();

        UInt64 memory = 0;
        String filter = ((String)mFilterText).ToLowerCase();
        for (auto& weak : mTextures)
        {
            auto texture = weak.Lock();
            if (!texture)
                continue;

            if (!filter.IsEmpty() && !TextureView::GetTextureName(TextureRef(texture)).ToLowerCase().Contains(filter))
                continue;

            mListed.Add(weak);
            memory += TextureView::GetTextureMemory(TextureRef(texture));
        }

        SortListed();

        mSummary->SetText((String)mListed.Count() + " of " + (String)mTextures.Count() + " textures, " +
                          TextureView::FormatBytes(memory));
        mList->OnItemsUpdated(true);
    }

    void TexturesWindow::SortListed()
    {
        if (mSortBy == SortBy::Order)
        {
            if (mSortDescending)
                mListed.Reverse();

            return;
        }

        struct Entry
        {
            WeakRef<Texture> texture;
            int              order = 0;
            String           name;
            UInt64           value = 0;
        };

        Vector<Entry> entries;
        for (int i = 0; i < mListed.Count(); i++)
        {
            Entry entry;
            entry.texture = mListed[i];
            entry.order = i;

            auto locked = mListed[i].Lock();
            if (!locked)
            {
                entries.Add(entry);
                continue;
            }

            auto texture = TextureRef(locked);
            Vec2I size = texture->GetSize();
            switch (mSortBy)
            {
            case SortBy::Name:   entry.name = TextureView::GetTextureName(texture).ToLowerCase(); break;
            case SortBy::Size:   entry.value = (UInt64)size.x*(UInt64)size.y; break;
            case SortBy::Memory: entry.value = TextureView::GetTextureMemory(texture); break;
            case SortBy::Format: entry.name = TextureView::GetFormatName(texture->GetFormat()); break;
            default: break;
            }

            entries.Add(entry);
        }

        bool descending = mSortDescending;
        bool byName = mSortBy == SortBy::Name || mSortBy == SortBy::Format;
        std::stable_sort(entries.begin(), entries.end(), [descending, byName](const Entry& a, const Entry& b) {
            if (byName ? a.name == b.name : a.value == b.value)
                return a.order < b.order;

            bool less = byName ? a.name < b.name : a.value < b.value;
            return descending ? !less : less;
        });

        mListed.Clear();
        for (auto& entry : entries)
            mListed.Add(entry.texture);
    }

    void TexturesWindow::SetSort(SortBy sortBy, bool descending /*= false*/)
    {
        mSortBy = sortBy;
        mSortDescending = descending;

        PreviewToolbar::SetChoice(mSortToggles, (int)sortBy);
        UpdateSortCaptions();
        ApplyFilter();
    }

    TexturesWindow::SortBy TexturesWindow::GetSortBy() const
    {
        return mSortBy;
    }

    bool TexturesWindow::IsSortDescending() const
    {
        return mSortDescending;
    }

    void TexturesWindow::UpdateSortCaptions()
    {
        for (int i = 0; i < mSortToggles.Count(); i++)
        {
            WString caption = sortNames[i];
            if (i == (int)mSortBy)
                caption += mSortDescending ? L" \u02C5" : L" \u02C4";

            mSortToggles[i]->caption = caption;
        }
    }

    void TexturesWindow::SetListWidth(float width)
    {
        // A window not laid out yet has no width to keep for the inspector
        float windowWidth = mWindow->layout->GetWidth();
        float maxWidth = windowWidth > 1.0f ? Math::Max(minListWidth, windowWidth - minInspectorWidth) : width;
        mListWidth = Math::Max(minListWidth, Math::Min(width, maxWidth));
        UpdateListWidth();
    }

    float TexturesWindow::GetListWidth() const
    {
        return mListWidth;
    }

    const Vector<Ref<Toggle>>& TexturesWindow::GetSortToggles() const
    {
        return mSortToggles;
    }

    void TexturesWindow::UpdateListWidth()
    {
        *mListPanel->layout = WidgetLayout::VerStretch(HorAlign::Left, 0, 0, mListWidth, 0);
        *mInspector->layout = WidgetLayout::BothStretch(mListWidth + 4.0f, 0, 0, 0);
        mSeparator->SetPosition(Vec2F(mListWidth + 2.0f, mWindow->layout->GetHeight()*0.5f));
    }

    void TexturesWindow::Select(int index)
    {
        auto texture = GetListed(index);
        if (!texture)
            return;

        if (mList->GetSelectedItemPosition() != index)
            mList->SelectItemAt(index);

        mInspector->SetNote(TextureView::GetTextureName(texture));
        mInspector->SetTexture(texture);
    }

    int TexturesWindow::GetListedCount() const
    {
        return mListed.Count();
    }

    TextureRef TexturesWindow::GetListed(int index) const
    {
        if (index < 0 || index >= mListed.Count())
            return TextureRef();

        auto texture = mListed[index].Lock();
        return texture ? TextureRef(texture) : TextureRef();
    }

    WString TexturesWindow::GetSummaryText() const
    {
        return mSummary->GetText();
    }

    const Ref<TextureInspector>& TexturesWindow::GetInspector() const
    {
        return mInspector;
    }

    const Ref<o2::Window>& TexturesWindow::GetWindow() const
    {
        return mWindow;
    }

    WString TexturesWindow::GetLine(const TextureRef& texture)
    {
        Vec2I size = texture->GetSize();
        return TextureView::GetTextureName(texture) + "  " + (String)size.x + "x" + (String)size.y + " " +
            TextureView::GetFormatName(texture->GetFormat()) + ", " + TextureView::FormatBytes(TextureView::GetTextureMemory(texture));
    }

    int TexturesWindow::GetItemsCount()
    {
        return mListed.Count();
    }

    Vector<void*> TexturesWindow::GetItemsRange(int start, int end)
    {
        Vector<void*> items;
        for (int i = start; i < end && i < mListed.Count(); i++)
            items.Add((void*)(size_t)(i + 1));

        return items;
    }

    void TexturesWindow::SetupItem(const Ref<Widget>& item, void* object)
    {
        int index = (int)(size_t)object - 1;
        auto texture = GetListed(index);
        auto caption = item->GetLayerDrawable<Text>("caption");
        caption->text = texture ? GetLine(texture) : WString("(released)");
    }
}
// --- META ---

ENUM_META(Editor::TexturesWindow::SortBy, Editor__TexturesWindow__SortBy)
{
    ENUM_ENTRY(Format);
    ENUM_ENTRY(Memory);
    ENUM_ENTRY(Name);
    ENUM_ENTRY(Order);
    ENUM_ENTRY(Size);
}
END_ENUM_META;
// --- END META ---
