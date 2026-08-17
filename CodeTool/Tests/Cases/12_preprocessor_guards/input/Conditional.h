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
