#include "o2Editor/stdafx.h"
#include "ScenePickMenu.h"

#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Editor/SceneEditableObject.h"

namespace Editor
{
    ScenePickMenu::ScenePickMenu(RefCounter* refCounter):
        ContextMenu(refCounter)
    {}

    ScenePickMenu::ScenePickMenu(RefCounter* refCounter, const ContextMenu& style):
        ContextMenu(refCounter, style)
    {
        RemoveAllItems();
        SetSearchEnabled(false);
    }

    ScenePickMenu::ScenePickMenu(RefCounter* refCounter, const ScenePickMenu& other):
        ContextMenu(refCounter, other)
    {}

    void ScenePickMenu::Show(const Vector<Ref<SceneEditableObject>>& objects, const Vec2F& position)
    {
        mHoveredEntry = -1;
        mParentsPanelRect = RectF();
        mEntries = BuildEntries(objects);

        RebuildEntryItems();
        ContextMenu::Show(position);
    }

    const Vector<ScenePickEntry>& ScenePickMenu::GetEntries() const
    {
        return mEntries;
    }

    int ScenePickMenu::GetHoveredEntryIndex() const
    {
        return mHoveredEntry;
    }

    void ScenePickMenu::SetHoveredEntry(int index)
    {
        if (index >= mEntries.Count())
            index = -1;

        if (index == mHoveredEntry)
            return;

        mHoveredEntry = index;
        UpdateParentsPanel();
        onHoverChanged(index >= 0 ? mEntries[index].object : Ref<SceneEditableObject>());
    }

    void ScenePickMenu::PickEntry(int index)
    {
        if (index < 0 || index >= mEntries.Count())
            return;

        auto object = mEntries[index].object;
        HideWithParent();
        onPicked(object);
    }

    const RectF& ScenePickMenu::GetParentsPanelRect() const
    {
        return mParentsPanelRect;
    }

    void ScenePickMenu::Update(float dt)
    {
        ContextMenu::Update(dt);

        if (!mResEnabledInHierarchy)
            return;

        int hovered = -1;
        if (mSelectedItem)
        {
            for (int i = 0; i < mItems.Count() && i < mEntries.Count(); i++)
            {
                if (mItems[i]->widget.Lock() == mSelectedItem)
                {
                    hovered = i;
                    break;
                }
            }
        }

        // The item stays hovered while the cursor is out of the menu, so the panel can be read
        if (hovered >= 0)
            SetHoveredEntry(hovered);
    }

    Vector<ScenePickEntry> ScenePickMenu::BuildEntries(const Vector<Ref<SceneEditableObject>>& objects)
    {
        Vector<Ref<SceneEditableObject>> picked;
        for (auto& object : objects)
        {
            if (object && !picked.Contains(object))
                picked.Add(object);
        }

        // Actors, widgets and layers are the nodes of the scene tree; the "layers" and "internal children"
        // groups of a widget are only containers
        auto isNode = [](const Ref<SceneEditableObject>& object) {
            return DynamicCast<Actor>(object) || DynamicCast<WidgetLayer>(object);
        };

        auto rootIndex = [](const Ref<SceneEditableObject>& object) {
            if (Scene::IsSingletonInitialzed())
            {
                if (auto actor = DynamicCast<Actor>(object))
                    return o2Scene.GetRootActors().IndexOf(actor);
            }

            return -1;
        };

        struct Ordered
        {
            Vector<Ref<SceneEditableObject>> chain; // From the root down to the object itself
            Vector<int>                      key;   // Indices in the siblings along the chain
            int                              index; // Index in the objects

            bool operator==(const Ordered& other) const { return index == other.index; }
        };

        Vector<Ordered> ordered;
        for (int i = 0; i < picked.Count(); i++)
        {
            Ordered item;
            item.index = i;

            for (auto it = picked[i]; it; it = it->GetEditableParent())
                item.chain.Insert(it, 0);

            for (int k = 0; k < item.chain.Count(); k++)
            {
                if (k == 0)
                    item.key.Add(rootIndex(item.chain[k]));
                else
                    item.key.Add(item.chain[k - 1]->GetEditableChildren().IndexOf(item.chain[k]));
            }

            ordered.Add(item);
        }

        ordered.Sort([](const Ordered& a, const Ordered& b) {
            int count = Math::Min(a.key.Count(), b.key.Count());
            for (int i = 0; i < count; i++)
            {
                if (a.key[i] != b.key[i])
                    return a.key[i] < b.key[i];
            }

            if (a.key.Count() != b.key.Count())
                return a.key.Count() < b.key.Count();

            return a.index < b.index;
        });

        Vector<ScenePickEntry> result;
        for (auto& item : ordered)
        {
            ScenePickEntry entry;
            entry.object = item.chain.Last();

            int nodeDepth = 0;
            for (int k = 0; k < item.chain.Count() - 1; k++)
            {
                auto& ancestor = item.chain[k];
                if (picked.Contains(ancestor))
                    entry.depth++;
                else if (isNode(ancestor))
                {
                    entry.hiddenParents.Add(ancestor);
                    entry.hiddenDepths.Add(nodeDepth);
                }

                if (isNode(ancestor))
                    nodeDepth++;
            }

            result.Add(entry);
        }

        return result;
    }

    String ScenePickMenu::GetObjectIcon(const Ref<SceneEditableObject>& object)
    {
        if (DynamicCast<WidgetLayer>(object))
            return "ui/UI4_node_layer_icon.svg";

        // The icon of the actor's type, as the create menu shows it
        if (auto actor = DynamicCast<Actor>(object))
        {
            String icon = actor->GetType().InvokeStatic<String>("GetEditorIcon");
            if (!icon.IsEmpty())
                return icon;
        }

        return "ui/UI4_node_actor_icon.svg";
    }

    String ScenePickMenu::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void ScenePickMenu::FitPosition(const Vec2F& position, Vec2F size)
    {
        int maxDepth = 0;
        for (auto& entry : mEntries)
            maxDepth = Math::Max(maxDepth, entry.depth);

        size.x += indentStep*(float)maxDepth;
        ContextMenu::FitPosition(position, size);
    }

    void ScenePickMenu::SpecialDraw()
    {
        ContextMenu::SpecialDraw();

        if (!mResEnabledInHierarchy || mParentsPanelRect.Width() < 1.0f || mParentsTexts.IsEmpty())
            return;

        if (mParentsBack)
            mParentsBack->Draw();

        for (auto& icon : mParentsIcons)
            icon->Draw();

        for (auto& text : mParentsTexts)
            text->Draw();
    }

    void ScenePickMenu::OnDisabled()
    {
        ContextMenu::OnDisabled();
        SetHoveredEntry(-1);
    }

    void ScenePickMenu::RebuildEntryItems()
    {
        PushEditorScopeOnStack scope;

        mItems.Clear();
        for (int i = 0; i < mEntries.Count(); i++)
        {
            auto& entry = mEntries[i];
            WString name = entry.object->GetName();
            if (name.IsEmpty())
                name = "(no name)";

            WeakRef<SceneEditableObject> object = entry.object;
            auto item = mmake<Item>(name, Function<void()>([this, object]() {
                                        if (auto locked = object.Lock())
                                            onPicked(locked);
                                    }), WString(),
                                    AssetRef<Asset>(AssetRef<VectorImageAsset>(GetObjectIcon(entry.object))));
            mItems.Add(item);
        }

        RebuildItems();

        // The whole content of an item lies on its "basic" layer, which is shifted by the depth
        for (int i = 0; i < mItems.Count(); i++)
        {
            if (auto widget = mItems[i]->widget.Lock())
            {
                if (auto basic = widget->FindLayer("basic"))
                    basic->layout = Layout::BothStretch(indentStep*(float)mEntries[i].depth, 0, 0, 0);
            }
        }
    }

    void ScenePickMenu::UpdateParentsPanel()
    {
        mParentsTexts.Clear();
        mParentsIcons.Clear();
        mParentsPanelRect = RectF();

        if (mHoveredEntry < 0 || mHoveredEntry >= mEntries.Count())
            return;

        auto& entry = mEntries[mHoveredEntry];
        if (entry.hiddenParents.IsEmpty())
            return;

        PushEditorScopeOnStack scope;

        const float rowHeight = 20.0f;
        const float padding = 6.0f;
        const float iconSize = 16.0f;
        const float iconGap = 4.0f;
        const float gap = 4.0f;

        Ref<Text> sampleText;
        if (mItemSample)
            sampleText = mItemSample->GetLayerDrawable<Text>("basic/caption");

        int minDepth = entry.hiddenDepths.IsEmpty() ? 0 : entry.hiddenDepths[0];
        for (int depth : entry.hiddenDepths)
            minDepth = Math::Min(minDepth, depth);

        float width = 0.0f;
        Vector<float> indents;
        for (int i = 0; i < entry.hiddenParents.Count(); i++)
        {
            auto text = sampleText ? sampleText->CloneAsRef<Text>() : mmake<Text>("stdFont.ttf");
            text->SetWordWrap(false);
            text->SetHorAlign(HorAlign::Left);
            text->SetVerAlign(VerAlign::Middle);
            text->SetText(entry.hiddenParents[i]->GetName());
            mParentsTexts.Add(text);

            auto icon = mmake<VectorSprite>(GetObjectIcon(entry.hiddenParents[i]));
            icon->SetMode(SpriteMode::FixedAspect);
            icon->SetColor(mIconsColor);
            mParentsIcons.Add(icon);

            float indent = indentStep*(float)(entry.hiddenDepths[i] - minDepth);
            indents.Add(indent);

            Vec2F textSize = Text::GetTextSize(text->GetText(), text->GetFont(), text->GetFontHeight(), Vec2F(),
                                               HorAlign::Left, VerAlign::Top, false);
            width = Math::Max(width, indent + iconSize + iconGap + textSize.x);
        }

        width += padding*2.0f;
        float height = rowHeight*(float)entry.hiddenParents.Count() + padding*2.0f;

        // Next to the hovered item, on the left of the menu, or on its right where the left has no room
        RectF menuRect = layout->GetWorldRect();
        float top = menuRect.top;
        if (mHoveredEntry < mItems.Count())
        {
            if (auto widget = mItems[mHoveredEntry]->widget.Lock())
                top = widget->layout->GetWorldRect().top + padding;
        }

        Vec2F resolution = (Vec2F)o2Render.GetResolution();
        float left = menuRect.left - gap - width;
        if (left < -resolution.x*0.5f)
            left = menuRect.right + gap;

        top = Math::Min(top, resolution.y*0.5f);
        if (top - height < -resolution.y*0.5f)
            top = -resolution.y*0.5f + height;

        mParentsPanelRect = RectF(left, top - height, left + width, top);

        if (!mParentsBack)
        {
            if (auto back = FindLayer("back"))
            {
                if (back->GetDrawable())
                    mParentsBack = back->GetDrawable()->CloneAsRef<IRectDrawable>();
            }
        }

        // The menu's background is drawn with its shadow around
        if (mParentsBack)
        {
            mParentsBack->SetRect(RectF(mParentsPanelRect.left - 20.0f, mParentsPanelRect.bottom - 19.0f,
                                        mParentsPanelRect.right + 20.0f, mParentsPanelRect.top + 19.0f));
        }

        for (int i = 0; i < mParentsTexts.Count(); i++)
        {
            float rowTop = mParentsPanelRect.top - padding - rowHeight*(float)i;
            float rowLeft = mParentsPanelRect.left + padding + indents[i];
            float iconBottom = rowTop - (rowHeight + iconSize)*0.5f;

            mParentsIcons[i]->SetRect(RectF(rowLeft, iconBottom, rowLeft + iconSize, iconBottom + iconSize));
            mParentsTexts[i]->SetRect(RectF(rowLeft + iconSize + iconGap, rowTop - rowHeight,
                                            mParentsPanelRect.right - padding, rowTop));
        }
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::ScenePickMenu>);
// --- META ---

DECLARE_CLASS(Editor::ScenePickMenu, Editor__ScenePickMenu);
// --- END META ---
