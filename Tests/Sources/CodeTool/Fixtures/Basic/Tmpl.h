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
