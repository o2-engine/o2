#pragma once

namespace game
{
    // Derives from a class known only through the parent project cache
    class Player: public Health
    {
    public:
        int level = 1; // @SERIALIZABLE

        IOBJECT(Player);
    };
}
