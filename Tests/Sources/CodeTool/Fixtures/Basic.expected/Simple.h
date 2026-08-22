#pragma once

#include "TestBase.h"
#include <map>

namespace game
{
    // Simple reflected class with fields and methods
    class Health: public o2::IObject
    {
    public:
        float current = 100.0f;  // @SERIALIZABLE
        float maxValue = 100.0f; // @SERIALIZABLE @RANGE(0, 200)
        float regenRate = 1.0f;  // @TAG
        float hiddenValue = 1.0f; // @IGNORE

        static int instancesCount;

        int untouched;

    public:
        Health() {}
        Health(float value);

        virtual ~Health();

        void Heal(float amount);
        float GetCurrent() const;
        static Health* Create();

        bool operator==(const Health& other) const;

        std::map<int, float> GetTable() const;
        void SetTable(const std::map<int, float>& table);

        template<typename T>
        T GetAs() const;

        IOBJECT(Health);

    protected:
        void OnDamage(float amount); // @IGNORE

    private:
        float mInternal = 0.5f;
    };

    // Derived class with multiple bases
    class Armor: public Health, protected o2::IAttribute
    {
    public:
        int defense = 10; // @SERIALIZABLE

        SERIALIZABLE(Armor);
    };

    // Class without reflection markers - must be skipped
    class Helper: public o2::IObject
    {
    public:
        int data;
    };

    // Class not derived from IObject - must be skipped
    class Loose
    {
    public:
        int data;

        IOBJECT(Loose);
    };
}
// --- META ---

CLASS_BASES_META(game::Health)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(game::Health)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(100.0f).NAME(current);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().RANGE_ATTRIBUTE(0, 200).DEFAULT_VALUE(100.0f).NAME(maxValue);
    FIELD().PUBLIC().ATTRIBUTE(o2::TagAttribute()).DEFAULT_VALUE(1.0f).NAME(regenRate);
    FIELD().PUBLIC().NAME(untouched);
    FIELD().PRIVATE().DEFAULT_VALUE(0.5f).NAME(mInternal);
}
END_META;
CLASS_METHODS_META(game::Health)
{

    typedef std::map<int, float> _tmp1;
    typedef const std::map<int, float>& _tmp2;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(float);
    FUNCTION().PUBLIC().SIGNATURE(void, Heal, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetCurrent);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Health*, Create);
    FUNCTION().PUBLIC().SIGNATURE(_tmp1, GetTable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTable, _tmp2);
}
END_META;

CLASS_BASES_META(game::Armor)
{
    BASE_CLASS(game::Health);
    BASE_CLASS(o2::IAttribute);
}
END_META;
CLASS_FIELDS_META(game::Armor)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(10).NAME(defense);
}
END_META;
CLASS_METHODS_META(game::Armor)
{
}
END_META;
// --- END META ---
