#include "o2/stdafx.h"
#include "ScriptValueDef.h"
// --- META ---

ENUM_META(o2::ScriptValue::ValueType, o2__ScriptValue__ValueType)
{
    ENUM_ENTRY(Array);
    ENUM_ENTRY(BigInt);
    ENUM_ENTRY(Bool);
    ENUM_ENTRY(Error);
    ENUM_ENTRY(Function);
    ENUM_ENTRY(None);
    ENUM_ENTRY(Null);
    ENUM_ENTRY(Number);
    ENUM_ENTRY(Object);
    ENUM_ENTRY(String);
    ENUM_ENTRY(Symbol);
    ENUM_ENTRY(Undefined);
}
END_ENUM_META;
// --- END META ---
