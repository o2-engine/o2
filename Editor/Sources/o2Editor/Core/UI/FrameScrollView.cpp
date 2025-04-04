#include "o2Editor/stdafx.h"
#include "FrameScrollView.h"

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"

namespace Editor
{
    FrameScrollView::FrameScrollView(RefCounter* refCounter) :
        ScrollView(refCounter)
    {
        mReady = false;

        mHorScrollbar = mmake<HorizontalScrollBar>();
        *mHorScrollbar->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, 20);
        mHorScrollbar->SetInternalParent(Ref(this));
        mHorScrollbar->onChangeByUser = THIS_FUNC(OnHorScrollScrolled);

        mVerScrollbar = mmake<VerticalScrollBar>();
        *mVerScrollbar->layout = WidgetLayout::VerStretch(HorAlign::Right, 0, 0, 20);
        mVerScrollbar->SetInternalParent(Ref(this));
        mVerScrollbar->onChangeByUser = THIS_FUNC(OnVerScrollScrolled);

        mReady = true;
    }

    FrameScrollView::FrameScrollView(RefCounter* refCounter, const FrameScrollView& other) :
        ScrollView(refCounter, other), mHorScrollbar(other.mHorScrollbar->CloneAs<HorizontalScrollBar>()),
        mVerScrollbar(other.mVerScrollbar->CloneAs<VerticalScrollBar>())
    {
        mReady = false;

        mHorScrollbar->SetInternalParent(Ref(this));
        mHorScrollbar->onChangeByUser = THIS_FUNC(OnHorScrollScrolled);

        mVerScrollbar->SetInternalParent(Ref(this));
        mVerScrollbar->onChangeByUser = THIS_FUNC(OnVerScrollScrolled);

        RetargetStatesAnimations();

        mReady = true;
    }

    FrameScrollView::~FrameScrollView()
    {}

    FrameScrollView& FrameScrollView::operator=(const FrameScrollView& other)
    {
        ScrollView::operator=(other);

        mHorScrollbar = other.mHorScrollbar->CloneAsRef<HorizontalScrollBar>();
        mHorScrollbar->SetInternalParent(Ref(this));
        mHorScrollbar->onChangeByUser = THIS_FUNC(OnHorScrollScrolled);

        mVerScrollbar = other.mVerScrollbar->CloneAsRef<VerticalScrollBar>();
        mVerScrollbar->SetInternalParent(Ref(this));
        mVerScrollbar->onChangeByUser = THIS_FUNC(OnVerScrollScrolled);

        return *this;
    }

    void FrameScrollView::Draw()
    {
        ScrollView::Draw();
    }

    void FrameScrollView::Update(float dt)
    {
        ScrollView::Update(dt);

        if (!mReady)
            return;

        UpdateCameraLimits(dt);
    }

    void FrameScrollView::UpdateSelfTransform()
    {
        ScrollView::UpdateSelfTransform();
    }

    void FrameScrollView::SetHorScrollbar(const Ref<HorizontalScrollBar>& scrollbar)
    {
		if (mHorScrollbar)
			RemoveInternalWidget(mHorScrollbar);

        mHorScrollbar = scrollbar;
        mHorScrollbar->SetInternalParent(Ref(this));
        mHorScrollbar->onChangeByUser = THIS_FUNC(OnHorScrollScrolled);

        SetLayoutDirty();
    }

    void FrameScrollView::SetVerScrollbar(const Ref<VerticalScrollBar>& scrollbar)
    {
		if (mVerScrollbar)
			RemoveInternalWidget(mVerScrollbar);

        mVerScrollbar = scrollbar;
        mVerScrollbar->SetInternalParent(Ref(this));
        mVerScrollbar->onChangeByUser = THIS_FUNC(OnVerScrollScrolled);

        SetLayoutDirty();
    }

    void FrameScrollView::SetViewArea(const RectF& area)
    {
        mAvailableArea = area;
    }

    RectF FrameScrollView::GetViewArea() const
    {
        return mAvailableArea;
    }

    String FrameScrollView::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void FrameScrollView::UpdateCameraLimits(float dt)
    {
        if (o2Input.IsCursorDown())
            return;

        RectF camRect = mViewCamera.GetRect();
        Vec2F camSize = camRect.Size();

        mHorScrollbar->SetValueRange(Math::Min(mAvailableArea.left + camSize.x*0.5f, camRect.Center().x),
                                     Math::Max(mAvailableArea.right - camSize.x*0.5f, camRect.Center().x));
        mHorScrollbar->SetScrollHandleSize(camRect.Width());

        mVerScrollbar->SetValueRange(Math::Min(mAvailableArea.bottom + camSize.y*0.5f, camRect.Center().y),
                                     Math::Max(mAvailableArea.top - camSize.y*0.5f, camRect.Center().y));

        mVerScrollbar->SetScrollHandleSize(camRect.Height());
    }

    void FrameScrollView::OnHorScrollScrolled(float value)
    {
        mViewCamera.SetPosition(Vec2F(value, mViewCamera.GetPosition().y));
        mViewCameraTargetPos = mViewCamera.GetPosition();
        mNeedRedraw = true;
    }

    void FrameScrollView::OnVerScrollScrolled(float value)
    {
        float min = mVerScrollbar->GetMinValue();
        float max = mVerScrollbar->GetMaxValue();

        float invertedValue = min + (max - min - (value - min));
        mViewCamera.SetPosition(Vec2F(mViewCamera.GetPosition().x, invertedValue));
        mViewCameraTargetPos = mViewCamera.GetPosition();
        mNeedRedraw = true;
    }

    void FrameScrollView::OnCameraTransformChanged()
    {
        mHorScrollbar->SetValue(mViewCamera.GetPosition().x);

        float min = mVerScrollbar->GetMinValue();
        float max = mVerScrollbar->GetMaxValue();
        mVerScrollbar->SetValue(min + (max - min - (mViewCamera.GetPosition().y - min)));

        onViewChanged();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::FrameScrollView>);
// --- META ---

DECLARE_CLASS(Editor::FrameScrollView, Editor__FrameScrollView);
// --- END META ---
