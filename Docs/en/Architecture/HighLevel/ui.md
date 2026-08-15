## UI system
For building user interfaces there is a set of special actors that can draw UI and handle clicks on it.

Everything works within the scene, through the `o2::Actor` descendant `o2::Widget`. All other UI actor types derive from it.

### o2::Widget
Inherits all actor properties: name, transform, components, prototyping. It also adds new ones:

#### layout
An extension of the transform that adds adaptive layout. It defines coordinates relative to the parent transform, to which numeric offsets are then added. Relative coordinates allow aligning to an edge or corner, as well as setting the percentage of occupied area horizontally and vertically. Numeric offsets allow specifying the area more precisely, or setting an explicit size.

#### Layers
UI graphics are drawn with layers, which are a simplified version of an actor. Layers are also organized into a structure; each layer has its own layout and a graphical element. The graphical element is any descendant of `o2::IRectDrawable`: Sprite, Text, etc.

Layers can be disabled and have configurable transparency, which also affects child layers.

#### States
An elementary animation state machine: a list of states is defined, and each state can be on or off. The transition between on and off happens through an animation defined by an animation clip.

#### Transparency and visibility
Transparency is set on the widget and propagates to its layers, as well as to all child widgets and so on. The composed layer transparency is written into the drawable color alpha, so authoring translucency belongs on the widget or layer transparency, not in the sprite color alpha (which any transparency update overwrites).

Visibility is controlled via the actor's `SetEnabled()` interface and can be animated. If a special state named 'visible' is defined, it is used to toggle visibility.

#### Focus
Some widgets can take focus, e.g. buttons, edit boxes, etc. They then become first in line for hotkey handling.

#### Internal children
Sometimes UI elements are complex enough that layers alone are not sufficient. Consider a window: it is much more convenient when the window is a single object and all its children are its content. Yet the window itself has a caption and a close button.

Widgets allow making such elements internal: they behave like children, but enumerating the widget's children reveals nothing about them.

## Built-in widget types
- `Grid/Horizontal/VerticalLayout` - dynamic arrangement of children in a grid / horizontally / vertically
- `Button` - button
- `EditBox` - text input field
- `Image` - sprite
- `Label` - text
- `Toggle` - flag, checkbox
- `Horizontal/VerticalScrollBar` - horizontal/vertical scroll bar
- `Horizontal/VerticalProgress` - progress bars
- `ScrollArea` - scrolling area with content clipping
- `List/LongList` - lists, `CustomList` - list with arbitrary item widgets
- `DropDown` - drop-down list, `CustomDropDown` - with arbitrary item widgets
- `Tree` - tree
- `ContextMenu` - context menu
- `MenuPanel` - menu panel
- `Window` - window with a caption and a close button
- `Spoiler` - collapsible container
- `PopupWidget` - popup widget drawn on top of the rest of the UI
