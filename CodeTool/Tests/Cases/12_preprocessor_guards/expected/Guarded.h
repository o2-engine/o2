#ifndef __GUARDED_H__
#define __GUARDED_H__

#include "o2/Utils/Basic/IObject.h"

class Guarded : public o2::IObject
{
public:
    int value = 0;

    IOBJECT(Guarded);
};

#endif // __GUARDED_H__
// --- META ---

CLASS_BASES_META(Guarded)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(Guarded)
{
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(value);
}
END_META;
CLASS_METHODS_META(Guarded)
{
}
END_META;
// --- END META ---
