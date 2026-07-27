#include "o2/stdafx.h"
#include "DistanceJoint3D.h"

#include "o2/Config/ProjectConfig.h"
#include "o2/Physics/Box3DConvert.h"
#include "o2/Physics/PhysicsWorld3D.h"

namespace o2
{
    DistanceJoint3D::DistanceJoint3D()
    {}

    DistanceJoint3D::DistanceJoint3D(const DistanceJoint3D& other):
        IJoint3D(other), mLength(other.mLength), mEnableSpring(other.mEnableSpring), mHertz(other.mHertz),
        mDampingRatio(other.mDampingRatio), mEnableLimit(other.mEnableLimit), mMinLength(other.mMinLength),
        mMaxLength(other.mMaxLength)
    {}

    DistanceJoint3D& DistanceJoint3D::operator=(const DistanceJoint3D& other)
    {
        IJoint3D::operator=(other);
        mLength = other.mLength;
        mEnableSpring = other.mEnableSpring;
        mHertz = other.mHertz;
        mDampingRatio = other.mDampingRatio;
        mEnableLimit = other.mEnableLimit;
        mMinLength = other.mMinLength;
        mMaxLength = other.mMaxLength;
        return *this;
    }

    void DistanceJoint3D::SetLength(float length) { mLength = length; RebuildJoint(); }
    float DistanceJoint3D::GetLength() const { return mLength; }
    void DistanceJoint3D::SetEnableSpring(bool enable) { mEnableSpring = enable; RebuildJoint(); }
    bool DistanceJoint3D::IsSpringEnabled() const { return mEnableSpring; }
    void DistanceJoint3D::SetHertz(float hz) { mHertz = hz; RebuildJoint(); }
    float DistanceJoint3D::GetHertz() const { return mHertz; }
    void DistanceJoint3D::SetDampingRatio(float ratio) { mDampingRatio = ratio; RebuildJoint(); }
    float DistanceJoint3D::GetDampingRatio() const { return mDampingRatio; }
    void DistanceJoint3D::SetEnableLimit(bool enable) { mEnableLimit = enable; RebuildJoint(); }
    bool DistanceJoint3D::IsLimitEnabled() const { return mEnableLimit; }
    void DistanceJoint3D::SetMinLength(float value) { mMinLength = value; RebuildJoint(); }
    float DistanceJoint3D::GetMinLength() const { return mMinLength; }
    void DistanceJoint3D::SetMaxLength(float value) { mMaxLength = value; RebuildJoint(); }
    float DistanceJoint3D::GetMaxLength() const { return mMaxLength; }

    String DistanceJoint3D::GetName() { return "Distance joint 3D"; }
    String DistanceJoint3D::GetCategory() { return "Physics 3D/Joints"; }
    bool DistanceJoint3D::IsAvailableFromCreateMenu() { return true; }

    b3JointId DistanceJoint3D::CreateJoint(RigidBody3D* bodyA, RigidBody3D* bodyB)
    {
        float invScale = 1.0f/o2Config.physics3D.scale;

        // A distance joint anchors at each body's center (identity frame), keeping the centers at length.
        b3DistanceJointDef def = b3DefaultDistanceJointDef();
        def.base.bodyIdA = bodyA->GetBodyId();
        def.base.bodyIdB = bodyB->GetBodyId();
        def.base.localFrameA = b3Transform{ ToBox3D(Vec3F()), ToBox3D(Quat(0, 0, 0, 1)) };
        def.base.localFrameB = b3Transform{ ToBox3D(Vec3F()), ToBox3D(Quat(0, 0, 0, 1)) };
        def.base.collideConnected = GetCollideConnected();

        def.length = mLength*invScale;
        def.enableSpring = mEnableSpring;
        def.hertz = mHertz;
        def.dampingRatio = mDampingRatio;
        def.enableLimit = mEnableLimit;
        def.minLength = mMinLength*invScale;
        def.maxLength = mMaxLength*invScale;

        return b3CreateDistanceJoint(o2Physics3D.GetWorldId(), &def);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::DistanceJoint3D>);
// --- META ---

DECLARE_CLASS(o2::DistanceJoint3D, o2__DistanceJoint3D);
// --- END META ---
