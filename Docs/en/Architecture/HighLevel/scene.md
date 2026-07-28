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
- `OnTransformUpdated` - transform update

### Component, o2::Component
All components derive from this common interface. A component implements some logic that can interact with other parts of the scene and with assets through references.

Components, like actors, have the same events and life cycle: constructor, deserialization, start, update, etc.

### Gizmos, OnDrawGizmos
In the editor build only (`IS_EDITOR`) actors and components can draw helper graphics in the scene
window: override `OnDrawGizmos` and draw through the `o2::Gizmos` singleton
(`o2Gizmos.DrawLine/DrawCircle/DrawBox/DrawSphere/DrawCapsule/DrawRect`) in world coordinates — the
projection into the drawing space is set by the scene window, so the same calls work in both 2D and
3D view. Drawing only, no controls. Entry point is `DrawGizmos()`; the editor walks the scene itself
and lists only the types which override the method and are present on the scene. Gizmos are implemented for colliders (2D and 3D),
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

### References
References are used to connect components, actors and assets to each other. They refer to entities by unique identifiers, automatically. This avoids looking up actors and their components by path in code. A path can break when the hierarchy changes, requiring code rewrites. With references, hierarchy changes do not break anything, everything keeps working.

Other actors are referenced via `o2::LinkRef<ActorType>`, components likewise via `o2::LinkRef<ComponentType>`. Assets are referenced via `o2::AssetRef<AssetType>`.

### Cameras, o2::CameraActor
Special actors that define how the scene is rendered. Without a camera nothing is drawn; there must be at least one in the scene.

A camera defines the list of layers it draws. The camera transform defines the visible area of the scene. It works like the render camera, a kind of "window" into scene space.
