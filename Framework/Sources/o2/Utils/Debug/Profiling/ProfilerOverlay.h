#pragma once

// @CODETOOLIGNORE: the profiler classes are not reflected, and the whole feature is compiled
// out by O2_PROFILER, which generated registrations would not follow
#if defined(O2_PROFILER_ENABLED)

#include "o2/Events/ApplicationEventsListener.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    class Component;
    class ProfilerWidget;
    class Widget;
    class WidgetLayer;

    struct PerfCounter;
    struct PerfMetric;

    // ------------------------------------------------------------------------------------------------------
    // Owns the profiler widget and its screen root. Shows and hides it on F12 or on a long tap in the top left
    // corner, and drives NanoProfiler recording with the widget's visibility, so an invisible profiler costs
    // one branch per profiling scope.
    //
    // Lives outside the scene: the widget is drawn straight after the frame's debug drawables with the default
    // camera, which puts it above both the game and, in the editor, the whole editor UI
    // ------------------------------------------------------------------------------------------------------
    class ProfilerOverlay: public RefCounterable, public ApplicationEventsListener
    {
    public:
        static constexpr float longTapTime = 0.7f;         // Seconds the corner has to be held to toggle the widget
        static constexpr float longTapCornerSize = 100.0f; // Side of the toggling corner square, in pixels

    public:
        // Default constructor
        ProfilerOverlay();

        // Destructor
        ~ProfilerOverlay();

        // Returns the overlay owned by the integration, null when the render isn't initialized
        static ProfilerOverlay* InstancePtr();

        // Shows or hides the widget, switching the profiler recording with it
        void SetVisible(bool visible);

        // Returns true when the widget is shown
        bool IsVisible() const { return mVisible; }

        // Handles the show/hide input and updates the widget
        void Update(float dt);

        // Draws the widget inside the area, anchored to its top left corner, and claims this frame's
        // drawing: the editor's Game window calls it so the panel belongs to the window it measures
        void DrawIn(const RectF& area);

        // Draws the widget over the whole screen, unless DrawIn already drew it this frame
        void Draw();

        // Registers a continuously sampled metric, shown after the built-in ones
        void AddMetric(const PerfMetric& metric);

        // Registers an object counter, shown after the built-in ones
        void AddCounter(const PerfCounter& counter);

        // Returns the widget, creating it on the first call
        const Ref<ProfilerWidget>& GetWidget();

    protected:
        // ------------------------------------------------------------------------------------------
        // What the scene is made of. Collected in one pass over the scene actors a few times a second,
        // the panel's counters just read it off
        // ------------------------------------------------------------------------------------------
        struct SceneContent
        {
            int actors = 0;     // Actors on the scene, UI widgets included
            int ui = 0;         // Widgets among them
            int sprites = 0;    // Image components and widget layers drawing a sprite
            int texts = 0;      // Widget layers drawing text
            int animations = 0; // Animation and animation graph components
            int particles = 0;  // Particle emitters
            int models = 0;     // 2D, 3D, skinned and primitive meshes
            int spines = 0;     // Spine skeletons
            int lights = 0;     // Light sources
        };

    protected:
        Ref<Widget>         mRoot;   // Screen sized root, gives the widget its top left anchored layout
        Ref<ProfilerWidget> mWidget; // The panel itself, created on the first show

        SceneContent mSceneContent; // Last collected scene content counts

        bool mVisible = false;       // Is the widget shown
        bool mDrawnThisFrame = false; // Did DrawIn already place and draw the widget this frame

        Vec2F mLongTapOrigin;         // Cursor position the current press started at
        float mLongTapTime = 0.0f;    // Seconds the current press lasts
        bool  mLongTapTracking = false; // Is the current press a candidate for the long tap

    protected:
        // Creates the root and the widget with the built-in metrics and counters
        void Initialize();

        // Walks the scene once and fills mSceneContent
        void CountSceneContent();

        // Adds the component to the content counts by its kind
        void CountComponent(Component* component);

        // Adds the widget layers and their children to the content counts by their drawable kind
        void CountWidgetLayers(const Vector<Ref<WidgetLayer>>& layers);

        // Returns true when F12 was pressed or the top left corner was held long enough
        bool CheckToggleInput(float dt);

        // Places the root over the area and the widget in its top left corner, fitting the panel into it
        void LayoutIn(const RectF& area);

        // Resizes the root to the application content
        void OnApplicationSized() override;

        REF_COUNTERABLE_IMPL(RefCounterable);

        friend class Integration;
    };
}

#endif // O2_PROFILER_ENABLED
