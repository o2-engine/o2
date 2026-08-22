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
