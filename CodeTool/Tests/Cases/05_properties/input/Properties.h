#pragma once
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Property.h"

namespace test
{
    class Props: public o2::ISerializable
    {
    public:
        PROPERTIES(Props);
        PROPERTY(int, value, SetValue, GetValue);          // Value property
        GETTER(float, readOnly, GetReadOnly);              // Read only getter
        SETTER(bool, writeOnly, SetWriteOnly);             // Write only setter

        void SetValue(int v);
        int GetValue() const;
        float GetReadOnly() const;
        void SetWriteOnly(bool v);

        SERIALIZABLE(Props);

    protected:
        int mValue = 0; // @SERIALIZABLE
    };
}
