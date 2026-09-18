#include "o2Editor/stdafx.h"
#include "PipelineNodePalette.h"

#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"

namespace Editor
{
    using namespace PipelineControls;

    static const Vector<Pair<PipelineNodeCategory, String>> paletteCategories = {
        { PipelineNodeCategory::Source, "Source" },
        { PipelineNodeCategory::Transform, "Transform" },
        { PipelineNodeCategory::AI, "AI" },
        { PipelineNodeCategory::Output, "Output" },
        { PipelineNodeCategory::Flow, "Flow" },
    };

    static const float entryHeight = 34.0f;
    static const float headerHeight = 18.0f;

    PipelineNodePalette::PipelineNodePalette(RefCounter* refCounter):
        Widget(refCounter)
    {
        InitializeControls();
        RebuildList();
    }

    void PipelineNodePalette::Reset()
    {
        mCategory = -1;
        for (auto& toggle : mTabToggles)
            toggle->SetValue(false);

        mFilter->SetText("");
        RebuildList();
        mFilter->Focus();
    }

    Vector<String> PipelineNodePalette::GetShownTypes() const
    {
        return mShownTypes;
    }

    void PipelineNodePalette::InitializeControls()
    {
        PushEditorScopeOnStack scope;

        AddLayer("back", mmake<Sprite>("ui/UI4_Context_menu.png"), Layout::BothStretch(-20, -19, -20, -19));

        mFilter = MakeEditBox("", false, "search");
        mFilter->name = "filter";
        *mFilter->layout = WidgetLayout::HorStretch(VerAlign::Top, 8, 8, 20, 8);
        mFilter->onChanged = [this](const WString&) { RebuildList(); };
        AddChild(mFilter);

        mTabs = mmake<HorizontalLayout>();
        mTabs->name = "tabs";
        mTabs->spacing = 2;
        mTabs->expandWidth = false;
        mTabs->expandHeight = true;
        mTabs->baseCorner = BaseCorner::Left;
        *mTabs->layout = WidgetLayout::HorStretch(VerAlign::Top, 4, 4, 20, 32);
        AddChild(mTabs);

        for (auto& category : paletteCategories)
        {
            bool hasNodes = PipelineNodeRegistry::AllSchemas().Any([&](const PipelineNodeSchema* schema)
            {
                return schema->category == category.first;
            });
            if (!hasNodes)
                continue;

            int index = mCategories.Count();
            auto toggle = MakeSegment(category.second, false);
            toggle->name = category.second;
            // Equal-width tabs would clip the longer category names, so a tab is as wide as its caption
            if (auto caption = toggle->GetLayerDrawable<Text>("caption"))
            {
                caption->SetHeight(10);
                float width = Math::Max(30.0f, Text::GetTextSize(category.second, caption->GetFont(), 10).x + 16.0f);
                toggle->layout->minWidth = width;
                toggle->layout->maxWidth = width;
            }
            toggle->onToggleByUser = [this, index](bool value)
            {
                mCategory = value ? index : -1;
                for (int i = 0; i < mTabToggles.Count(); i++)
                    mTabToggles[i]->SetValue(i == mCategory);

                RebuildList();
            };
            mTabs->AddChild(toggle);
            mTabToggles.Add(toggle);
            mCategories.Add(category.first);
        }

        mScroll = o2UI.CreateScrollArea();
        mScroll->SetEnableScrollsHiding(true);
        *mScroll->layout = WidgetLayout::BothStretch(8, 8, 8, 56);
        AddChild(mScroll);

        mList = mmake<VerticalLayout>();
        mList->name = "list";
        mList->spacing = 2;
        mList->expandWidth = true;
        mList->expandHeight = false;
        mList->fitByChildren = true;
        mList->baseCorner = BaseCorner::Top;
        *mList->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
        mScroll->AddChild(mList);

        mEmptyHint = MakeLabel("nothing matches the search", true);
        mEmptyHint->horAlign = HorAlign::Middle;
        mEmptyHint->verAlign = VerAlign::Top;
        *mEmptyHint->layout = WidgetLayout::HorStretch(VerAlign::Top, 8, 8, 40, 60);
        mEmptyHint->enabled = false;
        AddChild(mEmptyHint);
    }

    bool PipelineNodePalette::Matches(const PipelineNodeSchema* schema, const String& query)
    {
        if (query.IsEmpty())
            return true;

        return schema->label.ToLowerCase().Contains(query) || schema->type.ToLowerCase().Contains(query) ||
            schema->description.ToLowerCase().Contains(query);
    }

    void PipelineNodePalette::RebuildList()
    {
        PushEditorScopeOnStack scope;

        mList->RemoveAllChildren();
        mShownTypes.Clear();

        String query = ((String)mFilter->GetText()).ToLowerCase().Trimed();
        // A search looks across every category: the open tab only narrows the untyped browse
        bool searching = !query.IsEmpty();

        for (int i = 0; i < mCategories.Count(); i++)
        {
            if (!searching && mCategory >= 0 && mCategory != i)
                continue;

            Vector<const PipelineNodeSchema*> schemas;
            for (auto schema : PipelineNodeRegistry::AllSchemas())
            {
                if (schema->category == mCategories[i] && Matches(schema, query))
                    schemas.Add(schema);
            }

            if (schemas.IsEmpty())
                continue;

            if (searching || mCategory < 0)
            {
                auto header = MakeLabel(paletteCategories.FindOrDefault([&](auto& p) { return p.first == mCategories[i]; }).second, true);
                header->layout->minHeight = headerHeight;
                header->layout->maxHeight = headerHeight;
                mList->AddChild(header);
            }

            for (auto schema : schemas)
            {
                String type = schema->type;
                auto entry = o2UI.CreateButton(schema->label);
                entry->name = type;
                entry->layout->minHeight = entryHeight;
                entry->layout->maxHeight = entryHeight;
                entry->onClick = [this, type]() { onPick(type); };

                if (auto caption = entry->GetLayer("caption"))
                {
                    caption->layout = Layout::HorStretch(VerAlign::Top, 40, 6, 14, 2);
                    if (auto text = DynamicCast<Text>(caption->GetDrawable()))
                    {
                        text->horAlign = HorAlign::Left;
                        text->wordWrap = false;
                    }
                }

                auto description = mmake<Text>("stdFont.ttf");
                description->text = schema->description;
                description->horAlign = HorAlign::Left;
                description->verAlign = VerAlign::Top;
                description->dotsEngings = true;
                description->wordWrap = false;
                description->SetHeight(9);
                description->color = dimTextColor;
                entry->AddLayer("description", description, Layout::HorStretch(VerAlign::Top, 40, 6, 13, 18));

                auto icon = mmake<Sprite>(PipelineNodeWidget::MenuIconForType(type));
                icon->color = accentColor;
                entry->AddLayer("icon", icon, Layout::Based(BaseCorner::Left, Vec2F(18, 18), Vec2F(17, 0)));

                mList->AddChild(entry);
                mShownTypes.Add(type);
            }
        }

        mEmptyHint->enabled = mShownTypes.IsEmpty();
        mScroll->SetScroll(Vec2F());
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineNodePalette, Editor__PipelineNodePalette);
// --- END META ---
