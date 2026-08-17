#pragma once
#include "o2/Utils/Basic/IObject.h"

class Other;

class Pointers : public o2::IObject
{
public:
    static Pointers * createWithMap(int width);
    virtual Other * getLetter(int index);
    const Other * getConstLetter() const;
    static Pointers* create();
    Other *glued(float x);
    Other* attached(float x);

    Other* pointerField = nullptr;
    Other * spacedPointerField = nullptr;
    Other& referenceField;

    IOBJECT(Pointers);
};
