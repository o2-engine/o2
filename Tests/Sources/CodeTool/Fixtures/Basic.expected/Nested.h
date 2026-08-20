#pragma once

#include "TestBase.h"

namespace game
{
    namespace ui
    {
        class Control: public o2::IObject
        {
        public:
            IOBJECT(Control);

            // Nested class
            class Style: public o2::IObject
            {
            public:
                int color = 0; // @SERIALIZABLE

                IOBJECT(Style);
            };
        };
    }

    typedef ui::Control BaseControl;

    // Base referenced through typedef
    class Button: public BaseControl
    {
    public:
        IOBJECT(Button);
    };
}

namespace app
{
    using namespace game;

    // Base found via using namespace
    class Panel: public ui::Control
    {
    public:
        IOBJECT(Panel);
    };
}
// --- META ---

CLASS_BASES_META(game::Button)
{
    BASE_CLASS(BaseControl);
}
END_META;
CLASS_FIELDS_META(game::Button)
{
}
END_META;
CLASS_METHODS_META(game::Button)
{
}
END_META;

CLASS_BASES_META(game::ui::Control)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(game::ui::Control)
{
}
END_META;
CLASS_METHODS_META(game::ui::Control)
{
}
END_META;

CLASS_BASES_META(game::ui::Control::Style)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(game::ui::Control::Style)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(color);
}
END_META;
CLASS_METHODS_META(game::ui::Control::Style)
{
}
END_META;

CLASS_BASES_META(app::Panel)
{
    BASE_CLASS(ui::Control);
}
END_META;
CLASS_FIELDS_META(app::Panel)
{
}
END_META;
CLASS_METHODS_META(app::Panel)
{
}
END_META;
// --- END META ---
