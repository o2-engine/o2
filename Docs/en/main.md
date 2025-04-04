## Motivation

The engine is designed as a professional tool for developing 2D mobile games. Its design is guided by two key objectives: development speed and performance.

Development speed is ensured by an intuitive API, an editor, and a scripting language.
- A user-friendly API means it covers all the developer's use cases, is easy to understand, and has minimal hidden pitfalls. Unity3D's internal API served as a reference in many aspects.  
- The editor provides all the essential tools for game development: scene, assets, parameters, animations, particles. It is convenient and follows the modern standards of engines with editors, such as Unity3D, Unreal Engine, Unigine, etc.  
- The scripting language simplifies writing game logic and makes the engine's API even more accessible. o2 uses JavaScript and TypeScript for scripting, as these are popular languages, especially among web developers, who have very few available game engines.

Performance is achieved by implementing the engine in C++ and allowing game logic to be written in C++. The engine also has different abstraction layers, so for maximum performance, the developer can work directly with the low-level subsystems like rendering, resource loading, etc.

Combining all these approaches results in efficient game development, rapid prototyping, and high technical quality.

## General Engine Concept

The engine’s core concept is a scene with a component-based approach. The scene is a tree structure of actors with specific types. Each actor has a transform that depends on its parent, as well as a set of components defining behavior and rendering. Rendering is divided into layers; actors can be assigned to specific layers or inherit the rendering order from their parent.

You configure the scene and its assets in the editor. Actor, component, and game code can be written in C++ or in a scripting language (JS/TS).

The scene subsystem is high-level and is built atop the low-level subsystems: assets, application, rendering, animation, physics, scripting, input. When working at the high level, you can still use the low-level layer directly for more optimizations. For instance, if you need to display a large number of sprites, instead of creating multiple heavy actors, you can implement a single actor that draws many sprites directly via the low-level renderer.

The low-level layer relies on utilities that provide support for all subsystems: memory management, reflection, serialization, properties, math, file system, debugging, and logging.

## The Concept of Integrating o2 Into Other Engines

o2 can be used not only as a standalone engine but also as an embedded one. Essentially, it integrates into the game loop of another engine, hooks into its rendering and input processing, and interacts with it according to clearly defined boundaries.

There are two approaches: **dominant** and **subordinate**. In the first approach, o2 wraps the other engine inside itself, controlling its input handling, frame updates, and rendering. This is the preferred way. In the second approach, everything is reversed: o2 is embedded into the other engine’s game loop.

Typically, other engines also have a scene hierarchy. o2 can work with it if you inherit the base object of the other engine's hierarchy from a certain o2 interface. This allows o2’s editor to display the other engine’s hierarchy and lets you edit object parameters, which become available through reflection.

It is important that the other engine uses the same render API as o2. In extreme cases, it’s possible to implement a custom rendering subsystem for the specific implementation in the other engine, since essentially all that’s needed is the ability to draw textured, shaded triangle meshes. Usually, other engines have this functionality in a separate API.

## Documentation

- [Architecture](/Docs/en/Architecture/architecture.md)  
- [Editor](/Docs/en/Editor/editor.md)  
- [Video tutorial series on setting up a test slot-game scene](https://drive.google.com/drive/folders/1m-lgSaM2hYQxbKnIwymDfMCe3SzjKrP1?usp=sharing)  