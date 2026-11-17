#include "o2Editor/stdafx.h"
#include "GizmosPopup.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2Editor/UI/Style/EditorTheme.h"

namespace Editor
{
    static constexpr float gizmosPopupItemHeight = 20.0f;
    static constexpr float gizmosPopupWidth = 240.0f;
    static constexpr float gizmosPopupIconWidth = 20.0f;

    GizmosPopup::GizmosPopup(RefCounter* refCounter):
        PopupWidget(refCounter)
    {
        InitializeControls();

        WeakRef<GizmosPopup> weakThis(this);
        o2EditorSceneScreen.GetGizmos().onGizmosTypesChanged += [weakThis]() {
            if (auto self = weakThis.Lock())
                self->mTypesListDirty = true;
        };
    }

    GizmosPopup::~GizmosPopup()
    {}

    void GizmosPopup::Show(const Vec2F& position)
    {
        UpdateTypesList();
        PopupWidget::Show(position);
    }

    void GizmosPopup::Update(float dt)
    {
        PopupWidget::Update(dt);

        if (!mTypesListDirty)
            return;

        // types are collected while the scene draws, so the list is rebuilt outside of drawing
        UpdateTypesList();
        UpdateTransform();
        FitSizeAndPosition(layout->GetWorldLeftTop());
    }

    String GizmosPopup::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    Vec2F GizmosPopup::GetContentSize() const
    {
        return Vec2F(gizmosPopupWidth, (float)mChildWidgets.Count()*gizmosPopupItemHeight);
    }

    void GizmosPopup::InitializeControls()
    {
        PushEditorScopeOnStack scope;

        AddLayer("back", mmake<VectorSprite>("ui/UI4_Context_menu.svg"), Layout::BothStretch(-20, -19, -20, -19));

        mEnableToggle = o2UI.CreateWidget<Toggle>();
        mEnableToggle->name = "gizmos enable";
        mEnableToggle->caption = "Gizmos";
        mEnableToggle->onToggleByUser = [](bool enabled) { o2EditorSceneScreen.GetGizmos().SetEnabled(enabled); };

        mSelectionToggle = o2UI.CreateWidget<Toggle>();
        mSelectionToggle->name = "selection enable";
        mSelectionToggle->caption = "Selection";
        mSelectionToggle->AddLayer("line", mmake<VectorSprite>("ui/UI4_Separator.svg"),
                                   Layout::HorStretch(VerAlign::Top, 0, 0, 5, -2));

        mSelectionToggle->onToggleByUser = [](bool visible) { o2EditorSceneScreen.SetSelectionVisible(visible); };
    }

    String GizmosPopup::GetTypeIcon(const Type* type)
    {
        if (type->IsBasedOn(TypeOf(Component)))
            return type->InvokeStatic<String>("GetIcon");

        return type->InvokeStatic<String>("GetEditorIcon");
    }

    void GizmosPopup::AddTypeIcon(const Ref<Toggle>& toggle, const Type* type)
    {
        String iconPath = GetTypeIcon(type);
        if (iconPath.IsEmpty())
            return;

        // The component icons are white glyphs: they are tinted like the code-drawn icons
        auto icon = mmake<VectorSprite>(iconPath);
        if (type->IsBasedOn(TypeOf(Component)))
            icon->SetColor(EditorTheme::Get().ink);

        toggle->AddLayer("type icon", icon, Layout::Based(BaseCorner::Left, Vec2F(16, 16), Vec2F(21, 0)));

        if (auto caption = toggle->FindLayer("caption"))
            caption->layout.offsetMin = caption->layout.offsetMin.Get() + Vec2F(gizmosPopupIconWidth, 0.0f);
    }

    void GizmosPopup::UpdateTypesList()
    {
        PushEditorScopeOnStack scope;

        mTypesListDirty = false;

        auto& gizmos = o2EditorSceneScreen.GetGizmos();
        gizmos.UpdateGizmosTypes();

        RemoveAllChildren();
        mTypeToggles.Clear();

        mEnableToggle->value = gizmos.IsEnabled();
        AddChild(mEnableToggle);
        mEnableToggle->layout->CopyFrom(WidgetLayout::HorStretch(VerAlign::Top, 5, 5, gizmosPopupItemHeight, 0));

        int idx = 1;
        for (auto type : gizmos.GetGizmosTypes())
        {
            auto toggle = o2UI.CreateWidget<Toggle>();
            toggle->name = type->GetName();
            toggle->caption = (WString)type->GetName().ReplacedAll("o2::", "");
            toggle->value = gizmos.IsTypeEnabled(type);
            toggle->onToggleByUser = [=](bool enabled) {
                o2EditorSceneScreen.GetGizmos().SetTypeEnabled(type, enabled);
            };

            AddTypeIcon(toggle, type);

            AddChild(toggle);
            toggle->layout->CopyFrom(WidgetLayout::HorStretch(VerAlign::Top, 20, 5, gizmosPopupItemHeight,
                                                              gizmosPopupItemHeight*(float)idx));

            mTypeToggles.Add(toggle);
            idx++;
        }

        // selection switch is not a gizmos type, so it goes last under a separator line
        mSelectionToggle->value = o2EditorSceneScreen.IsSelectionVisible();
        AddChild(mSelectionToggle);
        mSelectionToggle->layout->CopyFrom(WidgetLayout::HorStretch(VerAlign::Top, 5, 5, gizmosPopupItemHeight,
                                                                    gizmosPopupItemHeight*(float)idx));
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::GizmosPopup>);
// --- META ---

DECLARE_CLASS(Editor::GizmosPopup, Editor__GizmosPopup);
// --- END META ---
