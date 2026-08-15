## Scene
The scene is another engine subsystem, built on top of the low-level systems.

The scene consists of a hierarchy of actors and layers. It can be loaded and unloaded, and can be modified at runtime.

### Hierarchy
Every actor can have child actors, which in turn can have their own children. The scene stores a list of "root" actors. Together this forms the actor hierarchy of the scene.

Actors can be of different types, derived from the base `o2::Actor`.

Actors can have components. Components define the actor's logic and can draw.

The layer and drawing depth are set on the actor itself; drawing components are rendered together with their actor.

### Layers
The scene is split into layers. Layers define the drawing order of the scene. The layer order, list and names are defined by the developer.

Drawable entities are distributed across layers, one entity can belong to only one layer.

Within a layer, entities are sorted by depth.

### Actor, o2::Actor
The basic scene element. It has a name, a transform, a list of children and a list of components. Actors can be of different types derived from `o2::Actor`.

Actors have a life cycle:
- `constructor` - actor constructor
- `deserialization` - deserialization of actor data. Happens automatically
- `OnDeserialized` - callback on deserialization completion
- `OnAddToScene`/`OnRemoveFromScene` - adding to / removing from the scene
- `OnStart` - actor start, before the first update
- `OnEnabled`/`OnDisabled` - enabling/disabling the actor
- `Update` - update of the actor and its components
- `OnTransformUpdated` - transform update. Fires once per transform update, for the actor and each of its components; heavy work (rebuilding geometry) belongs behind a dirty flag, evaluated when the result is actually needed

### Component, o2::Component
All components derive from this common interface. A component implements some logic that can interact with other parts of the scene and with assets through references.

Components, like actors, have the same events and life cycle: constructor, deserialization, start, update, etc.

### Gizmos, OnDrawGizmos
In the editor build only (`IS_EDITOR`) actors and components can draw helper graphics in the scene
window: override `OnDrawGizmos` and draw through the `o2::Gizmos` singleton
(`o2Gizmos.DrawLine/DrawCircle/DrawBox/DrawSphere/DrawCapsule/DrawRect`) in world coordinates — the
projection into the drawing space is set by the scene window, so the same calls work in both 2D and
3D view; in 3D view the window also passes the camera near plane, and geometry behind it is cut off
instead of being mirrored in front of the camera. Drawing only, no controls. Entry point is `DrawGizmos()`.
The editor draws gizmos of the **selected** objects only, walking each selection down its children, the
same subtree its selection outline covers — with nothing selected the scene draws no gizmos at all. The
types list of the control panel is collected from the whole scene, so a type can be switched off before
anything of it is selected. Gizmos are implemented for colliders (2D and 3D),
joints (`IJoint`, `IJoint3D`) and `o2::CameraActor` (perspective frustum or view rectangle). The
control panel is described in [editor scene](/Docs/en/Editor/Scene/scene.md).

### Drawable objects, o2::ISceneDrawable
This interface marks an object as able to draw on the scene; `o2::Actor` derives from it. It defines the layer and the drawing depth `drawDepth`. The higher the depth, the later the object is drawn within its layer.

Components are not registered in layers themselves: a drawing component overrides `OnDraw` and is drawn together with its actor.

The drawing depth is optional and can be inherited from the parent. In that case the parent draws this entity while drawing itself. If the parent has several such entities, it draws them in hierarchy order.

If there is no parent, the object is placed in a special layer container with depth 0, where such entities are also drawn in hierarchy order.

So an object with a negative depth is drawn before objects whose depth is not set, since theirs is zero.

By default the depth is not set and the parent's is used. That is, the scene is drawn in hierarchy traversal order.

### Prototypes
To speed up scene building and make work more convenient, prototypes can be used. These are pre-made pieces of scene hierarchy — essentially an actor with its children — that can then be "instantiated" into the scene, i.e. a copy of it is created.

If something is changed in one prototype instance in the editor, the change can be applied to the base prototype, and all other instances receive the same change. This works only in the editor.

Prototypes can be created from other prototypes, overriding some of their parameters. The final instance of a derived prototype then reflects changes of both the base and the derived prototype.

A prototype template outlives the scene, while a layer object (`o2::SceneLayer`) dies with it — so cloning rebinds the actor to the current scene's layer by name (just like deserialization does). Prototype instances stay valid after the scene is recreated.

### References
References are used to connect components, actors and assets to each other. They refer to entities by unique identifiers, automatically. This avoids looking up actors and their components by path in code. A path can break when the hierarchy changes, requiring code rewrites. With references, hierarchy changes do not break anything, everything keeps working.

Other actors are referenced via `o2::LinkRef<ActorType>`, components likewise via `o2::LinkRef<ComponentType>`. Assets are referenced via `o2::AssetRef<AssetType>`.

### Cameras, o2::CameraActor
Special actors that define how the scene is rendered. Without a camera nothing is drawn; there must be at least one in the scene.

A camera defines the list of layers it draws. The camera transform defines the visible area of the scene. It works like the render camera, a kind of "window" into scene space.
