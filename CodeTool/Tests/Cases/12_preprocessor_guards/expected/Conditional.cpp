#include "Conditional.h"
// --- META ---

#if defined(USE_FEATURE)
DECLARE_CLASS(Featured, Featured);
#endif

#if !defined(DISABLE_LEGACY)
DECLARE_CLASS(Legacy, Legacy);
#endif
// --- END META ---
