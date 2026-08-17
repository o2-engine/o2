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
