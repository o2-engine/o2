#pragma once
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Attributes.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Serialization/Serializable.h"

namespace o2
{
    class PrototypeDeltaSearchAttribute: public IAttribute
    {
        ATTRIBUTE_COMMENT_DEFINITION("DELTA_SEARCH");
        ATTRIBUTE_SHORT_DEFINITION("DELTA_SEARCH_ATTRIBUTE");
    };

    class IgnorePrototypeDeltaSearchAttribute: public IAttribute
    {
        ATTRIBUTE_COMMENT_DEFINITION("IGNORE_DELTA_SEARCH");
        ATTRIBUTE_SHORT_DEFINITION("IGNORE_DELTA_SEARCH_ATTRIBUTE");
    };

    // Includes field in prototype delta search when the condition function returns true
    template<typename _class>
    class PrototypeDeltaSearchIfAttribute: public ISerializeIfAttribute
    {
        ATTRIBUTE_COMMENT_DEFINITION("DELTA_SEARCH_IF");
        ATTRIBUTE_SHORT_DEFINITION("DELTA_SEARCH_IF_ATTRIBUTE");

    public:
        bool(_class::*functionPtr)() const;

        PrototypeDeltaSearchIfAttribute(bool(_class::*functionPtr)() const): functionPtr(functionPtr) {}

        bool Invoke(const _class* object) const { return (object->*functionPtr)(); }
        bool Invoke(const void* object) const override { return Invoke((_class*)object); }
    };

#define DELTA_SEARCH_ATTRIBUTE() \
    template AddAttribute<o2::PrototypeDeltaSearchAttribute>()

#define IGNORE_DELTA_SEARCH_ATTRIBUTE() \
    template AddAttribute<o2::IgnorePrototypeDeltaSearchAttribute>()

#define DELTA_SEARCH_IF_ATTRIBUTE(FUNC) \
    template AddAttribute<o2::PrototypeDeltaSearchIfAttribute<thisclass>>(&thisclass::FUNC)
}
