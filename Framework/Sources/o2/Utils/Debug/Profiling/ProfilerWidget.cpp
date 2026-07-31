// @CODETOOLIGNORE: the profiler classes are not reflected, and the whole feature is compiled
// out by O2_PROFILER, which generated registrations would not follow
#include "o2/stdafx.h"
#include "ProfilerWidget.h"

#if defined(O2_PROFILER_ENABLED)

#include "o2/Application/Input.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorFont.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    namespace
    {
        constexpr float kPadding = 6.0f;
        constexpr float kLineHeight = 13.0f;
        constexpr float kHeaderHeight = 21.0f; // The caption bar is a title, not a text line
        constexpr int   kCaptionHeight = 11;
        constexpr int   kSmallCaptionHeight = 9;

        constexpr float kTimelineDesignColumnWidth = 2.0f; // Pixels per frame at the design width
        constexpr float kTimelineMinColumnWidth = 1.0f;
        // Tall enough for a caption per shown scope: the details of the hovered frame are laid out
        // alongside the graph, one line per band
        constexpr float kTimelineMinHeight = (ProfilerWidget::maxDetailSamples + 1)*kLineHeight;
        constexpr float kTimelineLeadGap = 20.0f;
        constexpr float kTimelineLabelsWidth = 200.0f;
        constexpr float kTimelineMinScaleMs = 6.0f;
        constexpr float kTimelineMaxScaleMs = 50.0f;

        constexpr float kMetricGraphHeight = 10.0f;
        constexpr float kBaselineButtonWidth = 64.0f;
        constexpr float kResizeGripSize = 12.0f;

        constexpr int kCounterColumns = 3;

        // The editor's palette: a teal caption bar over a blue gray panel, blue gray text, and the
        // material accents the editor uses for its own highlights
        const Color4 kPanelColor = Color4(236, 239, 241, 240);
        const Color4 kCaptionBarColor = Color4(0, 150, 136, 255);
        const Color4 kPlotBackColor = Color4(255, 255, 255, 150);
        const Color4 kTextColor = Color4(69, 90, 100, 255);
        const Color4 kDimTextColor = Color4(96, 125, 139, 190);
        const Color4 kCaptionTextColor = Color4(235, 255, 253, 255);
        const Color4 kButtonColor = Color4(0, 121, 107, 255);
        const Color4 kGoodColor = Color4(56, 142, 60, 255);
        const Color4 kNormalColor = Color4(239, 154, 25, 255);
        const Color4 kBadColor = Color4(211, 47, 47, 255);

        Color4 Lighten(const Color4& color, int amount = 45)
        {
            return Color4(Math::Min(color.r + amount, 255), Math::Min(color.g + amount, 255),
                          Math::Min(color.b + amount, 255), color.a);
        }

        // Distinct hue per profiled scope, same corner stepping as Color4::SomeColor. The bars want a
        // light tone to read against the white plot, the captions a dark one to read as text
        Color4 ScopeColor(int index, float saturation, float lightness)
        {
            constexpr float kCornersOnCircle = 3.0f;

            const float corner = Math::Mod((float)index, kCornersOnCircle)/kCornersOnCircle;
            const float cycle = Math::Floor((float)index/kCornersOnCircle);

            Color4 color;
            color.SetHSL(Math::Mod(0.08f + corner + cycle*0.13f, 1.0f), saturation, lightness);

            return color;
        }

        // Trims a __PRETTY_FUNCTION__ or a scope literal down to what fits the caption column
        String ShortenScopeName(const char* name)
        {
            String full(name);

            if (full.Length() > 0 && full[0] == '_')
                full = full.SubStr(1);

            int paren = full.Find('(');
            if (paren < 0)
                return full;

            full = full.SubStr(0, paren);

            int space = full.FindLast(" ");
            if (space >= 0)
                full = full.SubStr(space + 1);

            // Keep the last two qualifiers: "void o2::Widget::Draw()" reads better as "Widget::Draw"
            int last = full.FindLast("::");
            if (last > 0)
            {
                int prev = full.SubStr(0, last).FindLast("::");
                if (prev >= 0)
                    full = full.SubStr(prev + 2);
            }

            return full;
        }

        String FormatValue(const char* format, double value)
        {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), format, value);
            return String(buffer);
        }
    }

    PerfMetric::PerfMetric(const String& name, const Function<double()>& getSample,
                           const PerfMetricSettings& settings, const Vector<double>& targetValues):
        PerfMetricSettings(settings), name(name), getSample(getSample), targetValues(targetValues)
    {
        if (this->targetValues.IsEmpty())
            this->targetValues.Add(1.0);

        mTargetValue = this->targetValues[0];
    }

    void PerfMetric::Update(float dt)
    {
        const double median = mSamples.Median();

        double newTarget = targetValues.Last();
        for (auto& value : targetValues)
        {
            if (value > median)
            {
                newTarget = value;
                break;
            }
        }

        mTargetValue = Math::Lerp(mTargetValue, newTarget, (double)Math::Min(dt*5.0f, 1.0f));

        mTimeSinceUpdate += dt;
        if (mTimeSinceUpdate >= updateInterval)
        {
            mTimeSinceUpdate = 0.0f;

            if (getSample)
            {
                const double sample = getSample();

                if (mSampled)
                    mSamples.Push(sample);
                else
                {
                    mSamples.Fill(sample);
                    mSampled = true;
                }
            }
        }

        mTimeSinceCaptions += dt;
        if (mTimeSinceCaptions >= captionsInterval || mValueCaption.IsEmpty())
        {
            mTimeSinceCaptions = 0.0f;

            mValueCaption = FormatValue("%.1f", mSamples.GetLastSample());
            mRangeCaption = FormatValue("low:%.0f", mSamples.Min()) + FormatValue(" max:%.0f", mSamples.Max());
        }
    }

    PerfStatus PerfMetric::GetStatus() const
    {
        return ProfilerWidget::GetValueStatus(mSamples.Median(), *this);
    }

    PerfCounter::PerfCounter(const String& name, const Function<int()>& getCount, const PerfMetricSettings& settings):
        PerfMetricSettings(settings), name(name), getCount(getCount)
    {}

    ProfilerWidget::ProfilerWidget(RefCounter* refCounter):
        Widget(refCounter, ActorCreateMode::NotInScene)
    {
        mHistory.Resize(historyFrames);

        mMesh = mmake<Mesh>();
        mMesh->Resize(4, 2);
    }

    ProfilerWidget::~ProfilerWidget()
    {}

    void ProfilerWidget::AddMetric(const PerfMetric& metric)
    {
        mMetrics.Add(metric);
    }

    void ProfilerWidget::AddCounter(const PerfCounter& counter)
    {
        mCounters.Add(counter);

        if (counter.getCount)
            mCounters.Last().count = counter.getCount();
    }

    float ProfilerWidget::GetFixedRowsHeight() const
    {
        float height = kPadding*2.0f + kHeaderHeight + 4.0f;
        height += mMetrics.Count()*(kLineHeight + kMetricGraphHeight + 3.0f);

        if (!mCounters.IsEmpty())
            height += kLineHeight + ((mCounters.Count() + kCounterColumns - 1)/kCounterColumns)*kLineHeight;

        return height;
    }

    Vec2F ProfilerWidget::GetDesignSize() const
    {
        return Vec2F(kPadding*2.0f + historyFrames*kTimelineDesignColumnWidth + kTimelineLeadGap +
                     kTimelineLabelsWidth,
                     GetFixedRowsHeight() + kTimelineMinHeight);
    }

    Vec2F ProfilerWidget::GetMinContentSize() const
    {
        return Vec2F(kPadding*2.0f + historyFrames*kTimelineMinColumnWidth + kTimelineLeadGap +
                     kTimelineLabelsWidth,
                     GetFixedRowsHeight() + kTimelineMinHeight);
    }

    Vec2F ProfilerWidget::GetContentSize() const
    {
        const Vec2F design = GetDesignSize();
        if (mSize == Vec2F())
            return design;

        const Vec2F min = GetMinContentSize();

        return Vec2F(Math::Clamp(mSize.x, min.x, design.x*maxSizeFactor),
                     Math::Clamp(mSize.y, min.y, design.y*maxSizeFactor));
    }

    void ProfilerWidget::SetContentSize(const Vec2F& size)
    {
        const Vec2F before = GetContentSize();

        mSize = size;

        if (GetContentSize() == before)
            return;

        onLayoutChanged();
    }

    void ProfilerWidget::SetContentOffset(const Vec2F& offset)
    {
        if (mOffset == offset)
            return;

        mOffset = offset;
        onLayoutChanged();
    }

    void ProfilerWidget::SetBaselineEnabled(bool enabled)
    {
        mBaselineEnabled = enabled;

        if (!mBaselineEnabled)
            return;

        for (auto& metric : mMetrics)
            metric.baseline = metric.GetSamples().GetLastSample();

        for (auto& counter : mCounters)
            counter.baseline = counter.count;
    }

    void ProfilerWidget::Update(float dt)
    {
        NanoProfiler::ExcludeScope exclude;

        Widget::Update(dt);

        UpdateInteraction();

        if (mDetailedFrame < 0)
            CaptureTimelineFrame();

        for (auto& metric : mMetrics)
            metric.Update(dt);

        UpdateCounters(dt);

        mOverallStatus = GetOverallStatus();
    }

    void ProfilerWidget::UpdateLayoutRects()
    {
        const RectF rect = layout->GetWorldRect();

        const float toolbarTop = rect.top - kPadding;
        const float timelineTop = toolbarTop - kHeaderHeight;

        // Only the timeline takes the panel's spare height; the caption rows keep theirs
        const float timelineHeight = Math::Max(rect.Height() - GetFixedRowsHeight(), kTimelineMinHeight);

        mTimelineRect = RectF(rect.left + kPadding, timelineTop - timelineHeight,
                              rect.right - kPadding - kTimelineLabelsWidth - kTimelineLeadGap, timelineTop);

        mColumnWidth = Math::Max(mTimelineRect.Width()/historyFrames, kTimelineMinColumnWidth);

        mHeaderRect = RectF(rect.left, rect.top - kHeaderHeight, rect.right, rect.top);

        // Part of the caption bar: the whole bar height, flush with the panel edge
        mBaselineRect = RectF(rect.right - kBaselineButtonWidth, rect.top - kHeaderHeight, rect.right, rect.top);

        // The panel hangs on its top left corner, so it grows to the right and down from this grip
        mResizeGripRect = RectF(rect.right - kResizeGripSize, rect.bottom, rect.right,
                                rect.bottom + kResizeGripSize);
    }

    void ProfilerWidget::UpdateInteraction()
    {
        UpdateLayoutRects();

        const Vec2F cursor = o2Input.GetCursorPos();

        UpdateResizing(cursor);
        UpdateDragging(cursor);

        mBaselineHovered = !mResizing && !mDragging && mBaselineRect.IsInside(cursor);
        mGripHovered = mResizing || mResizeGripRect.IsInside(cursor);

        mDetailedFrame = -1;
        if (mHistoryCount > 0 && !mResizing && !mDragging && mTimelineRect.IsInside(cursor))
        {
            const int index = (int)((cursor.x - mTimelineRect.left)/mColumnWidth);
            mDetailedFrame = Math::Clamp(index, historyFrames - mHistoryCount, historyFrames - 1);
        }

        if (o2Input.IsCursorPressed() && !mDragging && mBaselineHovered)
            SetBaselineEnabled(!mBaselineEnabled);
    }

    void ProfilerWidget::UpdateResizing(const Vec2F& cursor)
    {
        if (!mResizing)
        {
            if (!o2Input.IsCursorPressed() || !mResizeGripRect.IsInside(cursor))
                return;

            mResizing = true;
            mResizeStartSize = GetContentSize();
            mResizeStartCursor = cursor;

            return;
        }

        if (!o2Input.IsCursorDown())
        {
            mResizing = false;
            return;
        }

        // The corner follows the cursor in both axes; the panel hangs on its top left corner, so moving
        // down and to the right grows it
        SetContentSize(mResizeStartSize + Vec2F(cursor.x - mResizeStartCursor.x,
                                                mResizeStartCursor.y - cursor.y));
    }

    void ProfilerWidget::UpdateDragging(const Vec2F& cursor)
    {
        if (!mDragging)
        {
            // The baseline button lives in the caption bar too, and it is not a handle
            if (mResizing || !o2Input.IsCursorPressed() || !mHeaderRect.IsInside(cursor) ||
                mBaselineRect.IsInside(cursor))
            {
                return;
            }

            mDragging = true;
            mDragStartOffset = mOffset;
            mDragStartCursor = cursor;

            return;
        }

        if (!o2Input.IsCursorDown())
        {
            mDragging = false;
            return;
        }

        SetContentOffset(mDragStartOffset + cursor - mDragStartCursor);
    }

    void ProfilerWidget::UpdateCounters(float dt)
    {
        constexpr float kCountersUpdateInterval = 0.3f;

        mCountersUpdateTimer += dt;
        if (mCountersUpdateTimer < kCountersUpdateInterval)
            return;

        mCountersUpdateTimer = 0.0f;

        onCountersUpdate();

        for (auto& counter : mCounters)
        {
            if (counter.getCount)
                counter.count = counter.getCount();
        }
    }

    void ProfilerWidget::CaptureTimelineFrame()
    {
        static NanoProfiler::AggregatedSample aggregated[NanoProfiler::maxAggregatedSamples];
        int count = NanoProfiler::AggregateFrame(aggregated, NanoProfiler::maxAggregatedSamples);

        constexpr Int64 kNegligibleNs = 10000; // 0.01 ms

        int kept = 0;
        for (int i = 0; i < count; i++)
        {
            if (aggregated[i].time >= kNegligibleNs)
                aggregated[kept++] = aggregated[i];
        }
        count = kept;

        // Scopes named with a leading underscore are pinned: they keep their slot whatever their time is
        int pinned = 0;
        for (int i = 0; i < count; i++)
        {
            if (aggregated[i].name[0] == '_')
                std::swap(aggregated[pinned++], aggregated[i]);
        }
        pinned = Math::Min(pinned, maxDetailSamples);

        std::sort(aggregated + pinned, aggregated + count,
                  [](const NanoProfiler::AggregatedSample& a, const NanoProfiler::AggregatedSample& b)
                  { return a.time > b.time; });

        const int shown = Math::Min(count, maxDetailSamples);

        Int64 otherTime = 0;
        for (int i = shown; i < count; i++)
            otherTime += aggregated[i].time;

        TimelineFrame frame;
        for (int i = 0; i < shown; i++)
        {
            frame.entries[frame.count].style = GetSampleStyle(aggregated[i].name);
            frame.entries[frame.count].time = (float)((double)aggregated[i].time*1e-6);
            frame.count++;
        }

        if (otherTime > 0)
        {
            frame.entries[frame.count].style = GetSampleStyle(NanoProfiler::otherSampleName);
            frame.entries[frame.count].time = (float)((double)otherTime*1e-6);
            frame.count++;
        }

        // Stacking follows the order the scopes were first seen, so the bands don't jump between frames
        std::sort(frame.entries, frame.entries + frame.count,
                  [](const TimelineFrame::Entry& a, const TimelineFrame::Entry& b) { return a.style < b.style; });

        for (int i = 0; i < frame.count; i++)
            frame.totalTime += frame.entries[i].time;

        mHistory[mHistoryHead] = frame;
        mHistoryHead = (mHistoryHead + 1)%historyFrames;
        mHistoryCount = Math::Min(mHistoryCount + 1, historyFrames);
    }

    int ProfilerWidget::GetSampleStyle(const char* name)
    {
        for (int i = 0; i < mSampleStyles.Count(); i++)
        {
            if (mSampleStyles[i].name == name)
                return i;
        }

        SampleStyle style;
        style.name = name;
        style.caption = ShortenScopeName(name);

        const int hueIndex = mSampleStyles.Count();
        style.color = ScopeColor(hueIndex, 0.58f, 0.60f);
        style.topColor = ScopeColor(hueIndex, 0.62f, 0.74f);
        style.textColor = ScopeColor(hueIndex, 0.55f, 0.42f);

        mSampleStyles.Add(style);

        return mSampleStyles.Count() - 1;
    }

    const ProfilerWidget::TimelineFrame& ProfilerWidget::GetHistoryFrame(int index) const
    {
        return mHistory[(mHistoryHead + index)%historyFrames];
    }

    PerfStatus ProfilerWidget::GetValueStatus(double value, const PerfMetricSettings& settings)
    {
        if (settings.badValue > settings.goodValue)
        {
            if (value >= settings.badValue)
                return PerfStatus::Bad;

            return value <= settings.goodValue ? PerfStatus::Good : PerfStatus::Normal;
        }

        if (value <= settings.badValue)
            return PerfStatus::Bad;

        return value >= settings.goodValue ? PerfStatus::Good : PerfStatus::Normal;
    }

    const Color4& ProfilerWidget::GetStatusColor(PerfStatus status)
    {
        switch (status)
        {
            case PerfStatus::Good: return kGoodColor;
            case PerfStatus::Normal: return kNormalColor;
            default: return kBadColor;
        }
    }

    PerfStatus ProfilerWidget::GetOverallStatus() const
    {
        float points = 0.0f;
        float weights = 0.0f;

        auto add = [&](PerfStatus status, const PerfMetricSettings& settings)
        {
            const float weight = status == PerfStatus::Bad ? settings.badWeight :
                status == PerfStatus::Normal ? settings.normalWeight : settings.goodWeight;

            points += (float)status*weight;
            weights += weight;
        };

        for (auto& metric : mMetrics)
            add(metric.GetStatus(), metric);

        for (auto& counter : mCounters)
            add(GetValueStatus(counter.count, counter), counter);

        if (weights < FLT_EPSILON)
            return PerfStatus::Good;

        const float overall = points/weights;
        if (overall < 0.33f)
            return PerfStatus::Good;

        return overall < 0.66f ? PerfStatus::Normal : PerfStatus::Bad;
    }

    void ProfilerWidget::PushRect(const RectF& rect, const Color4& color)
    {
        PushQuad(rect.LeftTop(), rect.RightTop(), rect.RightBottom(), rect.LeftBottom(), color);
    }

    void ProfilerWidget::PushRect(const RectF& rect, const Color4& topColor, const Color4& bottomColor)
    {
        PushQuad(rect.LeftTop(), rect.RightTop(), rect.RightBottom(), rect.LeftBottom(),
                 topColor, topColor, bottomColor, bottomColor);
    }

    void ProfilerWidget::PushQuad(const Vec2F& a, const Vec2F& b, const Vec2F& c, const Vec2F& d, const Color4& color)
    {
        PushQuad(a, b, c, d, color, color, color, color);
    }

    void ProfilerWidget::PushQuad(const Vec2F& a, const Vec2F& b, const Vec2F& c, const Vec2F& d,
                                  const Color4& colorA, const Color4& colorB, const Color4& colorC,
                                  const Color4& colorD)
    {
        if (mMesh->vertexCount + 4 > mMesh->GetMaxVertexCount() || mMesh->polyCount + 2 > mMesh->GetMaxPolyCount())
            return;

        Vertex* vertices = mMesh->GetVertices<Vertex>() + mMesh->vertexCount;
        VertexIndex* indexes = mMesh->GetIndexes() + mMesh->polyCount*3;

        const Vec2F corners[] = { a, b, c, d };
        const Color32Bit packedColors[] = { colorA.ABGR(), colorB.ABGR(), colorC.ABGR(), colorD.ABGR() };
        for (int i = 0; i < 4; i++)
        {
            vertices[i].x = corners[i].x;
            vertices[i].y = corners[i].y;
            vertices[i].z = 0.0f;
            vertices[i].color = packedColors[i];
            vertices[i].tu = 0.0f;
            vertices[i].tv = 0.0f;
        }

        indexes[0] = mMesh->vertexCount;
        indexes[1] = mMesh->vertexCount + 1;
        indexes[2] = mMesh->vertexCount + 2;
        indexes[3] = mMesh->vertexCount;
        indexes[4] = mMesh->vertexCount + 2;
        indexes[5] = mMesh->vertexCount + 3;

        mMesh->vertexCount += 4;
        mMesh->polyCount += 2;
    }

    void ProfilerWidget::PlaceCaption(const Vec2F& position, const WString& text, const Color4& color, float width,
                                      bool rightAlign /*= false*/, bool small /*= false*/)
    {
        if (mUsedCaptions >= mCaptions.Count())
        {
            // Plain glyphs: the debug font style strokes them black, which is meant for light text over
            // the game, not for the panel's own light background
            Caption caption;
            caption.text = mmake<Text>(o2Debug.GetFont());
            caption.text->verAlign = VerAlign::Top;
            caption.text->SetDotsEngings(true);

            mCaptions.Add(caption);
        }

        Caption& caption = mCaptions[mUsedCaptions++];

        const int height = small ? kSmallCaptionHeight : kCaptionHeight;
        if (caption.height != height)
        {
            caption.text->SetHeight(height);
            caption.height = height;
        }

        const HorAlign align = rightAlign ? HorAlign::Right : HorAlign::Left;
        if (caption.text->GetHorAlign() != align)
            caption.text->SetHorAlign(align);

        if (caption.color != color)
        {
            caption.text->SetColor(color);
            caption.color = color;
        }

        const RectF rect(rightAlign ? position.x - width : position.x, position.y - kLineHeight,
                         rightAlign ? position.x : position.x + width, position.y);
        if (caption.rect != rect)
        {
            caption.text->SetRect(rect);
            caption.rect = rect;
        }

        caption.text->SetText(text);
    }

    void ProfilerWidget::Draw()
    {
        if (!mResEnabledInHierarchy)
            return;

        NanoProfiler::ExcludeScope exclude;

        // Panel geometry is rebuilt per frame into a mesh sized for the worst case, so the bars cost one draw call
        const UInt maxQuads = (UInt)(4 + historyFrames*(maxDetailSamples + 1) + (maxDetailSamples + 1)*3 +
                                     mMetrics.Count()*(PerfMetric::samplesCount + 1) + 16);
        if (mMesh->GetMaxPolyCount() < maxQuads*2)
            mMesh->Resize(maxQuads*4, maxQuads*2);

        mMesh->vertexCount = 0;
        mMesh->polyCount = 0;
        mUsedCaptions = 0;

        // A drag moves the panel after the update, so the parts are placed against where it is now
        UpdateLayoutRects();

        PushRect(layout->GetWorldRect(), kPanelColor);

        float y = layout->GetWorldRect().top - kPadding;
        DrawToolbar(y);
        DrawTimeline(y);

        for (auto& metric : mMetrics)
            DrawMetric(metric, y);

        DrawCounters(y);
        DrawResizeGrip();

        o2Render.DrawMesh(mMesh.Get());

        for (int i = 0; i < mUsedCaptions; i++)
            mCaptions[i].text->Draw();
    }

    void ProfilerWidget::DrawToolbar(float& y)
    {
        const RectF rect = layout->GetWorldRect();

        PushRect(RectF(rect.left, rect.top - kHeaderHeight, rect.right, rect.top), Lighten(kCaptionBarColor, 12),
                 kCaptionBarColor);

        Color4 buttonColor = mBaselineEnabled ? GetStatusColor(PerfStatus::Normal) : kButtonColor;
        if (mBaselineHovered)
            buttonColor = Lighten(buttonColor);

        PushRect(mBaselineRect, Lighten(buttonColor, 12), buttonColor);

        // The captions are a text line tall, so they are centred in the taller bar
        const float captionY = rect.top - (kHeaderHeight - kLineHeight)*0.5f;
        PlaceCaption(Vec2F(rect.left + kPadding, captionY), "Profiler", kCaptionTextColor, 120.0f);
        PlaceCaption(Vec2F(mBaselineRect.left + 12.0f, captionY), "Base", kCaptionTextColor,
                     kBaselineButtonWidth - 20.0f);

        y -= kHeaderHeight;
    }

    void ProfilerWidget::DrawResizeGrip()
    {
        const Color4 color = mGripHovered ? kTextColor : kDimTextColor;
        const float step = kResizeGripSize/3.0f;

        // Three diagonal notches, the usual corner grip
        for (int i = 1; i <= 3; i++)
        {
            PushQuad(Vec2F(mResizeGripRect.right - i*step, mResizeGripRect.bottom),
                     Vec2F(mResizeGripRect.right - i*step + 1.5f, mResizeGripRect.bottom),
                     Vec2F(mResizeGripRect.right, mResizeGripRect.bottom + i*step),
                     Vec2F(mResizeGripRect.right, mResizeGripRect.bottom + i*step - 1.5f), color);
        }
    }

    void ProfilerWidget::DrawTimeline(float& y)
    {
        // The scale follows the slowest frame in the history, so a spike doesn't clip and a calm run isn't flat
        float scaleMs = kTimelineMinScaleMs;
        for (int i = 0; i < mHistoryCount; i++)
            scaleMs = Math::Clamp(GetHistoryFrame(historyFrames - mHistoryCount + i).totalTime, scaleMs, kTimelineMaxScaleMs);

        const float timelineHeight = mTimelineRect.Height();
        const float columnWidth = mColumnWidth;
        const float lineHeight = kLineHeight;

        const float labelsLeft = mTimelineRect.right + kTimelineLeadGap;
        const float leadWidth = 4.0f;

        // The plot backing covers the graph and the leaders growing out of it, and stops where they end:
        // the captions belong to the panel, not to the plot
        PushRect(RectF(mTimelineRect.left, mTimelineRect.bottom, labelsLeft - leadWidth, mTimelineRect.top),
                 kPlotBackColor);

        const float pixelsPerMs = timelineHeight/scaleMs;
        const float bottom = mTimelineRect.bottom;

        const int lastFrame = historyFrames - 1;
        const int detailed = mDetailedFrame >= 0 ? mDetailedFrame : lastFrame;

        float detailY[maxDetailSamples + 1];
        float detailHeight[maxDetailSamples + 1];
        int detailStyles[maxDetailSamples + 1];
        int detailCount = 0;
        float detailX = 0.0f;

        for (int i = historyFrames - mHistoryCount; i < historyFrames; i++)
        {
            // Frozen: the frames past the cursor are the future of the frame being read, drawing them
            // only distracts from it
            if (mDetailedFrame >= 0 && i > mDetailedFrame)
                break;

            const TimelineFrame& frame = GetHistoryFrame(i);
            const float x = mTimelineRect.left + i*columnWidth;
            const bool isDetailed = i == detailed;

            float stack = 0.0f;
            for (int j = 0; j < frame.count; j++)
            {
                if (stack >= timelineHeight)
                    break;

                const float height = Math::Min(frame.entries[j].time*pixelsPerMs, timelineHeight - stack);

                const float top = Math::Round(bottom + stack + height);
                const float bot = Math::Round(bottom + stack);

                stack += height;

                if (isDetailed && detailCount < maxDetailSamples + 1)
                {
                    detailY[detailCount] = bot;
                    detailHeight[detailCount] = top - bot;
                    detailStyles[detailCount] = frame.entries[j].style;
                    detailCount++;
                }

                // Most scopes of a frame are far below a pixel tall; emitting them would multiply the
                // panel geometry and change nothing on screen
                if (top <= bot)
                    continue;

                const SampleStyle& style = mSampleStyles[frame.entries[j].style];
                PushRect(RectF(x, bot, x + columnWidth, top), style.topColor, style.color);
            }

            if (isDetailed)
                detailX = x + columnWidth;
        }

        // Hovered frame details: a leader from every band of the column to its caption on the right.
        // The bands stack upwards from the graph bottom, so the captions are stacked the same way and
        // sit at the bottom too: the leaders stay short and don't cross however tall the panel is
        const float labelsWidth = layout->GetWorldRect().right - kPadding - labelsLeft;

        const TimelineFrame& detailedFrame = GetHistoryFrame(detailed);
        for (int i = 0; i < detailCount; i++)
        {
            const SampleStyle& style = mSampleStyles[detailStyles[i]];
            const float labelY = mTimelineRect.bottom + (i + 1)*lineHeight;

            PushQuad(Vec2F(detailX, detailY[i] + detailHeight[i]),
                     Vec2F(detailX + leadWidth, detailY[i] + detailHeight[i]),
                     Vec2F(detailX + leadWidth, detailY[i]), Vec2F(detailX, detailY[i]),
                     style.topColor, style.topColor, style.color, style.color);

            // The leader keeps the band's own shading along its whole length: fading it into the darker
            // caption tone made it look like it dims towards the text
            PushQuad(Vec2F(detailX + leadWidth, detailY[i] + detailHeight[i]),
                     Vec2F(labelsLeft - leadWidth, labelY),
                     Vec2F(labelsLeft - leadWidth, labelY - lineHeight), Vec2F(detailX + leadWidth, detailY[i]),
                     style.topColor, style.topColor, style.color, style.color);

            PlaceCaption(Vec2F(labelsLeft, labelY),
                         FormatValue("[%.2fms] ", detailedFrame.entries[i].time) + style.caption,
                         style.textColor, labelsWidth, false, true);
        }

        y -= timelineHeight + 4.0f;
    }

    void ProfilerWidget::DrawMetric(PerfMetric& metric, float& y)
    {
        const RectF rect = layout->GetWorldRect();
        const float left = rect.left + kPadding;
        const float right = rect.right - kPadding;
        const float graphHeight = kMetricGraphHeight;

        const TimeSeries<double>& samples = metric.GetSamples();
        const double value = samples.GetLastSample();

        // The caption grades the value it shows; the median grade drives the overall status instead
        const PerfStatus status = GetValueStatus(value, metric);

        PlaceCaption(Vec2F(left, y), metric.name + ":", kTextColor, 80.0f);

        String valueText = metric.GetValueCaption();
        if (mBaselineEnabled)
        {
            const double delta = value - metric.baseline;
            valueText += FormatValue(delta > 0.0 ? " (+%.1f)" : " (%.1f)", delta);
        }

        PlaceCaption(Vec2F(left + 84.0f, y), valueText, GetStatusColor(status), 140.0f);
        PlaceCaption(Vec2F(right, y), metric.GetRangeCaption(), kDimTextColor, 160.0f, true, true);

        y -= kLineHeight;

        const float graphWidth = right - left;
        const float step = graphWidth/PerfMetric::samplesCount;
        const float barWidth = Math::Max(step - 1.0f, 1.0f);
        const float scale = metric.GetTargetValue() > DBL_EPSILON ?
            (float)(graphHeight/metric.GetTargetValue()) : 0.0f;

        PushRect(RectF(left, y - graphHeight, right, y), kPlotBackColor);

        const double* values = samples.GetSamples();
        const int count = samples.GetSamplesCount();
        const int head = samples.GetHead();

        for (int i = 0; i < count; i++)
        {
            const double sample = values[(i + head)%count];
            const float height = Math::Round(Math::Min((float)sample*scale, graphHeight));
            if (height < 1.0f)
                continue;

            const float x = left + i*step;
            const Color4& barColor = GetStatusColor(GetValueStatus(sample, metric));
            PushRect(RectF(x, y - graphHeight, x + barWidth, y - graphHeight + height),
                     Lighten(barColor, 40), barColor);
        }

        y -= graphHeight + 3.0f;
    }

    void ProfilerWidget::DrawCounters(float& y)
    {
        if (mCounters.IsEmpty())
            return;

        const RectF rect = layout->GetWorldRect();
        const float left = rect.left + kPadding;
        const float lineHeight = kLineHeight;
        const float columnWidth = (rect.Width() - kPadding*2.0f)/kCounterColumns;

        PlaceCaption(Vec2F(left, y), "Entities:", kTextColor, 80.0f);
        y -= lineHeight;

        for (int i = 0; i < mCounters.Count(); i++)
        {
            PerfCounter& counter = mCounters[i];

            const float x = left + (i%kCounterColumns)*columnWidth;
            const float rowY = y - (i/kCounterColumns)*lineHeight;

            PlaceCaption(Vec2F(x, rowY), counter.name, kDimTextColor, columnWidth*0.55f, false, true);

            String countText = (String)counter.count;
            if (mBaselineEnabled)
            {
                const int delta = counter.count - counter.baseline;
                countText += delta > 0 ? " (+" + (String)delta + ")" : " (" + (String)delta + ")";
            }

            // A counter with no weights doesn't grade anything, it just reports; coloring it would read
            // as a warning
            const bool graded = counter.goodWeight > 0.0f || counter.normalWeight > 0.0f ||
                counter.badWeight > 0.0f;

            PlaceCaption(Vec2F(x + columnWidth*0.55f, rowY), countText,
                         graded ? GetStatusColor(GetValueStatus(counter.count, counter)) : kTextColor,
                         columnWidth*0.45f, false, true);
        }

        y -= ((mCounters.Count() + kCounterColumns - 1)/kCounterColumns)*lineHeight;
    }
}

#endif // O2_PROFILER_ENABLED
