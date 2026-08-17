#pragma once
#include "o2/Utils/Serialization/Serializable.h"

namespace test
{
    // Simple serializable class with a couple of fields
    class BasicClass: public o2::ISerializable
    {
    public:
        int   publicField = 5;   // @SERIALIZABLE
        float otherField;

        BasicClass();

        void DoSomething();
        int GetValue() const;

        SERIALIZABLE(BasicClass);

    protected:
        bool mProtectedField = false; // @SERIALIZABLE

    private:
        double mPrivateField;
    };
}
