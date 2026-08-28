## Built-in components
The engine ships with a basic set of components:

- `o2::ImageComponent`  
Wrapper over the render `o2::Sprite`, can do all the same things

- `o2::MeshComponent`  
Wrapper over the engine `o2::Mesh`, with additional editor tooling for building the mesh

- `o2::SkinningMeshComponent`  
2D mesh with skeletal skinning. Bones are defined by child actors with `o2::SkinningMeshBoneComponent`. Spine-like animations can be made with it

- `o2::SkinnedMeshComponent`  
Draws a skinned 3D model from a `SkinnedModelAsset`. The pose comes from bone actors or the built-in clip player, skinning runs on the GPU

- `o2::AnimationComponent`  
Animation component. Holds a list of animations that can change the actor, its components and everything below in the hierarchy

- `o2::AnimationStateGraphComponent`  
Graph of animation states and transitions between them

- `o2::SpineComponent`  
Plays EsotericSoftware Spine animations, derived from `o2::AnimationComponent`

- `o2::ParticlesEmitterComponent`  
Wrapper over the render particle emitter. Can do all the same things

- `o2::FlightTrajectoryComponent`  
Flies the actor along a spline trajectory. The spline (key ranges give a random offset corridor) is geometrically mapped between the start and finish points; progress is driven by the animatable `position` property 0..1 (`SetPosition`); the trajectory point is written into the actor transform (widgets get layout offsets) immediately when the property is set — including editor scrubbing — and on each update. The random offset within the corridor changes only by an explicit `ResetRandomOffset()` (the game calls it before a flight; the trajectory stays stable while scrubbing in the editor). The spline is edited in the scene with the spline tool when the component is selected

- `o2::ScissorClippingComponent`  
Clips child objects by its transform. Clipping only affects entities that inherit their drawing depth from it

- `o2::ScriptableComponent`  
Component whose behavior is defined by a JS script

- `o2::SoundComponent`  
Plays a sound at the actor position, also controllable from the animation editor

- `o2::SoundListenerComponent`  
Spatial audio listener at the actor position and orientation

- `o2::LightComponent`  
Light source. Position and direction are taken from the actor transform, consumed by the lighting render passes

- `o2::Mesh3DComponent`  
Draws 3D geometry from a `Mesh3DAsset` with the actor transform

- `o2::MeshPrimitiveComponent`  
Builds parametric 3D geometry (box, sphere, plane, cylinder)

- `o2::TextSplitterComponent`  
Splits text into symbols and creates an actor with `o2::TextSymbolComponent` per symbol

- `o2::ICollider`: `BoxCollider`, `CircleCollider`, `SplineCollider`, `SplineMeshCollider`  
Physics colliders, used together with the `o2::RigidBody` actor
