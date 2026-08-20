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
