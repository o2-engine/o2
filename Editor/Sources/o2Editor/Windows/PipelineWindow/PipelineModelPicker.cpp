#include "o2Editor/stdafx.h"
#include "PipelineModelPicker.h"

#include "o2/Animation/AnimationClip.h"
#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"

namespace Editor
{
    static const float padding = 6.0f;
    static const float searchHeight = 22.0f;
    static const float chipHeight = 20.0f;
    static const float chipGap = 4.0f;
    static const float sectionGap = 6.0f;
    static const float headerHeight = 20.0f;
    static const float rowHeight = 22.0f;
    static const float groupGap = 2.0f;
    static const float scrollBarWidth = 10.0f;
    static const float minMenuWidth = 320.0f;
    static const float maxMenuHeight = 440.0f;
    static const float minMenuHeight = 160.0f;
    static const float screenMargin = 8.0f;
    static const float fieldGap = 4.0f;

    static Ref<Text> MakeText(const String& text, const Color4& color, HorAlign align, int height = 11)
    {
        auto drawable = mmake<Text>("stdFont.ttf");
        drawable->text = text;
        drawable->color = color;
        drawable->horAlign = align;
        drawable->verAlign = VerAlign::Middle;
        drawable->wordWrap = false;
        drawable->dotsEngings = true;
        drawable->height = height;
        return drawable;
    }

    PipelineModelPicker::PipelineModelPicker(RefCounter* refCounter):
        PopupWidget(refCounter)
    {
        fitByChildren = false;
        InitializeControls();
    }

    void PipelineModelPicker::InitializeControls()
    {
        PushEditorScopeOnStack scope;

        AddLayer("back", mmake<Sprite>("ui/UI4_Context_menu.png"), Layout::BothStretch(-20, -19, -20, -19));
        SetViewLayout(Layout::BothStretch(0, 0, 0, 0));
        SetClippingLayout(Layout::BothStretch(0, 0, 0, 0));

        mMeasure = mmake<Text>("stdFont.ttf");

        mSearchPanel = mmake<Widget>();
        mSearchPanel->name = "search panel";
        mSearchPanel->AddLayer("icon", mmake<Sprite>("ui/UI4_search_regular.png"), Layout::Based(BaseCorner::Left, Vec2F(20, 20)));
        *mSearchPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, padding, padding, searchHeight, padding);
        AddChild(mSearchPanel);

        mSearch = o2UI.CreateEditBox("singleline");
        mSearch->name = "search";
        *mSearch->layout = WidgetLayout::BothStretch(22, 0, 0, 0);
        mPlaceholder = mSearch->AddLayer("placeholder", MakeText("Search models", PipelineControls::dimTextColor, HorAlign::Left),
                                         Layout::BothStretch(6, 0, 6, 0), 1.0f);
        mSearch->onChanged = [this](const WString& text)
        {
            mPlaceholder->transparency = text.IsEmpty() ? 1.0f : 0.0f;
            if (mRefilling)
                return;

            mFilter.query = (String)text;
            Refilter(true);
        };
        mSearchPanel->AddChild(mSearch);

        mChips = mmake<Widget>();
        mChips->name = "chips";
        AddChild(mChips);

        mSeparator = mmake<Widget>();
        mSeparator->name = "separator";
        mSeparator->AddLayer("line", mmake<Sprite>("ui/UI4_Separator.png"), Layout::HorStretch(VerAlign::Middle, 0, 0, 5, 0));
        AddChild(mSeparator);

        mList = mmake<ScrollArea>();
        mList->name = "list";
        mList->SetClippingLayout(Layout::BothStretch(0, 0, 0, 0));
        mList->SetViewLayout(Layout::BothStretch(0, 0, scrollBarWidth, 0));
        auto bar = o2UI.CreateVerScrollBar();
        bar->layout->anchorMin = Vec2F(1, 0);
        bar->layout->anchorMax = Vec2F(1, 1);
        bar->layout->offsetMin = Vec2F(-scrollBarWidth, 0);
        bar->layout->offsetMax = Vec2F(0, 0);
        mList->SetVerticalScrollBar(bar);
        AddChild(mList);

        mListContent = mmake<Widget>();
        mListContent->name = "rows";
        mList->AddChild(mListContent);
    }

    void PipelineModelPicker::Open(const PipelineModelPickerRequest& request)
    {
        PushEditorScopeOnStack scope;

        mRequest = request;
        mToggled.Clear();
        mFilter = PipelineMenuFilter();
        mPendingToggle = "";
        mClosedByField = nullptr;
        mClosedByFieldTime = -1.0f;
        mGroups = PipelineModelMenu::BuildGroups(request.ids, request.kind, request.current,
                                                 PipelineModelMenu::LoadRecent(request.kind));

        Vec2F resolution = (Vec2F)o2Render.GetResolution();
        mWidth = Math::Min(resolution.x - screenMargin*2.0f, Math::Max(request.anchor.Width(), minMenuWidth));

        mRefilling = true;
        mSearch->SetText("");
        mRefilling = false;
        mPlaceholder->transparency = 1.0f;

        RebuildChips();
        mVisible = PipelineModelMenu::VisibleGroups(mGroups, mFilter, mRequest.current, mToggled);
        mCustomId = "";
        mPickable = PipelineModelMenu::PickableIds(mVisible, mCustomId);
        RebuildRows();

        // Below the field when the menu fits there or there is more room below than above
        RectF screen(-resolution.x*0.5f, resolution.y*0.5f, resolution.x*0.5f, -resolution.y*0.5f);
        float needed = Math::Min(maxMenuHeight, GetHeaderHeight() + Math::Max(mContentHeight, rowHeight) + padding);
        float below = request.anchor.bottom - fieldGap - (screen.bottom + screenMargin);
        float above = (screen.top - screenMargin) - (request.anchor.top + fieldGap);
        mOpensDown = below >= needed || below >= above;

        PopupWidget::Show(Vec2F());

        mHighlight = PipelineModelMenu::InitialHighlight(mPickable, mRequest.current);
        UpdateHighlight();
        ScrollToRow(mHighlight, true);

        o2UI.FocusWidget(mSearch);
    }

    void PipelineModelPicker::Close()
    {
        if (IsOpen())
            HideWithParent();
    }

    bool PipelineModelPicker::IsOpen() const
    {
        return IsEnabled();
    }

    bool PipelineModelPicker::IsOpenFor(const Ref<Widget>& field) const
    {
        return IsOpen() && field && mRequest.field.Lock() == field;
    }

    bool PipelineModelPicker::ConsumeClosedByField(const Ref<Widget>& field)
    {
        // The click ends the press that closed the menu: it follows within moments, never after a new press
        bool result = field && mClosedByField.Lock() == field && mClosedByFieldTime >= 0.0f &&
            o2Time.GetApplicationTime() - mClosedByFieldTime < 2.0f;

        mClosedByField = nullptr;
        mClosedByFieldTime = -1.0f;
        return result;
    }

    void PipelineModelPicker::Pick(const String& id)
    {
        if (id.IsEmpty())
            return;

        auto request = mRequest;
        PipelineModelMenu::RememberPick(request.kind, id);
        Close();
        if (request.onPick)
            request.onPick(id);
    }

    void PipelineModelPicker::PickHighlighted()
    {
        Pick(GetHighlightedId());
    }

    void PipelineModelPicker::MoveHighlight(int step)
    {
        int count = mPickable.Count();
        if (count == 0)
            return;

        mHighlight = ((mHighlight + step) % count + count) % count;
        UpdateHighlight();
        ScrollToRow(mHighlight, false);
    }

    void PipelineModelPicker::SetProviderFilter(bool anyProvider, PipelineModelProvider provider)
    {
        mFilter.anyProvider = anyProvider;
        mFilter.provider = provider;
        RebuildChips();
        Refilter(true);
    }

    void PipelineModelPicker::SetAlphaOnly(bool alphaOnly)
    {
        mFilter.alphaOnly = alphaOnly;
        RebuildChips();
        Refilter(true);
    }

    void PipelineModelPicker::ToggleGroup(const String& key)
    {
        auto group = mVisible.FindOrDefault([&](const PipelineVisibleGroup& g) { return g.group.key == key; });
        if (group.group.key.IsEmpty())
            return;

        mToggled[key] = !group.folded;
        Refilter(false);
    }

    String PipelineModelPicker::GetHighlightedId() const
    {
        return mHighlight >= 0 && mHighlight < mPickable.Count() ? mPickable[mHighlight] : String();
    }

    Ref<Button> PipelineModelPicker::FindRow(const String& id) const
    {
        int index = mPickable.IndexOf(id);
        return index >= 0 && index < mRows.Count() ? mRows[index] : nullptr;
    }

    Ref<Button> PipelineModelPicker::FindHeader(const String& key) const
    {
        auto header = mHeaders.FindOrDefault([&](const Pair<String, Ref<Button>>& h) { return h.first == key; });
        return header.second;
    }

    bool PipelineModelPicker::IsRowInView(int index) const
    {
        if (index < 0 || index >= mRows.Count())
            return false;

        RectF view = mList->layout->GetWorldRect();
        RectF row = mRows[index]->layout->GetWorldRect();
        return row.top <= view.top + 0.5f && row.bottom >= view.bottom - 0.5f;
    }

    void PipelineModelPicker::Update(float dt)
    {
        if (mResEnabledInHierarchy)
        {
            Vec2F cursor = o2Input.GetCursorPos();
            bool inside = layout->IsPointInside(cursor);

            // A press on the field closes the menu here; the click it ends must not open it again
            bool pressed = o2Input.IsCursorPressed() || o2Input.IsRightMousePressed();
            if (pressed && !inside && !mShownAtFrame)
            {
                mClosedByField = mRequest.field;
                mClosedByFieldTime = mRequest.anchor.IsInside(cursor) ? o2Time.GetApplicationTime() : -1.0f;
            }

            // The canvas zooms under a wheel outside, and the menu would stay behind its field
            if (!inside && !mShownAtFrame && Math::Abs(o2Input.GetMouseWheelDelta()) > 0.1f)
            {
                Close();
                return;
            }

            if (!mPendingToggle.IsEmpty())
            {
                String key = mPendingToggle;
                mPendingToggle = "";
                ToggleGroup(key);
            }

            if (o2Input.GetCursorDelta() != Vec2F() && mList->layout->IsPointInside(cursor))
            {
                for (int i = 0; i < mRows.Count(); i++)
                {
                    if (mRows[i]->layout->IsPointInside(cursor))
                    {
                        if (mHighlight != i)
                        {
                            mHighlight = i;
                            UpdateHighlight();
                        }
                        break;
                    }
                }
            }
        }

        PopupWidget::Update(dt);
    }

    void PipelineModelPicker::OnKeyPressed(const Input::Key& key)
    {
        if (!IsOpen())
            return;

        if (key.keyCode == VK_ESCAPE)
            Close();
        else if (key.keyCode == VK_UP)
            MoveHighlight(-1);
        else if (key.keyCode == VK_DOWN)
            MoveHighlight(1);
        else if (key.keyCode == VK_RETURN)
            PickHighlighted();
    }

    void PipelineModelPicker::OnKeyStayDown(const Input::Key& key)
    {
        if (!IsOpen() || !o2Input.IsKeyRepeating(key.keyCode))
            return;

        if (key.keyCode == VK_UP)
            MoveHighlight(-1);
        else if (key.keyCode == VK_DOWN)
            MoveHighlight(1);
    }

    void PipelineModelPicker::OnDisabled()
    {
        PopupWidget::OnDisabled();

        bool ownFocus = false;
        for (Ref<Widget> widget = o2UI.GetFocusedWidget(); widget; widget = widget->GetParentWidget().Lock())
        {
            if (widget.Get() == this)
            {
                ownFocus = true;
                break;
            }
        }

        if (ownFocus)
        {
            auto owner = GetParentWidget().Lock();
            o2UI.FocusWidget(owner && owner->IsFocusable() ? owner : nullptr);
        }
    }

    void PipelineModelPicker::OnCursorReleasedOutside(const Input::Cursor& cursor)
    {}

    void PipelineModelPicker::OnCursorPressBreak(const Input::Cursor& cursor)
    {}

    void PipelineModelPicker::FitSizeAndPosition(const Vec2F& position)
    {
        Place();
    }

    void PipelineModelPicker::RebuildChips()
    {
        PushEditorScopeOnStack scope;

        mChips->RemoveAllChildren();

        struct Chip { String name; String caption; bool on; Function<void()> onClick; };
        Vector<Chip> chips;

        auto providers = PipelineModelMenu::ProvidersOf(mGroups);
        if (providers.Count() > 1)
        {
            chips.Add({ "chip all", "All", mFilter.anyProvider, [this]() { SetProviderFilter(true, mFilter.provider); } });
            for (auto provider : providers)
            {
                chips.Add({ "chip " + PipelineModelMenu::ProviderKey(provider), PipelineModelMenu::ChipLabel(provider),
                            !mFilter.anyProvider && mFilter.provider == provider,
                            [this, provider]() { SetProviderFilter(false, provider); } });
            }
        }

        if (mRequest.kind == PipelineModelKind::Image && PipelineModelMenu::HasAlpha(mGroups))
            chips.Add({ "chip alpha", "\xCE\xB1 Transparent bg", mFilter.alphaOnly, [this]() { SetAlphaOnly(!mFilter.alphaOnly); } });

        float available = mWidth - padding*2.0f;
        float x = 0.0f, y = 0.0f;
        for (auto& chip : chips)
        {
            float width = Math::Min(available, MeasureText(chip.caption, 11) + 16.0f);
            if (x > 0.0f && x + width > available)
            {
                x = 0.0f;
                y += chipHeight + chipGap;
            }

            auto toggle = PipelineControls::MakeSegment(chip.caption, chip.on);
            toggle->name = chip.name;
            *toggle->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(width, chipHeight), Vec2F(x, -y));
            auto onClick = chip.onClick;
            toggle->onToggleByUser = [this, onClick](bool)
            {
                onClick();
                o2UI.FocusWidget(mSearch);
            };
            mChips->AddChild(toggle);
            x += width + chipGap;
        }

        mChipsHeight = chips.IsEmpty() ? 0.0f : y + chipHeight;
        mChips->SetEnabledForcible(!chips.IsEmpty());
    }

    void PipelineModelPicker::Refilter(bool resetHighlight)
    {
        String keep = resetHighlight ? String() : GetHighlightedId();
        float scroll = mList->GetScroll().y;

        mVisible = PipelineModelMenu::VisibleGroups(mGroups, mFilter, mRequest.current, mToggled);
        mCustomId = PipelineModelMenu::CustomIdFor(mFilter.query, mGroups);
        mPickable = PipelineModelMenu::PickableIds(mVisible, mCustomId);
        RebuildRows();
        Place();

        if (resetHighlight)
        {
            // A new filter starts from the row of the exact id typed, else from its first row
            mHighlight = PipelineModelMenu::HighlightFor(mFilter.query, PipelineModelMenu::PickableIds(mVisible, ""));
            UpdateHighlight();
            mList->SetScrollForcible(Vec2F());
            ScrollToRow(mHighlight, false);
            return;
        }

        // A folded or unfolded group leaves the list where it was
        int kept = mPickable.IndexOf(keep);
        mHighlight = kept >= 0 ? kept : 0;
        UpdateHighlight();
        mList->SetScrollForcible(Vec2F(0.0f, scroll));
    }

    void PipelineModelPicker::RebuildRows()
    {
        PushEditorScopeOnStack scope;

        mListContent->RemoveAllChildren();
        mRows.Clear();
        mRowTops.Clear();
        mRowHeights.Clear();
        mGroupTops.Clear();
        mHeaders.Clear();

        float width = mWidth - padding*2.0f - scrollBarWidth;
        float y = 0.0f;
        auto place = [&](const Ref<Widget>& widget, float height)
        {
            *widget->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, height, y);
            mListContent->AddChild(widget);
            y += height;
        };

        auto addRow = [&](const Ref<Button>& row, float groupTop)
        {
            mRows.Add(row);
            mRowTops.Add(y);
            mRowHeights.Add(rowHeight);
            mGroupTops.Add(groupTop);
            place(row, rowHeight);
        };

        for (int i = 0; i < mVisible.Count(); i++)
        {
            auto& group = mVisible[i];
            if (i > 0)
                y += groupGap;

            float groupTop = y;
            auto header = MakeHeader(group, width);
            mHeaders.Add(Pair<String, Ref<Button>>(group.group.key, header));
            place(header, headerHeight);

            if (group.folded)
                continue;

            for (auto& model : group.shown)
                addRow(MakeModelRow(model, width), groupTop);
        }

        if (!mCustomId.IsEmpty())
        {
            if (!mVisible.IsEmpty())
                y += groupGap;

            addRow(MakeCustomRow(mCustomId), y);
        }

        if (mVisible.IsEmpty() && mCustomId.IsEmpty())
        {
            bool filtered = !mFilter.query.Trimed(" \n\r\t").IsEmpty() || !mFilter.anyProvider || mFilter.alphaOnly;
            auto empty = PipelineControls::MakeLabel(filtered ? "No model matches" : "No models", true);
            empty->name = "empty";
            empty->horAlign = HorAlign::Middle;
            place(empty, rowHeight);
        }

        mContentHeight = y;
        *mListContent->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, Math::Max(y, 1.0f), 0);
    }

    float PipelineModelPicker::GetHeaderHeight() const
    {
        return padding + searchHeight + sectionGap + (mChipsHeight > 0.0f ? mChipsHeight + sectionGap : 0.0f);
    }

    void PipelineModelPicker::Place()
    {
        Vec2F resolution = (Vec2F)o2Render.GetResolution();
        RectF screen(-resolution.x*0.5f, resolution.y*0.5f, resolution.x*0.5f, -resolution.y*0.5f);
        const RectF& anchor = mRequest.anchor;

        float headerPart = GetHeaderHeight();
        float needed = Math::Min(maxMenuHeight, headerPart + Math::Max(mContentHeight, rowHeight) + padding);
        float below = anchor.bottom - fieldGap - (screen.bottom + screenMargin);
        float above = (screen.top - screenMargin) - (anchor.top + fieldGap);
        float room = mOpensDown ? below : above;
        float height = Math::Min(needed, Math::Max(room, Math::Min(needed, minMenuHeight)));
        height = Math::Min(height, resolution.y - screenMargin*2.0f);

        float left = Math::Clamp(anchor.left, screen.left + screenMargin, screen.right - screenMargin - mWidth);
        float top = mOpensDown ? anchor.bottom - fieldGap : anchor.top + fieldGap + height;
        top = Math::Min(top, screen.top - screenMargin);
        top = Math::Max(top, screen.bottom + screenMargin + height);

        layout->worldRect = RectF(Math::Round(left), Math::Round(top), Math::Round(left + mWidth), Math::Round(top - height));

        *mChips->layout = WidgetLayout::HorStretch(VerAlign::Top, padding, padding, Math::Max(mChipsHeight, 1.0f),
                                                   padding + searchHeight + sectionGap);
        *mSeparator->layout = WidgetLayout::HorStretch(VerAlign::Top, padding, padding, 5.0f, headerPart - sectionGap*0.5f - 2.5f);
        *mList->layout = WidgetLayout::BothStretch(padding, padding, padding, headerPart);

        UpdateSelfTransform();
        UpdateChildrenTransforms();
    }

    void PipelineModelPicker::UpdateHighlight()
    {
        for (int i = 0; i < mRows.Count(); i++)
        {
            if (auto select = mRows[i]->FindLayer("select"))
                select->transparency = i == mHighlight ? 1.0f : 0.0f;
        }
    }

    void PipelineModelPicker::ScrollToRow(int index, bool withGroup)
    {
        if (index < 0 || index >= mRows.Count())
            return;

        UpdateSelfTransform();
        UpdateChildrenTransforms();

        float viewHeight = mList->layout->GetHeight();
        float scroll = mList->GetScroll().y;
        float top = mRowTops[index];
        float bottom = top + mRowHeights[index];
        if (withGroup && bottom - mGroupTops[index] <= viewHeight)
            top = mGroupTops[index];

        float target = scroll;
        if (top < scroll)
            target = top;
        else if (bottom > scroll + viewHeight)
            target = bottom - viewHeight;

        if (!Math::Equals(target, scroll))
            mList->SetScrollForcible(Vec2F(0.0f, target));
    }

    float PipelineModelPicker::MeasureText(const String& text, int height) const
    {
        // Glyphs the font has not rasterized yet measure too narrow
        mMeasure->GetFont()->CheckCharacters(text, height);
        return Text::GetTextSize(text, mMeasure->GetFont(), height, Vec2F(), HorAlign::Left, VerAlign::Top, false).x;
    }

    Ref<Button> PipelineModelPicker::MakeModelRow(const PipelineMenuModel& model, float width)
    {
        auto row = mmake<Button>();
        row->name = model.id;
        row->AddLayer("select", mmake<Sprite>("ui/UI4_Context_menu_select.png"), Layout::BothStretch(-10, -16, -10, -16))->transparency = 0.0f;
        row->AddLayer("check", mmake<Sprite>("ui/UI4_Ckeck.png"), Layout::Based(BaseCorner::Left, Vec2F(20, 20)))->transparency =
            model.id == mRequest.current ? 1.0f : 0.0f;

        // Fixed columns: the name, the transparency badge, the id; a text too long for its column ends with dots
        const float left = 22.0f, right = 6.0f, gap = 8.0f;
        float available = width - left - right;
        float idWidth = Math::Floor(available*0.4f);
        float alphaWidth = model.alpha ? 16.0f : 0.0f;
        float nameWidth = available - idWidth - alphaWidth - gap;

        row->AddLayer("name", MakeText(model.name, PipelineControls::textColor, HorAlign::Left),
                      Layout::Based(BaseCorner::Left, Vec2F(nameWidth, rowHeight), Vec2F(left, 0)));

        if (model.alpha)
        {
            auto badge = MakeText("\xCE\xB1", PipelineControls::accentColor, HorAlign::Middle, 12);
            badge->dotsEngings = false;
            row->AddLayer("alpha", badge, Layout::Based(BaseCorner::Left, Vec2F(alphaWidth, rowHeight), Vec2F(left + nameWidth, 0)));
        }

        row->AddLayer("id", MakeText(model.id, PipelineControls::dimTextColor, HorAlign::Right, 10),
                      Layout::Based(BaseCorner::Right, Vec2F(idWidth, rowHeight), Vec2F(-right, 0)));

        String id = model.id;
        row->onClick = [this, id]() { Pick(id); };
        return row;
    }

    Ref<Button> PipelineModelPicker::MakeHeader(const PipelineVisibleGroup& group, float width)
    {
        auto header = mmake<Button>();
        header->name = "group " + group.group.key;

        auto hover = mmake<PipelineRoundedRect>();
        hover->color = PipelineControls::textColor;
        hover->radius = 3.0f;
        hover->roundBottom = true;
        header->AddLayer("hover", hover, Layout::BothStretch(0, 0, 0, 0))->transparency = 0.0f;
        header->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 0.1f, 0.1f))->offStateAnimationSpeed = 0.25f;

        auto arrow = mmake<PipelineFoldArrow>();
        arrow->color = PipelineControls::dimTextColor;
        arrow->open = group.folded ? 0.0f : 1.0f;
        header->AddLayer("arrow", arrow, Layout::Based(BaseCorner::Left, Vec2F(10, 10), Vec2F(6, 0)));

        header->AddLayer("label", MakeText(group.group.label, PipelineControls::textColor, HorAlign::Left),
                         Layout::BothStretch(20, 0, 40, 0));
        header->AddLayer("count", MakeText((String)group.shown.Count(), PipelineControls::dimTextColor, HorAlign::Right, 10),
                         Layout::Based(BaseCorner::Right, Vec2F(34, headerHeight), Vec2F(-6, 0)));

        // Applied on the next update: the rebuild it causes replaces this header
        String key = group.group.key;
        header->onClick = [this, key]()
        {
            if (mFilter.query.Trimed(" \n\r\t").IsEmpty() && !mFilter.alphaOnly)
                mPendingToggle = key;

            o2UI.FocusWidget(mSearch);
        };
        return header;
    }

    Ref<Button> PipelineModelPicker::MakeCustomRow(const String& id)
    {
        auto row = mmake<Button>();
        row->name = "custom id";
        row->AddLayer("select", mmake<Sprite>("ui/UI4_Context_menu_select.png"), Layout::BothStretch(-10, -16, -10, -16))->transparency = 0.0f;
        row->AddLayer("plus", MakeText("+", PipelineControls::accentColor, HorAlign::Middle, 13), Layout::Based(BaseCorner::Left, Vec2F(20, rowHeight)));
        row->AddLayer("name", MakeText("Use \"" + id + "\" as model id", PipelineControls::textColor, HorAlign::Left),
                      Layout::BothStretch(22, 0, 6, 0));
        row->onClick = [this, id]() { Pick(id); };
        return row;
    }

    namespace PipelineControls
    {
        Ref<Button> MakeModelField()
        {
            // The layers of the standard drop down, so the closed field reads as one
            auto field = mmake<Button>();
            field->name = "model";
            field->layout->minSize = Vec2F(20, 20);
            field->AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9));
            field->AddLayer("hover", mmake<Sprite>("ui/UI4_Editbox_select.png"), Layout::BothStretch(-9, -9, -9, -9))->transparency = 0.0f;
            field->AddLayer("pressed", mmake<Sprite>("ui/UI4_Editbox_pressed.png"), Layout::BothStretch(-9, -9, -9, -9))->transparency = 0.0f;
            field->AddLayer("arrow", mmake<Sprite>("ui/UI4_Down_icn.png"), Layout(Vec2F(1.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(-20, -10), Vec2F(0, 10)));
            field->AddLayer("caption", MakeText("", textColor, HorAlign::Left), Layout::BothStretch(6, 0, 20, 0));
            auto badge = MakeText("\xCE\xB1", accentColor, HorAlign::Middle, 12);
            badge->dotsEngings = false;
            field->AddLayer("alpha", badge, Layout(Vec2F(1.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(-34, -10), Vec2F(-20, 10)))->transparency = 0.0f;

            field->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))->offStateAnimationSpeed = 0.5f;
            field->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))->offStateAnimationSpeed = 0.5f;
            return field;
        }

        void SetModelFieldValue(const Ref<Button>& field, const String& id, PipelineModelKind kind)
        {
            if (!field)
                return;

            bool alpha = PipelineModelMenu::IsAlpha(kind, id);
            if (auto caption = field->GetLayerDrawable<Text>("caption"))
                caption->text = PipelineUtils::PrettyModelName(id);

            if (auto captionLayer = field->FindLayer("caption"))
                captionLayer->layout = Layout::BothStretch(6, 0, alpha ? 34.0f : 20.0f, 0);

            if (auto alphaLayer = field->FindLayer("alpha"))
                alphaLayer->transparency = alpha ? 1.0f : 0.0f;

            field->SetLayoutDirty();
        }
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineModelPicker, Editor__PipelineModelPicker);
// --- END META ---
