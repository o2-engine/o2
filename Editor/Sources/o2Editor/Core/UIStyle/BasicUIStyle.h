#pragma once

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"

namespace o2
{
    class BasicUIStyleBuilder: public IObject
    {
    public:
        void RebuildButtonStyle();
        void RebuildCloseButtonStyle();
        void RebuildArrowButtonStyle();
        void RebuildHorSeparatorStyle();
        void RebuildHorProgressBarStyle();
        void RebuildVerProgressBarStyle();
        void RebuildHorScrollBarStyle();
        void RebuildVerScrollBarStyle();
        void RebuildCheckboxStyle();
        void RebuildCheckboxWithoutCaptionStyle();
        void RebuildScrollAreaStyle();
        void RebuildScrollAreaStraightBarsStyle();
        void RebuildPopupStyle();
        void RebuildLabelStyle();
        void RebuildEditBoxStyle();
        void RebuildCustomListStyle();
        void RebuildLongListStyle();
        void RebuildListStyle();
        void RebuildCustomDropDownStyle();
        void RebuildDropDownStyle();
        void RebuildEditBoxDropDownStyle();
        void RebuildWindowStyle();
        void RebuildTreeStyle();
        void RebuildMenuPanelStyle();
        void RebuildBacklessDropdown();
        void RebuildBacklessEditbox();
        void RebuildBacklessMultilineEditbox();
        void RebuildSinglelineEditbox();
        void RebuildBacklessScrollarea();
        void RebuildContextMenuStyle();
        void RebuildExpandButton();
        void RebuildExpandDownButton();
        void RebuildExpandWhiteButton();
        void RebuildSpoiler();

        IOBJECT(BasicUIStyleBuilder);
    };
}
// --- META ---

CLASS_BASES_META(o2::BasicUIStyleBuilder)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(o2::BasicUIStyleBuilder)
{
}
END_META;
CLASS_METHODS_META(o2::BasicUIStyleBuilder)
{

    FUNCTION().PUBLIC().SIGNATURE(void, RebuildButtonStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildCloseButtonStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildArrowButtonStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildHorSeparatorStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildHorProgressBarStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildVerProgressBarStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildHorScrollBarStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildVerScrollBarStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildCheckboxStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildCheckboxWithoutCaptionStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildScrollAreaStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildScrollAreaStraightBarsStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildPopupStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildLabelStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildEditBoxStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildCustomListStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildLongListStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildListStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildCustomDropDownStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildDropDownStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildEditBoxDropDownStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildWindowStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildTreeStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildMenuPanelStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildBacklessDropdown);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildBacklessEditbox);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildBacklessMultilineEditbox);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildSinglelineEditbox);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildBacklessScrollarea);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildContextMenuStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildExpandButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildExpandDownButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildExpandWhiteButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildSpoiler);
}
END_META;
// --- END META ---
