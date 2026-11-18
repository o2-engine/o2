#include "o2Editor/stdafx.h"
#include "SegmentedIcons.h"

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/UI/Style/EditorTheme.h"

namespace Editor
{
    namespace
    {
        const float iconAlphaOff = 0.72f; // Icons of the segments that are not chosen, on or hovered
        const float fadeSpeed = 12.0f;    // How fast the blocks fade in and out

        float Approach(float value, float target, float step)
        {
            return value < target ? Math::Min(target, value + step) : Math::Max(target, value - step);
        }
    }

    SegmentedIcons::SegmentedIcons(RefCounter* refCounter):
        Widget(refCounter)
    {
        InitializeDrawables();
    }

    SegmentedIcons::SegmentedIcons(RefCounter* refCounter, const SegmentedIcons& other):
        Widget(refCounter, other), mMode(other.mMode), mIconNames(other.mIconNames), mOn(other.mOn), mOnCoefs(other.mOnCoefs),
        mSelected(other.mSelected), mThumbPosition(other.mThumbPosition), mThumbCoef(other.mThumbCoef)
    {
        InitializeDrawables();
        InitializeIcons();
    }

    SegmentedIcons& SegmentedIcons::operator=(const SegmentedIcons& other)
    {
        Widget::operator=(other);
        mMode = other.mMode;
        mIconNames = other.mIconNames;
        mOn = other.mOn;
        mOnCoefs = other.mOnCoefs;
        mSelected = other.mSelected;
        mThumbPosition = other.mThumbPosition;
        mThumbCoef = other.mThumbCoef;
        InitializeIcons();
        return *this;
    }

    void SegmentedIcons::InitializeDrawables()
    {
        mTrack = mmake<PipelineRoundedRect>();
        mTrack->roundBottom = true;

        mBlock = mmake<PipelineRoundedRect>();
        mBlock->roundBottom = true;
    }

    void SegmentedIcons::InitializeIcons()
    {
        mIcons.Clear();
        for (auto& icon : mIconNames)
            mIcons.Add(mmake<VectorSprite>(icon));
    }

    void SegmentedIcons::SetMode(Mode mode)
    {
        mMode = mode;
    }

    SegmentedIcons::Mode SegmentedIcons::GetMode() const
    {
        return mMode;
    }

    void SegmentedIcons::SetSegments(const Vector<String>& icons)
    {
        mIconNames = icons;
        mOn.Clear();
        mOnCoefs.Clear();
        for (int i = 0; i < mIconNames.Count(); i++)
        {
            mOn.Add(0);
            mOnCoefs.Add(0.0f);
        }

        mSelected = -1;
        mHovered = -1;
        mPressed = -1;
        mThumbCoef = 0.0f;

        InitializeIcons();

        layout->minWidth = minSegmentWidth*(float)mIconNames.Count();
        layout->maxWidth = segmentWidth*(float)mIconNames.Count();
        layout->minHeight = trackHeight;
    }

    const Vector<String>& SegmentedIcons::GetSegments() const
    {
        return mIconNames;
    }

    void SegmentedIcons::SetSelected(int index, bool animate /*= true*/)
    {
        if (index >= mIconNames.Count())
            index = -1;

        bool wasNone = mSelected < 0;
        mSelected = index;

        // From nothing chosen the block appears in place instead of sliding from the last one
        if (index >= 0 && (!animate || wasNone || mThumbCoef < 0.01f))
            mThumbPosition = (float)index;

        if (!animate)
            mThumbCoef = index >= 0 ? 1.0f : 0.0f;
    }

    int SegmentedIcons::GetSelected() const
    {
        return mSelected;
    }

    void SegmentedIcons::SetOn(int index, bool on)
    {
        if (index >= 0 && index < mOn.Count())
            mOn[index] = on ? 1 : 0;
    }

    bool SegmentedIcons::IsOn(int index) const
    {
        return index >= 0 && index < mOn.Count() && mOn[index] != 0;
    }

    int SegmentedIcons::GetHoveredSegment() const
    {
        return mHovered;
    }

    RectF SegmentedIcons::GetTrackRect() const
    {
        RectF rect = layout->GetWorldRect();
        float width = Math::Min(rect.Width(), segmentWidth*(float)mIconNames.Count());
        float centerY = Math::Round(rect.Center().y);
        float left = Math::Round(rect.left);
        return RectF(left, centerY - trackHeight*0.5f, left + Math::Round(width), centerY + trackHeight*0.5f);
    }

    RectF SegmentedIcons::GetSegmentRect(int index) const
    {
        RectF track = GetTrackRect();
        if (mIconNames.IsEmpty())
            return track;

        float width = track.Width()/(float)mIconNames.Count();
        return RectF(track.left + width*(float)index, track.bottom, track.left + width*(float)(index + 1), track.top);
    }

    RectF SegmentedIcons::GetBlockRect(float position) const
    {
        RectF track = GetTrackRect();
        float width = mIconNames.IsEmpty() ? track.Width() : track.Width()/(float)mIconNames.Count();
        float left = track.left + width*position;
        return RectF(left + blockInset, track.bottom + blockInset, left + width - blockInset, track.top - blockInset);
    }

    RectF SegmentedIcons::GetThumbRect() const
    {
        return GetBlockRect(mThumbPosition);
    }

    int SegmentedIcons::GetSegmentAt(const Vec2F& point) const
    {
        RectF track = GetTrackRect();
        if (mIconNames.IsEmpty() || !track.IsInside(point))
            return -1;

        int index = Math::FloorToInt((point.x - track.left)/(track.Width()/(float)mIconNames.Count()));
        return Math::Clamp(index, 0, mIconNames.Count() - 1);
    }

    void SegmentedIcons::Update(float dt)
    {
        Widget::Update(dt);

        if (mSelected >= 0)
        {
            float target = (float)mSelected;
            float step = dt*slideSpeed*Math::Max(0.25f, Math::Abs(target - mThumbPosition));
            mThumbPosition = Approach(mThumbPosition, target, step);
        }

        mThumbCoef = Approach(mThumbCoef, mSelected >= 0 ? 1.0f : 0.0f, dt*fadeSpeed);

        for (int i = 0; i < mOnCoefs.Count(); i++)
            mOnCoefs[i] = Approach(mOnCoefs[i], mOn[i] != 0 ? 1.0f : 0.0f, dt*fadeSpeed);
    }

    void SegmentedIcons::DrawBlock(const RectF& rect, const Color4& color, float alpha)
    {
        if (alpha < 0.01f)
            return;

        mBlock->radius = blockRadius;
        mBlock->color = color;
        mBlock->SetRect(rect);
        mBlock->SetTransparency(mResTransparency*alpha);
        mBlock->Draw();
    }

    void SegmentedIcons::Draw()
    {
        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        auto& theme = EditorTheme::Get();

        mTrack->radius = trackRadius;
        mTrack->color = theme.segmentTrack;
        mTrack->SetRect(GetTrackRect());
        mTrack->SetTransparency(mResTransparency);
        mTrack->Draw();

        if (mHovered >= 0 && mHovered != mSelected && !IsOn(mHovered))
            DrawBlock(GetBlockRect((float)mHovered), theme.segmentHover, 1.0f);

        if (mMode == Mode::Flags)
        {
            for (int i = 0; i < mOnCoefs.Count(); i++)
                DrawBlock(GetBlockRect((float)i), theme.segmentThumb, mOnCoefs[i]);
        }
        else
            DrawBlock(GetThumbRect(), theme.segmentThumb, mThumbCoef);

        for (int i = 0; i < mIcons.Count(); i++)
        {
            RectF segment = GetSegmentRect(i);
            Vec2F center(Math::Round(segment.Center().x), Math::Round(segment.Center().y));

            bool lit = i == mHovered || (mMode == Mode::Choice && i == mSelected) || (mMode == Mode::Flags && IsOn(i));

            auto& icon = mIcons[i];
            icon->SetRect(RectF(center - Vec2F(iconSize, iconSize)*0.5f, center + Vec2F(iconSize, iconSize)*0.5f));
            icon->SetTransparency(mResTransparency*(lit ? 1.0f : iconAlphaOff));
            icon->Draw();
        }

        Widget::Draw();
        CursorAreaEventsListener::OnDrawn();
    }

    bool SegmentedIcons::IsUnderPoint(const Vec2F& point)
    {
        return mResEnabledInHierarchy && GetTrackRect().IsInside(point);
    }

    String SegmentedIcons::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void SegmentedIcons::Click(int segment)
    {
        if (mMode == Mode::Choice)
            SetSelected(segment);
        else
            SetOn(segment, !IsOn(segment));

        onClicked(segment);
    }

    void SegmentedIcons::OnCursorPressed(const Input::Cursor& cursor)
    {
        mPressed = GetSegmentAt(cursor.position);
    }

    void SegmentedIcons::OnCursorReleased(const Input::Cursor& cursor)
    {
        int segment = GetSegmentAt(cursor.position);
        bool clicked = segment >= 0 && segment == mPressed;
        mPressed = -1;

        if (clicked)
            Click(segment);
    }

    void SegmentedIcons::OnCursorDblClicked(const Input::Cursor& cursor)
    {
        int segment = GetSegmentAt(cursor.position);
        if (segment >= 0)
            Click(segment);
    }

    void SegmentedIcons::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        mPressed = -1;
    }

    void SegmentedIcons::OnCursorMoved(const Input::Cursor& cursor)
    {
        mHovered = GetSegmentAt(cursor.position);
    }

    void SegmentedIcons::OnCursorEnter(const Input::Cursor& cursor)
    {
        mHovered = GetSegmentAt(cursor.position);
    }

    void SegmentedIcons::OnCursorExit(const Input::Cursor& cursor)
    {
        mHovered = -1;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::SegmentedIcons>);
// --- META ---

ENUM_META(Editor::SegmentedIcons::Mode, Editor__SegmentedIcons__Mode)
{
    ENUM_ENTRY(Choice);
    ENUM_ENTRY(Flags);
}
END_ENUM_META;

DECLARE_CLASS(Editor::SegmentedIcons, Editor__SegmentedIcons);
// --- END META ---
