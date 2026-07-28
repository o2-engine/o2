## Physics
The Box2D physics engine is integrated into the engine. It is integrated into the scene and operates with physical bodies (RigidBody), shapes (Colliders) and joints.

### Physical body, o2::RigidBody
This is a descendant of `o2::Actor` that binds to the dynamics of a Box2D physical body.

It defines mass, inertia, velocity and behavior. Behavior can be of 3 types:
- Dynamic - dynamic, affects other bodies and other bodies affect it
- Static - static body, does not move, pushes other bodies away
- Kinematic - a moving static body, pushes others away, but other bodies cannot affect it

### Shapes, colliders
Used to define the shape of rigid bodies, descendants of `o2::ICollider`:
- `o2::BoxCollider` - rectangle
- `o2::CircleCollider` - circle
- `o2::SplineCollider` - spline; in loop mode the chain also samples the closing curve segment (last key -> first key)
- `o2::SplineMeshCollider` - mesh along a spline

### Joints
Descendants of `o2::IJoint` (a `Component`) connect two `RigidBody` actors referenced by serialized
`LinkRef<RigidBody>` fields (bodyA/bodyB) plus `collideConnected`. The joint's own actor position is
the pivot/anchor. The joint is created in `OnStart` (retried each `OnUpdate` until both bodies exist)
and destroyed on removal. Available types: `o2::DistanceJoint`, `o2::RevoluteJoint`,
`o2::PrismaticJoint`, `o2::WeldJoint` (the remaining Box2D types — friction, motor, rope, pulley,
mouse, gear — follow the same `IJoint` pattern).

## 3D Physics
The Box3D physics engine (Erin Catto, C17, MIT) is integrated alongside Box2D for 3D rigid body
simulation. It is a git submodule at `Framework/3rdPartyLibs/box3d` (pinned; its core library is
built inline by `3rdPartyLibs/CMakeLists.txt`, not via upstream's own CMake) and wrapped by the
`o2::PhysicsWorld3D` singleton, owned by `o2::Integration` and stepped in the same fixed 60 Hz loop
as the 2D world. `o2Config.physics3D` holds gravity, world-to-physics scale and the solver
sub-step count.

### 3D physical body, o2::RigidBody3D
A descendant of `o2::Actor` bound to a Box3D body. Same three behavior types as 2D (Dynamic, Static,
Kinematic). Exposes linear/angular velocity, damping, gravity scale, continuous collision (bullet)
and rotation lock.

Transform sync keeps the Box3D quaternion authoritative: each step the simulated pose is written
back to the actor, and the actor pose is pushed into the body only when it was moved externally (a
dirty check). This avoids round-tripping the actor's euler-stored rotation back into the simulation.
World rotation is read/written through `ActorTransform::GetWorldRotation` / `SetWorldRotation`.

### 3D shapes, colliders
Descendants of `o2::ICollider3D` (density, friction, restitution, sensor):
- `o2::BoxCollider3D` - box
- `o2::SphereCollider3D` - sphere
- `o2::CapsuleCollider3D` - capsule (aligned with the collider's local Y axis)

### 3D joints
Descendants of `o2::IJoint3D` connect two `RigidBody3D` actors (`LinkRef<RigidBody3D>` bodyA/bodyB).
The joint frame on each body is derived from this actor's world transform. Available types:
`o2::DistanceJoint3D`, `o2::RevoluteJoint3D`, `o2::PrismaticJoint3D`, `o2::WeldJoint3D`,
`o2::SphericalJoint3D` (the remaining Box3D types — motor, wheel, parallel, filter — follow the same
`IJoint3D` pattern).

### Editor gizmos
Colliders and joints draw themselves through the common gizmos system (`OnDrawGizmos`, see
[scene](/Docs/en/Architecture/HighLevel/scene.md)): box/circle wireframe for 2D, box/sphere/capsule
for 3D and joint connection lines (bodyA → anchor → bodyB) for `IJoint`/`IJoint3D`. They are drawn
for all scene objects, not only the selected ones; switching them on and off is done in the Gizmos
panel of the scene window.

Not yet available: mesh / height-field colliders, runtime physics debug draw.
