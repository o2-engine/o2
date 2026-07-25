#include "o2/stdafx.h"
#include "SphericalJoint3D.h"

#include "o2/Physics/PhysicsWorld3D.h"
#include "o2/Utils/Math/Math.h"

namespace o2
{
    SphericalJoint3D::SphericalJoint3D()
    {}

    SphericalJoint3D::SphericalJoint3D(const SphericalJoint3D& other):
        IJoint3D(other), mEnableConeLimit(other.mEnableConeLimit), mConeAngle(other.mConeAngle),
        mEnableTwistLimit(other.mEnableTwistLimit), mLowerTwistAngle(other.mLowerTwistAngle),
        mUpperTwistAngle(other.mUpperTwistAngle)
    {}

    SphericalJoint3D& SphericalJoint3D::operator=(const SphericalJoint3D& other)
    {
        IJoint3D::operator=(other);
        mEnableConeLimit = other.mEnableConeLimit;
        mConeAngle = other.mConeAngle;
        mEnableTwistLimit = other.mEnableTwistLimit;
        mLowerTwistAngle = other.mLowerTwistAngle;
        mUpperTwistAngle = other.mUpperTwistAngle;
        return *this;
    }

    void SphericalJoint3D::SetEnableConeLimit(bool enable) { mEnableConeLimit = enable; RebuildJoint(); }
    bool SphericalJoint3D::IsConeLimitEnabled() const { return mEnableConeLimit; }
    void SphericalJoint3D::SetConeAngle(float degrees) { mConeAngle = degrees; RebuildJoint(); }
    float SphericalJoint3D::GetConeAngle() const { return mConeAngle; }
    void SphericalJoint3D::SetEnableTwistLimit(bool enable) { mEnableTwistLimit = enable; RebuildJoint(); }
    bool SphericalJoint3D::IsTwistLimitEnabled() const { return mEnableTwistLimit; }
    void SphericalJoint3D::SetLowerTwistAngle(float degrees) { mLowerTwistAngle = degrees; RebuildJoint(); }
    float SphericalJoint3D::GetLowerTwistAngle() const { return mLowerTwistAngle; }
    void SphericalJoint3D::SetUpperTwistAngle(float degrees) { mUpperTwistAngle = degrees; RebuildJoint(); }
    float SphericalJoint3D::GetUpperTwistAngle() const { return mUpperTwistAngle; }

    String SphericalJoint3D::GetName() { return "Spherical joint 3D"; }
    String SphericalJoint3D::GetCategory() { return "Physics 3D/Joints"; }
    bool SphericalJoint3D::IsAvailableFromCreateMenu() { return true; }

    b3JointId SphericalJoint3D::CreateJoint(RigidBody3D* bodyA, RigidBody3D* bodyB)
    {
        b3SphericalJointDef def = b3DefaultSphericalJointDef();
        SetupBaseDef(def.base, bodyA, bodyB);

        def.enableConeLimit = mEnableConeLimit;
        def.coneAngle = Math::Deg2rad(mConeAngle);
        def.enableTwistLimit = mEnableTwistLimit;
        def.lowerTwistAngle = Math::Deg2rad(mLowerTwistAngle);
        def.upperTwistAngle = Math::Deg2rad(mUpperTwistAngle);

        return b3CreateSphericalJoint(o2Physics3D.GetWorldId(), &def);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::SphericalJoint3D>);
// --- META ---

DECLARE_CLASS(o2::SphericalJoint3D, o2__SphericalJoint3D);
// --- END META ---
