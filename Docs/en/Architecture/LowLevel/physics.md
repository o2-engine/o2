## Physics
The Box2D physics engine is integrated into the engine. It is integrated into the scene and operates with physical bodies (RigidBody), shapes (Colliders) and joints (not yet available)

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
- `o2::SplineCollider` - spline
- `o2::SplineMeshCollider` - mesh along a spline
