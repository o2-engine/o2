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
// --- META ---

CLASS_BASES_META(game::Player)
{
    BASE_CLASS(game::Health);
}
END_META;
CLASS_FIELDS_META(game::Player)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1).NAME(level);
}
END_META;
CLASS_METHODS_META(game::Player)
{
}
END_META;
// --- END META ---
