#pragma once

#include "TestBase.h"

namespace game
{
    class Widget: public o2::IObject
    {
    public:
        PROPERTIES(Widget);
        PROPERTY(float, transparency, SetTransparency, GetTransparency); // Transparency property
        GETTER(bool, isVisible, IsVisible);                              // Visibility getter
        SETTER(int, order, SetOrder);
        ACCESSOR(float, param, int, GetParam, GetAllParams);

        void SetTransparency(float value);
        float GetTransparency() const;
        bool IsVisible() const;
        void SetOrder(int order);
        float GetParam(int id);

        IOBJECT(Widget);

    protected:
        float mTransparency = 1.0f; // @SERIALIZABLE

        ATTRIBUTES(o2::TagAttribute, UnknownAttribute);
        int attributed = 5;
    };
}
