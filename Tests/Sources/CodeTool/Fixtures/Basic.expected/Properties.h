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
// --- META ---

CLASS_BASES_META(game::Widget)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(game::Widget)
{
    FIELD().PUBLIC().NAME(transparency);
    FIELD().PUBLIC().NAME(isVisible);
    FIELD().PUBLIC().NAME(order);
    FIELD().PUBLIC().NAME(param);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mTransparency);
    FIELD().PROTECTED().ATTRIBUTE(o2::TagAttribute).ATTRIBUTE(UnknownAttribute).DEFAULT_VALUE(5).NAME(attributed);
}
END_META;
CLASS_METHODS_META(game::Widget)
{

    FUNCTION().PUBLIC().SIGNATURE(void, SetTransparency, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTransparency);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsVisible);
    FUNCTION().PUBLIC().SIGNATURE(void, SetOrder, int);
    FUNCTION().PUBLIC().SIGNATURE(float, GetParam, int);
}
END_META;
// --- END META ---
