#include "Conditional.h"
// --- META ---

#if  IS_EDITOR
DECLARE_CLASS(game::EditorOnly, game__EditorOnly);
#endif

DECLARE_CLASS(game::Partial, game__Partial);
// --- END META ---
