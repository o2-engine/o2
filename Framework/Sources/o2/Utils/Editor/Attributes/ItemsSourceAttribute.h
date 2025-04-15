#pragma once
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Attributes.h"
#include "o2/Utils/Reflection/Reflection.h"

namespace o2
{
    class ItemsSourceAttribute : public IAttribute
    {
        ATTRIBUTE_COMMENT_DEFINITION("ITEMS_SOURCE");
        ATTRIBUTE_SHORT_DEFINITION("ITEMS_SOURCE_ATTRIBUTE");

    public:
        String methodName;

    public:
        ItemsSourceAttribute() { }
        ItemsSourceAttribute(const String& methodName):methodName(methodName) { }
    };

#define ITEMS_SOURCE_ATTRIBUTE(methodName) \
    template AddAttribute<o2::ItemsSourceAttribute>(#methodName)
}
