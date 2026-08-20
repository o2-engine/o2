#pragma once

namespace o2
{
    // Basic object interface
    class IObject
    {
    public:
        virtual ~IObject() {}
    };

    // Attribute interface
    class IAttribute
    {
    public:
        virtual ~IAttribute() {}
    };

    // Serialization attribute
    class SerializableAttribute: public IAttribute
    {
    public:
        ATTRIBUTE_COMMENT_DEFINITION("SERIALIZABLE");
        ATTRIBUTE_SHORT_DEFINITION("SERIALIZABLE_ATTRIBUTE");
    };

    // Range attribute, used with parameters
    class RangeAttribute: public IAttribute
    {
    public:
        ATTRIBUTE_COMMENT_DEFINITION("RANGE");
        ATTRIBUTE_SHORT_DEFINITION("RANGE_ATTRIBUTE");
    };

    // Attribute without short definition
    class TagAttribute: public IAttribute
    {
    public:
        ATTRIBUTE_COMMENT_DEFINITION("TAG");
    };
}
