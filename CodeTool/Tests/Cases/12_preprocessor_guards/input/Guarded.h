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
