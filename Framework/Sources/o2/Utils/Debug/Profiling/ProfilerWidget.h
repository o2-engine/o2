#pragma once

// @CODETOOLIGNORE: the profiler classes are not reflected, and the whole feature is compiled
// out by O2_PROFILER, which generated registrations would not follow
#if defined(O2_PROFILER_ENABLED)

#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Debug/Profiling/NanoProfiler.h"
#include "o2/Utils/Debug/Profiling/TimeSeries.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Color.h"

namespace o2
{
    class Mesh;
    class Text;

    // Performance grade of a single metric and of the whole widget
    enum class PerfStatus { Good, Normal, Bad };

    // ---------------------------------------------------------------------------------------------
    // Thresholds of a profiler metric. Either goodValue < badValue or badValue < goodValue, so both
    // "less is better" (frame time) and "more is better" (fps) metrics are described the same way
    // ---------------------------------------------------------------------------------------------
    struct PerfMetricSettings
    {
        double goodValue = 0.0; // Value at which the metric counts as good
        double badValue = 0.0;  // Value at which the metric counts as bad

        float goodWeight = 1.0f;   // Weight of a good metric in the overall status
        float normalWeight = 1.0f; // Weight of a normal metric in the overall status
        float badWeight = 1.0f;    // Weight of a bad metric in the overall status
    };

    // ------------------------------------------------------------------------------------------
    // Continuously sampled metric: keeps a time series of the sampled values and picks the graph
    // scale from the target values, so the graph stays readable when the value range changes
    // ------------------------------------------------------------------------------------------
    struct PerfMetric: public PerfMetricSettings
    {
        static constexpr int samplesCount = 130; // Time series length

        String             name;      // Displayed name
        Function<double()> getSample; // Value source, called every update interval

        Vector<double> targetValues; // Graph scale steps, ascending; the smallest one above the median wins

        float updateInterval = 0.0f; // Seconds between samples, zero samples every frame

        double baseline = 0.0; // Value captured when the baseline comparison was turned on

    public:
        PerfMetric() = default;
        PerfMetric(const String& name, const Function<double()>& getSample, const PerfMetricSettings& settings,
                   const Vector<double>& targetValues);

        // Samples the value when the update interval has passed and moves the graph scale to its target
        void Update(float dt);

        // Returns the status of the median sample
        PerfStatus GetStatus() const;

        // Returns the time series of the sampled values
        const TimeSeries<double>& GetSamples() const { return mSamples; }

        // Returns the current graph scale
        double GetTargetValue() const { return mTargetValue; }

        // Returns the last value formatted for display. Refreshed a few times per second, so that reading
        // the panel doesn't mean re-laying out its captions every frame
        const String& GetValueCaption() const { return mValueCaption; }

        // Returns the minimum and maximum of the time series formatted for display
        const String& GetRangeCaption() const { return mRangeCaption; }

    private:
        static constexpr float captionsInterval = 0.1f; // Seconds between caption refreshes

        TimeSeries<double> mSamples = TimeSeries<double>(samplesCount); // Sampled values

        float  mTimeSinceUpdate = 0.0f; // Seconds since the last sample
        double mTargetValue = 1.0;      // Graph scale, smoothly follows the picked target value

        String mValueCaption; // Last value, formatted
        String mRangeCaption; // Time series range, formatted

        float mTimeSinceCaptions = 0.0f; // Seconds since the captions were formatted

        bool mSampled = false; // Was the first sample taken; it fills the whole series so the graph starts flat
    };

    // -------------------------------------------------------------
    // Counted metric: an object count sampled at a coarse interval
    // -------------------------------------------------------------
    struct PerfCounter: public PerfMetricSettings
    {
        String          name;     // Displayed name
        Function<int()> getCount; // Count source

        int count = 0;    // Last sampled count
        int baseline = 0; // Count captured when the baseline comparison was turned on

    public:
        PerfCounter() = default;
        PerfCounter(const String& name, const Function<int()>& getCount, const PerfMetricSettings& settings);
    };

    // ------------------------------------------------------------------------------------------------------
    // On-screen performance monitor. Shows the NanoProfiler frame timeline, time series of registered metrics
    // and object counters. Hovering the timeline freezes it and details the hovered frame.
    //
    // All of the panel geometry goes into a single mesh built in place, so a frame of the widget costs one
    // draw call for the bars plus the caption texts, which are pooled and only re-laid out when they change
    // ------------------------------------------------------------------------------------------------------
    class ProfilerWidget: public Widget
    {
    public:
        static constexpr int historyFrames = 140;   // Timeline length in frames
        static constexpr int maxDetailSamples = 8;  // Scopes shown per frame, the rest are summed into "Other"

        static constexpr float maxSizeFactor = 4.0f; // How far past its design size the panel can be dragged

    public:
        Function<void()> onLayoutChanged;      // Called when a drag or a resize changed the place the panel needs
        Function<void()> onCountersUpdate;     // Called once before the counters are sampled, so a whole
                                               // group of them can be collected in a single pass

    public:
        // Default constructor, creates the drawing resources
        explicit ProfilerWidget(RefCounter* refCounter);

        // Destructor
        ~ProfilerWidget();

        // Samples the metrics, captures the profiler frame and tracks the cursor over the timeline
        void Update(float dt) override;

        // Draws the panel
        void Draw() override;

        // Registers a continuously sampled metric
        void AddMetric(const PerfMetric& metric);

        // Registers an object counter
        void AddCounter(const PerfCounter& counter);

        // Returns the weighted status of all registered metrics and counters
        PerfStatus GetOverallStatus() const;

        // Returns the registered counters with their last sampled values
        const Vector<PerfCounter>& GetCounters() const { return mCounters; }

        // Returns the frame the cursor details, -1 when the timeline is running
        int GetDetailedFrame() const { return mDetailedFrame; }

        // Returns the captured timeline frames count
        int GetHistoryCount() const { return mHistoryCount; }

        // Returns true when the values are shown as a difference from the captured baseline
        bool IsBaselineEnabled() const { return mBaselineEnabled; }

        // Returns true when the cursor is over the baseline button, which is then highlighted
        bool IsBaselineHovered() const { return mBaselineHovered; }

        // Returns true when the cursor is over the resize grip, which is then highlighted
        bool IsResizeGripHovered() const { return mGripHovered; }

        // Returns the world rect of the frame timeline, as of the last update or drawing
        const RectF& GetTimelineRect() const { return mTimelineRect; }

        // Returns the world rect of the baseline button, as of the last update or drawing
        const RectF& GetBaselineButtonRect() const { return mBaselineRect; }

        // Captures the current values as the baseline, or drops it
        void SetBaselineEnabled(bool enabled);

        // Returns the panel size: the dragged one when it was resized, the design one otherwise
        Vec2F GetContentSize() const;

        // Returns the size the panel looks best at: two pixels per timeline frame and a caption line
        // per shown scope
        Vec2F GetDesignSize() const;

        // Returns the smallest size the panel still reads at: a pixel per timeline frame and the
        // caption lines
        Vec2F GetMinContentSize() const;

        // Resizes the panel, clamping it between the minimal and the maximal size. Only the timeline
        // stretches: the caption rows keep their height, so the panel stays readable at any size
        void SetContentSize(const Vec2F& size);

        // Returns where the panel is dragged to, relative to the top left corner of its host
        const Vec2F& GetContentOffset() const { return mOffset; }

        // Moves the panel relative to the top left corner of its host. Whoever lays the panout out
        // clamps it into the host, so it can't be dragged out of reach
        void SetContentOffset(const Vec2F& offset);

        // Returns the status of the value against the thresholds
        static PerfStatus GetValueStatus(double value, const PerfMetricSettings& settings);

    protected:
        // ----------------------------------------------------------------
        // Drawing style of a profiler scope, resolved once per scope name
        // ----------------------------------------------------------------
        struct SampleStyle
        {
            const char* name = nullptr; // Scope name, identified by pointer
            Color4      color;          // Bar color, light enough to read on the plot background
            Color4      topColor;       // Bar color at its top edge, gives the band a soft gradient
            Color4      textColor;      // Caption color, the same hue darkened for the text to read
            String      caption;        // Scope name shortened for display
        };

        // -------------------------------------------------------------------------------------------
        // Pooled caption. The panel redraws every frame while most of its captions stay put, and every
        // Text setter rebuilds the glyph mesh, so the last applied state is kept and only the changed
        // properties are pushed through
        // -------------------------------------------------------------------------------------------
        struct Caption
        {
            Ref<Text> text;   // The drawable

            RectF  rect;      // Last applied rect
            Color4 color;     // Last applied color
            int    height = 0;// Last applied font height
        };

        // ------------------------------------------------
        // One profiler frame, reduced to the shown scopes
        // ------------------------------------------------
        struct TimelineFrame
        {
            struct Entry
            {
                int   style = 0;    // Index in mSampleStyles
                float time = 0.0f;  // Self time, milliseconds
            };

            Entry entries[maxDetailSamples + 1]; // Scopes, the last one may be the "Other" sum
            int   count = 0;                     // Filled entries
            float totalTime = 0.0f;              // Sum of the entries times
        };

    protected:
        Vector<PerfMetric>  mMetrics;  // Registered metrics
        Vector<PerfCounter> mCounters; // Registered counters

        PerfStatus mOverallStatus = PerfStatus::Good; // Weighted status of all metrics

        Vector<SampleStyle> mSampleStyles; // Colors and captions by scope name

        Vector<TimelineFrame> mHistory;         // Ring buffer of the timeline frames
        int                   mHistoryHead = 0; // Ring head: index of the oldest frame
        int                   mHistoryCount = 0;// Filled frames

        int mDetailedFrame = -1; // Timeline frame under the cursor, -1 when running

        float mCountersUpdateTimer = 0.0f; // Seconds since the counters were sampled

        bool mBaselineEnabled = false; // Show values as a difference from the baseline

        Vec2F mSize;   // Size dragged by the corner grip, zero until the panel was resized
        Vec2F mOffset; // Where the panel was dragged to by its caption bar

        Ref<Mesh> mMesh; // Panel geometry: background and all the bars

        Vector<Caption> mCaptions;      // Reused caption drawables
        int             mUsedCaptions = 0; // Captions taken from the pool this frame

        RectF mTimelineRect;   // World rect of the timeline graph, used for cursor hit testing
        RectF mHeaderRect;     // World rect of the caption bar, the panel is dragged by it
        RectF mBaselineRect;   // World rect of the baseline button
        RectF mResizeGripRect; // World rect of the resize grip in the bottom right corner

        float mColumnWidth = 1.0f; // Timeline pixels per frame, follows the panel width

        bool mBaselineHovered = false; // Is the cursor over the baseline button
        bool mGripHovered = false;     // Is the cursor over the resize grip

        bool  mResizing = false;   // Is the grip being dragged
        Vec2F mResizeStartSize;    // Panel size the current drag started with
        Vec2F mResizeStartCursor;  // Cursor position the current drag started at

        bool  mDragging = false;  // Is the panel being dragged by its caption bar
        Vec2F mDragStartOffset;   // Panel offset the current drag started with
        Vec2F mDragStartCursor;   // Cursor position the current drag started at

    protected:
        // Reduces the completed profiler frame to the shown scopes and pushes it into the timeline
        void CaptureTimelineFrame();

        // Returns the style index of the scope name, creating the style when the name is seen first
        int GetSampleStyle(const char* name);

        // Samples the counters when their interval has passed
        void UpdateCounters(float dt);

        // Recomputes the rects the panel's parts occupy from its current world rect. Called both before
        // the hit testing and before the drawing: a drag moves the panel after the update, and drawing
        // the parts where they were would leave them a frame behind
        void UpdateLayoutRects();

        // Tracks the cursor over the timeline, the baseline button and the zoom grip
        void UpdateInteraction();

        // Starts, follows and ends a resize grip drag
        void UpdateResizing(const Vec2F& cursor);

        // Starts, follows and ends a caption bar drag
        void UpdateDragging(const Vec2F& cursor);

        // Returns the height of everything but the timeline: the toolbar, the metric rows and the counters
        float GetFixedRowsHeight() const;

        // Returns the timeline frame by its position from the oldest one
        const TimelineFrame& GetHistoryFrame(int index) const;

        // Drawing

        // Appends a rectangle to the panel mesh
        void PushRect(const RectF& rect, const Color4& color);

        // Appends a vertically shaded rectangle to the panel mesh
        void PushRect(const RectF& rect, const Color4& topColor, const Color4& bottomColor);

        // Appends a quad to the panel mesh
        void PushQuad(const Vec2F& a, const Vec2F& b, const Vec2F& c, const Vec2F& d, const Color4& color);

        // Appends a quad with a color per corner to the panel mesh
        void PushQuad(const Vec2F& a, const Vec2F& b, const Vec2F& c, const Vec2F& d,
                      const Color4& colorA, const Color4& colorB, const Color4& colorC, const Color4& colorD);

        // Takes a caption from the pool and places it in a box of the given width; the captions are drawn
        // together after the panel geometry. Text that doesn't fit the box is cut with an ellipsis, so a
        // long scope name can't spill over the panel
        void PlaceCaption(const Vec2F& position, const WString& text, const Color4& color, float width,
                          bool rightAlign = false, bool small = false);

        // Draws the top bar: the baseline button
        void DrawToolbar(float& y);

        // Draws the profiler timeline with the details of the hovered frame
        void DrawTimeline(float& y);

        // Draws a metric caption and its graph
        void DrawMetric(PerfMetric& metric, float& y);

        // Draws the counters grid
        void DrawCounters(float& y);

        // Draws the resize grip in the bottom right corner
        void DrawResizeGrip();

    protected:
        // Returns the color of the status
        static const Color4& GetStatusColor(PerfStatus status);
    };
}

#endif // O2_PROFILER_ENABLED
