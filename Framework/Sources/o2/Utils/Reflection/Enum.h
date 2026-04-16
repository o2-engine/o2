#pragma once

#include <type_traits>

namespace o2
{
    template<typename _type>
    class IsEnumReflectable: public std::false_type {};

#define PRE_ENUM_META(NAME)                                      \
    template<>                                                   \
    class o2::IsEnumReflectable<NAME>: public std::true_type {};

#define ENUM_META(NAME, REGISTRATOR_ID)                                                                    \
    extern void __RegisterEnum__##REGISTRATOR_ID()                                                         \
    {                                                                                                      \
        o2::EnumTypeContainer<NAME>::type = o2::Reflection::InitializeEnum<NAME>(#NAME, []() {             \
        typedef NAME EnumName;                                                                             \
        o2::Map<int, o2::String> res;

#define ENUM_ENTRY(NAME) \
    res.Add((int)EnumName::NAME, #NAME)

#define END_ENUM_META \
    return res; }); }
}
