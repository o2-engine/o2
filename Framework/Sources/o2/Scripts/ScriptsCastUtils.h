#pragma once

#include "o2/Utils/Reflection/Type.h"

namespace o2
{
    // Casts an object pointer of a reflected type to a target type walking the reflection
    // hierarchy. The per-base cast functions are instantiated in each class' own translation unit,
    // which keeps them correct where a template-side dynamic_cast may resolve a wrong layout.
    // Types are matched by name: inline Type statics may not coalesce across static libraries
    inline void* CastThroughReflection(const Type* fromType, void* object, const Type& targetType)
    {
        if (!fromType || !object)
            return nullptr;

        if (fromType == &targetType || fromType->GetName() == targetType.GetName())
            return object;

        for (auto& base : fromType->GetBaseTypes())
        {
            if (void* res = CastThroughReflection(base.type, base.dynamicCastUpFunc(object), targetType))
                return res;
        }

        return nullptr;
    }
}
