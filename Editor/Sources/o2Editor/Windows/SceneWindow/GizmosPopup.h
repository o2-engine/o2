#pragma once
#include "o2/Scene/UI/Widgets/PopupWidget.h"

using namespace o2;

namespace o2
{
    class Toggle;
}

namespace Editor
{
    // ---------------------------------------------------------------------------------------
    // Scene view display popup. Contains common gizmos switch, scene selection switch and a
    // switch for each actor and component type which draws gizmos on the scene
    // ---------------------------------------------------------------------------------------
    class GizmosPopup: public PopupWidget
    {
    public:
        // Default constructor
        GizmosPopup(RefCounter* refCounter);

        // Destructor
        ~GizmosPopup();

        // Shows gizmos popup at position
        void Show(const Vec2F& position);

        // Updates widget and rebuilds types list when new gizmos drawing type was met
        void Update(float dt) override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(GizmosPopup);
        CLONEABLE_REF(GizmosPopup);

    private:
        Ref<Toggle> mEnableToggle;    // Common gizmos drawing switch
        Ref<Toggle> mSelectionToggle; // Scene objects selection drawing switch

        Vector<Ref<Toggle>> mTypeToggles; // Per type gizmos drawing switches

        bool mTypesListDirty = false; // True when types list changed while popup is opened

    private:
        // Returns content size for fitting by children
        Vec2F GetContentSize() const override;

        // Initializes background and common switches
        void InitializeControls();

        // Rebuilds switches of types which draw gizmos
        void UpdateTypesList();
    };
}
// --- META ---

CLASS_BASES_META(Editor::GizmosPopup)
{
    BASE_CLASS(o2::PopupWidget);
}
END_META;
CLASS_FIELDS_META(Editor::GizmosPopup)
{
    FIELD().PRIVATE().NAME(mEnableToggle);
    FIELD().PRIVATE().NAME(mSelectionToggle);
    FIELD().PRIVATE().NAME(mTypeToggles);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mTypesListDirty);
}
END_META;
CLASS_METHODS_META(Editor::GizmosPopup)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Show, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PRIVATE().SIGNATURE(Vec2F, GetContentSize);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeControls);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateTypesList);
}
END_META;
// --- END META ---
