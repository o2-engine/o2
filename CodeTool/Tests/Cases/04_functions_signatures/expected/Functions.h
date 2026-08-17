#pragma once
#include "o2/Utils/Basic/IObject.h"

namespace test
{
    class Value;

    class Functions: public o2::IObject
    {
    public:
        IOBJECT(Functions);

        void NoArgs();
        int WithArgs(int a, float b);
        const o2::String& ConstRefReturn() const;
        o2::Vector<int> VectorReturn();

        void RefArg(const o2::String& text);
        void PointerArg(Value* value);
        void GluedPointerArg(Value *value);
        Value* PointerReturn();
        Value *GluedPointerReturn();

        void MultiWordTypes(unsigned short mask, long long counter);
        unsigned short GetMask() const;

        static int StaticFunction(int x);
        virtual void VirtualFunction();

        void DefaultArgs(int a = 5, bool b = true);
    };
}
// --- META ---

CLASS_BASES_META(test::Functions)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(test::Functions)
{
}
END_META;
CLASS_METHODS_META(test::Functions)
{

    FUNCTION().PUBLIC().SIGNATURE(void, NoArgs);
    FUNCTION().PUBLIC().SIGNATURE(int, WithArgs, int, float);
    FUNCTION().PUBLIC().SIGNATURE(const o2::String&, ConstRefReturn);
    FUNCTION().PUBLIC().SIGNATURE(o2::Vector<int>, VectorReturn);
    FUNCTION().PUBLIC().SIGNATURE(void, RefArg, const o2::String&);
    FUNCTION().PUBLIC().SIGNATURE(void, PointerArg, Value*);
    FUNCTION().PUBLIC().SIGNATURE(void, GluedPointerArg, Value*);
    FUNCTION().PUBLIC().SIGNATURE(Value*, PointerReturn);
    FUNCTION().PUBLIC().SIGNATURE(Value*, GluedPointerReturn);
    FUNCTION().PUBLIC().SIGNATURE(void, MultiWordTypes, unsigned short, long long);
    FUNCTION().PUBLIC().SIGNATURE(unsigned short, GetMask);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, StaticFunction, int);
    FUNCTION().PUBLIC().SIGNATURE(void, VirtualFunction);
    FUNCTION().PUBLIC().SIGNATURE(void, DefaultArgs, int, bool);
}
END_META;
// --- END META ---
