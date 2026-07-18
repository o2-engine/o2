#pragma once

#include "o2/Utils/Math/Basis3D.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    class ParticlesEmitter;

    // --------------------------------------
    // Particles emitter shape base interface
    // --------------------------------------
    class ParticlesEmitterShape: public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        // Virtual destructor
        virtual ~ParticlesEmitterShape() {}

        // Returns random emitting point in shape. Local shape space is a unit square/cube
        // with center at (0.5, 0.5, 0); z is used by volumetric shapes in 3D emitters
        virtual Vec3F GetEmittinPoint(const Basis3D& transform, bool fromShell);

        SERIALIZABLE(ParticlesEmitterShape);

    protected:
        WeakRef<ParticlesEmitter> mEmitter; // Owning emitter

    protected:
        // Called  when particle effect parameters are changed, used to invalidate baked frames
        void OnChanged();

        friend class ParticlesEmitter;
    };

    // ---------------------------------
    // Circle with radius emitting shape
    // ---------------------------------
    class CircleParticlesEmitterShape: public ParticlesEmitterShape
    {
    public:
        // Default constructor @SCRIPTABLE
        CircleParticlesEmitterShape() {}

        // Returns random emitting point in circle
        Vec3F GetEmittinPoint(const Basis3D& transform, bool fromShell) override;

        SERIALIZABLE(CircleParticlesEmitterShape);
        CLONEABLE_REF(CircleParticlesEmitterShape);
    };

    // ---------------------------------
    // Square with radius emitting shape
    // ---------------------------------
    class SquareParticlesEmitterShape: public ParticlesEmitterShape
    {
    public:
        // Default constructor @SCRIPTABLE
        SquareParticlesEmitterShape() {}

        // Returns random emitting point in square
        Vec3F GetEmittinPoint(const Basis3D& transform, bool fromShell) override;

        SERIALIZABLE(SquareParticlesEmitterShape);
        CLONEABLE_REF(SquareParticlesEmitterShape);
    };

    // ------------------------------------------
    // Sphere emitting shape, volumetric emitting
    // ------------------------------------------
    class SphereParticlesEmitterShape: public ParticlesEmitterShape
    {
    public:
        // Returns random emitting point in sphere
        Vec3F GetEmittinPoint(const Basis3D& transform, bool fromShell) override;

        SERIALIZABLE(SphereParticlesEmitterShape);
        CLONEABLE_REF(SphereParticlesEmitterShape);
    };
}
// --- META ---

CLASS_BASES_META(o2::ParticlesEmitterShape)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::ParticlesEmitterShape)
{
    FIELD().PROTECTED().NAME(mEmitter);
}
END_META;
CLASS_METHODS_META(o2::ParticlesEmitterShape)
{

    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetEmittinPoint, const Basis3D&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChanged);
}
END_META;

CLASS_BASES_META(o2::CircleParticlesEmitterShape)
{
    BASE_CLASS(o2::ParticlesEmitterShape);
}
END_META;
CLASS_FIELDS_META(o2::CircleParticlesEmitterShape)
{
}
END_META;
CLASS_METHODS_META(o2::CircleParticlesEmitterShape)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetEmittinPoint, const Basis3D&, bool);
}
END_META;

CLASS_BASES_META(o2::SquareParticlesEmitterShape)
{
    BASE_CLASS(o2::ParticlesEmitterShape);
}
END_META;
CLASS_FIELDS_META(o2::SquareParticlesEmitterShape)
{
}
END_META;
CLASS_METHODS_META(o2::SquareParticlesEmitterShape)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetEmittinPoint, const Basis3D&, bool);
}
END_META;

CLASS_BASES_META(o2::SphereParticlesEmitterShape)
{
    BASE_CLASS(o2::ParticlesEmitterShape);
}
END_META;
CLASS_FIELDS_META(o2::SphereParticlesEmitterShape)
{
}
END_META;
CLASS_METHODS_META(o2::SphereParticlesEmitterShape)
{

    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetEmittinPoint, const Basis3D&, bool);
}
END_META;
// --- END META ---
