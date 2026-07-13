#pragma once

#include "o2/Utils/Math/Vector3.h"
#include "o2/Utils/Serialization/Serializable.h"

namespace o2
{
    // ----------------------------------------------------------------------------------
    // Spatial audio listener point. Registers itself in the sound system; the first
    // listening one drives the listener, without any the listener follows render camera
    // ----------------------------------------------------------------------------------
    class SoundListener: virtual public ISerializable, public RefCounterable
    {
    public:
        // Default constructor. Registers listener in the sound system
        SoundListener();

        // Default constructor with ref counter
        explicit SoundListener(RefCounter* refCounter);

        // Copy-constructor
        SoundListener(const SoundListener& other);

        // Copy-constructor with ref counter
        SoundListener(RefCounter* refCounter, const SoundListener& other);

        // Destructor. Unregisters listener from the sound system
        ~SoundListener();

        // Assign operator
        SoundListener& operator=(const SoundListener& other);

        // Called by mmake after reference counter initialization; registers listener in the sound system
        void PostRefConstruct();

        // Sets listener position
        void SetPosition(const Vec3F& position);

        // Returns listener position
        Vec3F GetPosition() const;

        // Sets listener orientation
        void SetOrientation(const Vec3F& forward, const Vec3F& up);

        // Returns listener forward direction
        Vec3F GetForward() const;

        // Returns listener up direction
        Vec3F GetUp() const;

        // Returns true if this listener is able to drive the sound system
        virtual bool IsListening() const;

        // Returns true if this listener drives the sound system now
        bool IsActiveListener() const;

        SERIALIZABLE(SoundListener);

    protected:
        Vec3F mPosition;                    // Listener position
        Vec3F mForward = Vec3F(0, 0, -1);   // Listener forward direction
        Vec3F mUp = Vec3F(0, 1, 0);         // Listener up direction
    };
}
// --- META ---

CLASS_BASES_META(o2::SoundListener)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(o2::SoundListener)
{
    FIELD().PROTECTED().NAME(mPosition);
    FIELD().PROTECTED().DEFAULT_VALUE(Vec3F(0, 0, -1)).NAME(mForward);
    FIELD().PROTECTED().DEFAULT_VALUE(Vec3F(0, 1, 0)).NAME(mUp);
}
END_META;
CLASS_METHODS_META(o2::SoundListener)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const SoundListener&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SoundListener&);
    FUNCTION().PUBLIC().SIGNATURE(void, PostRefConstruct);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPosition, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetPosition);
    FUNCTION().PUBLIC().SIGNATURE(void, SetOrientation, const Vec3F&, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetForward);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetUp);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsListening);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsActiveListener);
}
END_META;
// --- END META ---
