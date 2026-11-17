#include "o2Editor/stdafx.h"
#include "ViewModeSwitch.h"

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/UI/Style/EditorTheme.h"

namespace Editor
{
    namespace
    {
        const float trackHeight = 16.0f;
        const float thumbInset = 2.0f;
        const float sideMargin = 4.0f;

        Color4 Mix(const Color4& a, const Color4& b, float coef)
        {
            auto mix = [&](int x, int y) { return (int)Math::Round((float)x + (float)(y - x)*coef); };
            return Color4(mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a));
        }
    }

    ViewModeSwitch::ViewModeSwitch(RefCounter* refCounter):
        Widget(refCounter)
    {
        InitializeDrawables();
    }

    ViewModeSwitch::ViewModeSwitch(RefCounter* refCounter, const ViewModeSwitch& other):
        Widget(refCounter, other), mValue(other.mValue), mThumbPosition(other.mThumbPosition)
    {
        InitializeDrawables();
    }

    ViewModeSwitch& ViewModeSwitch::operator=(const ViewModeSwitch& other)
    {
        Widget::operator=(other);
        mValue = other.mValue;
        mThumbPosition = other.mThumbPosition;
        return *this;
    }

    void ViewModeSwitch::InitializeDrawables()
    {
        auto& theme = EditorTheme::Get();

        mTrack = mmake<PipelineRoundedRect>();
        mTrack->roundBottom = true;
        mTrack->color = theme.switchTrack;

        mThumb = mmake<PipelineRoundedRect>();
        mThumb->roundBottom = true;
        mThumb->color = theme.switchThumb;

        auto makeLabel = [&](const char* caption)
        {
            auto text = mmake<Text>("stdFont.ttf");
            text->SetText(caption);
            text->SetHorAlign(HorAlign::Middle);
            text->SetVerAlign(VerAlign::Middle);
            text->SetWordWrap(false);
            text->SetDotsEngings(false);
            return text;
        };

        mLabel2D = makeLabel("2D");
        mLabel3D = makeLabel("3D");
    }

    void ViewModeSwitch::SetValue(bool is3D, bool animate /*= true*/)
    {
        mValue = is3D;
        if (!animate)
            mThumbPosition = is3D ? 1.0f : 0.0f;
    }

    bool ViewModeSwitch::GetValue() const
    {
        return mValue;
    }

    float ViewModeSwitch::GetThumbPosition() const
    {
        return mThumbPosition;
    }

    RectF ViewModeSwitch::GetTrackRect() const
    {
        RectF rect = layout->GetWorldRect();
        float centerY = Math::Round(rect.Center().y);
        return RectF(rect.left + sideMargin, centerY - trackHeight*0.5f, rect.right - sideMargin, centerY + trackHeight*0.5f);
    }

    RectF ViewModeSwitch::GetThumbRect() const
    {
        RectF track = GetTrackRect();
        float halfWidth = track.Width()*0.5f - thumbInset;
        float left = Math::Lerp(track.left + thumbInset, track.Center().x, mThumbPosition);
        return RectF(left, track.bottom + thumbInset, left + halfWidth, track.top - thumbInset);
    }

    void ViewModeSwitch::Update(float dt)
    {
        Widget::Update(dt);

        float target = mValue ? 1.0f : 0.0f;
        if (mThumbPosition != target)
        {
            float step = dt*slideSpeed*Math::Max(0.15f, Math::Abs(target - mThumbPosition));
            mThumbPosition = mThumbPosition < target ? Math::Min(target, mThumbPosition + step)
                                                     : Math::Max(target, mThumbPosition - step);
        }
    }

    void ViewModeSwitch::Draw()
    {
        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        auto& theme = EditorTheme::Get();

        RectF track = GetTrackRect();
        mTrack->radius = track.Height()*0.5f;
        mTrack->color = mHovered ? theme.switchTrackHover : theme.switchTrack;
        mTrack->SetRect(track);
        mTrack->SetTransparency(mResTransparency);
        mTrack->Draw();

        RectF thumb = GetThumbRect();
        mThumb->radius = thumb.Height()*0.5f;
        mThumb->color = theme.switchThumb;
        mThumb->SetRect(thumb);
        mThumb->SetTransparency(mResTransparency);
        mThumb->Draw();

        // A caption takes the colour over the thumb as the thumb comes under it
        RectF left(track.left, track.bottom, track.Center().x, track.top);
        RectF right(track.Center().x, track.bottom, track.right, track.top);
        mLabel2D->SetColor(Mix(theme.switchTextOn, theme.switchText, mThumbPosition));
        mLabel3D->SetColor(Mix(theme.switchText, theme.switchTextOn, mThumbPosition));

        for (auto& [label, rect] : { std::make_pair(mLabel2D, left), std::make_pair(mLabel3D, right) })
        {
            label->SetRect(rect);
            label->SetTransparency(mResTransparency);
            label->Draw();
        }

        Widget::Draw();
        CursorAreaEventsListener::OnDrawn();
    }

    bool ViewModeSwitch::IsUnderPoint(const Vec2F& point)
    {
        return Widget::IsUnderPoint(point);
    }

    String ViewModeSwitch::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void ViewModeSwitch::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (!IsUnderPoint(cursor.position))
            return;

        SetValue(!mValue);
        onSwitchByUser(mValue);
    }

    void ViewModeSwitch::OnCursorEnter(const Input::Cursor& cursor)
    {
        mHovered = true;
    }

    void ViewModeSwitch::OnCursorExit(const Input::Cursor& cursor)
    {
        mHovered = false;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::ViewModeSwitch>);
// --- META ---

DECLARE_CLASS(Editor::ViewModeSwitch, Editor__ViewModeSwitch);
// --- END META ---
