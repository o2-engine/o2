#pragma once

#include "o2/Events/CursorEventsArea.h"
#include "o2/Utils/Math/Curve.h"
#include "o2Editor/Core/Properties/IPropertyField.h"

using namespace o2;

namespace o2
{
    class Widget;
}

namespace Editor
{
    FORWARD_CLASS_REF(CurvePreview);

    // ------------------------------
    // Editor curve property edit box
    // ------------------------------
    class CurveProperty: public TPropertyField<Ref<Curve>>
    {
    public:
        // Default constructor
        CurveProperty(RefCounter* refCounter);

        // Copy constructor
        CurveProperty(RefCounter* refCounter, const CurveProperty& other);

        // Copy operator
        CurveProperty& operator=(const CurveProperty& other);

        SERIALIZABLE(CurveProperty);
        CLONEABLE_REF(CurveProperty);

    protected:
        Ref<Widget>           mEditBox;      // Edit box 
        Ref<CurvePreview>     mPreviewImage; // Curve preview image
        Ref<CursorEventsArea> mClickArea;    // Box click area

    protected:
        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Called when common curve changed in curve edit dialog, tells to drawing content
        void OnValueChanged() override;

        // Called when box pressed and shows color picking dialog
        void OnClicked();
    };
}
// --- META ---

CLASS_BASES_META(Editor::CurveProperty)
{
    BASE_CLASS(Editor::TPropertyField<Ref<Curve>>);
}
END_META;
CLASS_FIELDS_META(Editor::CurveProperty)
{
    FIELD().PROTECTED().NAME(mEditBox);
    FIELD().PROTECTED().NAME(mPreviewImage);
    FIELD().PROTECTED().NAME(mClickArea);
}
END_META;
CLASS_METHODS_META(Editor::CurveProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const CurveProperty&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, OnValueChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClicked);
}
END_META;
// --- END META ---
