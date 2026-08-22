#pragma once

#include "TestBase.h"

namespace game
{
    // Template class: metas go to header, no DECLARE_CLASS
    template<typename _type>
    class Container: public o2::IObject
    {
    public:
        _type value; // @SERIALIZABLE

        int size = 0;

        Container();

        _type& Get();
        void Set(const _type& value);

        IOBJECT(Container);

        // Nested class inside template
        class Node: public o2::IObject
        {
        public:
            int index = 0;

            IOBJECT(Node);
        };
    };

    // Template with two parameters
    template<typename _key, typename _value>
    struct PairHolder: public o2::IObject
    {
        _key first;
        _value second;

        IOBJECT(PairHolder);
    };
}
// --- META ---

META_TEMPLATES(typename _type)
CLASS_BASES_META(game::Container<_type>)
{
    BASE_CLASS(o2::IObject);
}
END_META;
META_TEMPLATES(typename _type)
CLASS_FIELDS_META(game::Container<_type>)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(value);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(size);
}
END_META;
META_TEMPLATES(typename _type)
CLASS_METHODS_META(game::Container<_type>)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(_type&, Get);
    FUNCTION().PUBLIC().SIGNATURE(void, Set, const _type&);
}
END_META;

META_TEMPLATES(typename _key, typename _value)
CLASS_BASES_META(game::PairHolder<_key, _value>)
{
    BASE_CLASS(o2::IObject);
}
END_META;
META_TEMPLATES(typename _key, typename _value)
CLASS_FIELDS_META(game::PairHolder<_key, _value>)
{
    FIELD().PUBLIC().NAME(first);
    FIELD().PUBLIC().NAME(second);
}
END_META;
META_TEMPLATES(typename _key, typename _value)
CLASS_METHODS_META(game::PairHolder<_key, _value>)
{
}
END_META;

META_TEMPLATES(typename _type)
CLASS_BASES_META(game::Container<_type>::Node)
{
    BASE_CLASS(o2::IObject);
}
END_META;
META_TEMPLATES(typename _type)
CLASS_FIELDS_META(game::Container<_type>::Node)
{
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(index);
}
END_META;
META_TEMPLATES(typename _type)
CLASS_METHODS_META(game::Container<_type>::Node)
{
}
END_META;
// --- END META ---
