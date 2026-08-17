#pragma once
#include "o2/Utils/Basic/IObject.h"

#ifdef USE_FEATURE

class Featured : public o2::IObject
{
public:
    int featureValue = 0;

    IOBJECT(Featured);
};

#endif

#ifndef DISABLE_LEGACY

class Legacy : public o2::IObject
{
public:
    int legacyValue = 0;

    IOBJECT(Legacy);
};

#endif
// --- META ---

#if defined(USE_FEATURE)
CLASS_BASES_META(Featured)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(Featured)
{
#if defined(USE_FEATURE)
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(featureValue);
#endif
}
END_META;
CLASS_METHODS_META(Featured)
{
}
END_META;
#endif

#if !defined(DISABLE_LEGACY)
CLASS_BASES_META(Legacy)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(Legacy)
{
#if !defined(DISABLE_LEGACY)
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(legacyValue);
#endif
}
END_META;
CLASS_METHODS_META(Legacy)
{
}
END_META;
#endif
// --- END META ---
