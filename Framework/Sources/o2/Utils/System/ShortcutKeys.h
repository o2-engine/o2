#pragma once

#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // -------------------------
    // Shortcut keys description
    // -------------------------
    class ShortcutKeys: public ISerializable
    {
    public:
        Vector<KeyboardKey> keys; // Keys that should be pressed // @SERIALIZABLE

        String custromString; // Custom string representation

    public:
        // Default constructor
        ShortcutKeys();

        // Constructor by array of keys
        ShortcutKeys(const Vector<KeyboardKey>& keys);

        // Returns true if all keys are pressed
        bool IsPressed() const;

        // Returns true if all keys are down
        bool IsDown() const;

        // Returns string representation
        String AsString() const;

        // Returns true if all keys are empty
        bool IsEmpty() const;

        // Check equals operator
        bool operator==(const ShortcutKeys& other) const;

        // Less operator
        bool operator<(const ShortcutKeys& other) const;

        // Builds with custom string
        static ShortcutKeys FromCustomString(const String& str);

        SERIALIZABLE(ShortcutKeys);

    private:
        static const std::unordered_map<KeyboardKey, String> mKeyNames;
            
    private:
        // Normalize key: convert lowercase to uppercase for macOS
        KeyboardKey NormalizeKey(KeyboardKey key) const;
    };
}
// --- META ---

CLASS_BASES_META(o2::ShortcutKeys)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::ShortcutKeys)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(keys);
    FIELD().PUBLIC().NAME(custromString);
}
END_META;
CLASS_METHODS_META(o2::ShortcutKeys)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<KeyboardKey>&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPressed);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsDown);
    FUNCTION().PUBLIC().SIGNATURE(String, AsString);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEmpty);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(ShortcutKeys, FromCustomString, const String&);
    FUNCTION().PRIVATE().SIGNATURE(KeyboardKey, NormalizeKey, KeyboardKey);
}
END_META;
// --- END META ---
