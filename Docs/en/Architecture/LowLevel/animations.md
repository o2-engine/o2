## Animations
All animations in the engine inherit from the common interface `o2::IAnimation`. It defines the interface of an entity that can be updated; it has a start, a duration, an end and a current time.

The main animation class `o2::AnimationPlayer` inherits from it. It defines the animation target — the object the animation is applied to — and the animation clip.

### Animation clip, o2::AnimationClip
An animation clip contains a list of tracks with keys. Each track binds to a specific parameter of the target via reflection. The parameter is bound through a path like a directory path, "myClass/someObject/myProperty", which describes how to find the needed property of the object via reflection.

When the target is an actor, accessors are available in the path: `component/<type name>` — a component by type name (the full name including namespace: `component/o2::ParticlesEmitterComponent`; game types without a namespace go by their own name), `child/<name>` — a child actor. Example path to a child's component field: `child/Sparks/component/o2::ParticlesEmitterComponent`. A track with an unresolvable path or a mismatched value type is not bound and logs a warning.

A track also has a set of keys representing the key states of the parameter at specific moments in time and the interpolation method between them.

Track types:
- float - numeric, represented as a bezier curve
- bool - boolean, showing true/false on certain parts of the timeline
- color - color change over time
- vector2 - a combination of a movement spline in 2D space and a time curve: the value travels along the spline, and the time curve maps the track time to the normalized spline position from 0 (spline start) to 1 (spline end). Meant for movement trajectories; to animate a scale use two float tracks on the `scaleX`/`scaleY` properties instead of a vector2 track on `scale`
- vector3 - 3D vector animation
- sub-track (`o2::AnimationSubTrack`) - playback of a nested animation on a part of the timeline
- any custom ones

An animation clip can be created from code or from the editor. It can be saved to and loaded from an asset — `o2::AnimationAsset`.

There are 2 ways to create it from code:
- manually adding tracks and keys to them
- via the helper class `o2::Animate`

### o2::Animate
Allows defining sequences for some parameters — position, alpha, scale, rotation — in a way similar to natural language.

To do this, create an `o2::Animate` object, passing the animation target into it. Then, using the Move/Alpha/Scale/Rotate functions, the initial state of the animation is set.

The next state is separated by the `Then()` function, after which the next state of the parameters can be described, and the interpolation time set via `.For(sec)`, in seconds. A pause can also be set via `.Wait(sec)`.

### Animation target and initialization
The target must be an object represented in reflection, i.e. inherited from `o2::IObject`.

For any animation at all, an animation clip `o2::AnimationClip` must be set.

As soon as these two parameters are present, the tracks from the clip are bound to specific parameters of the target: for each track, the needed field is found by its path and handed over to the animation tracks via the `o2::IValueProxy` interface.

When the animation starts, all tracks start synchronously; during interpolation they pass new values through the proxies into the needed class fields or setter functions via properties.

### Animation on the scene, o2::AnimationComponent
This component works similarly to `o2::AnimationPlayer`, or rather uses it internally. It allows playing several clips on one object simultaneously, blending tracks with each other.

For example, the component has two clips animating some numeric value. They are defined as named states, with a weight. The greater the state's weight, the more influence it has on the final value of the parameter.

This way you can make smooth transitions between animations and run several animations simultaneously or in sequence.

### Animation state graph, o2::AnimationStateGraphComponent
This component plays an animation state graph from an `o2::AnimationStateGraphAsset` asset: states with lists of animations and transitions between them. It works on top of the `o2::AnimationComponent` of the same actor, controlling its states and weights during transitions.

### Spine
EsotericSoftware Spine is integrated for skeletal animations. The `o2::Spine` class wraps a skeleton and animation tracks (`o2::Spine::Track` — an `o2::IAnimation` descendant), the assets are `o2::SpineAsset` and `o2::SpineAtlasAsset`, and `o2::SpineComponent` is used on the scene.
