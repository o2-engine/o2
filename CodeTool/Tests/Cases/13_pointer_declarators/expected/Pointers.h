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
// --- META ---

CLASS_BASES_META(Pointers)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(Pointers)
{
    FIELD().PUBLIC().DEFAULT_VALUE(nullptr).NAME(pointerField);
    FIELD().PUBLIC().DEFAULT_VALUE(nullptr).NAME(spacedPointerField);
    FIELD().PUBLIC().NAME(referenceField);
}
END_META;
CLASS_METHODS_META(Pointers)
{

    FUNCTION().PUBLIC().SIGNATURE_STATIC(Pointers*, createWithMap, int);
    FUNCTION().PUBLIC().SIGNATURE(Other*, getLetter, int);
    FUNCTION().PUBLIC().SIGNATURE(const Other*, getConstLetter);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Pointers*, create);
    FUNCTION().PUBLIC().SIGNATURE(Other*, glued, float);
    FUNCTION().PUBLIC().SIGNATURE(Other*, attached, float);
}
END_META;
// --- END META ---
