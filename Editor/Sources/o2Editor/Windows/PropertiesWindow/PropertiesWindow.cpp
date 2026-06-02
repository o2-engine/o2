#include "o2Editor/stdafx.h"
#include "PropertiesWindow.h"

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Windows/PropertiesWindow/DefaultPropertiesViewer.h"
#include "o2Editor/Windows/PropertiesWindow/IPropertiesViewer.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"

DECLARE_SINGLETON(Editor::PropertiesWindow);

namespace Editor
{
    PropertiesWindow::PropertiesWindow(RefCounter* refCounter):
        IEditorWindow(refCounter), Singleton<PropertiesWindow>(refCounter), mCurrentViewer(nullptr)
    {
        InitializeWindow();
        mViewersSelector.Initialize();
    }

    PropertiesWindow::~PropertiesWindow()
    {
        if (mCurrentViewer)
            mCurrentViewer->SetPropertiesEnabled(false);
    }

    void PropertiesWindow::ResetTargets()
    {
        SetTarget(nullptr);
    }

    void PropertiesWindow::InitializeWindow()
    {
        mWindow->caption = "Properties";
        mWindow->name = "properties window";
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_gear_icon.png"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 2)));
        mWindow->SetViewLayout(Layout::BothStretch(-2, 0, 0, 18));
        mWindow->SetClippingLayout(Layout::BothStretch(-1, -2, 0, 15));

		mWindow->onFocused += THIS_FUNC(OnFocusedWindow);

        InitializeWindowContext();
    }

    void PropertiesWindow::InitializeWindowContext()
    {
        auto context = mWindow->GetOptionsMenu();
        if (!context)
        {
            o2Debug.LogError("Failed to initialize properties window context menu: not found menu on window");
            return;
        }

        context->AddItem(ContextMenu::Item::Separator());
        context->AddItem(mmake<ContextMenu::Item>("Private visible", false, THIS_FUNC(OnPrivateFieldsVisibleChanged)));
    }

    void PropertiesWindow::OnPrivateFieldsVisibleChanged(bool visible)
    {
        o2EditorProperties.SetPrivateFieldsVisible(visible);

        if (mCurrentViewer)
            mCurrentViewer->SetTargets(mTargets);
    }

    void PropertiesWindow::OnPropertyChanged(const Vector<IObject*>& targets, const Ref<IPropertyField>& field, bool byUser)
    {
        mTargetsChanged = true;

        onPropertyChanged(targets, field, byUser);
    }

    void PropertiesWindow::OnPropertyChangeCompleted(const Vector<IObject*>& targets, const String& path, const Vector<DataDocument>& before, 
                                                     const Vector<DataDocument>& after)
    {
        onPropertyChangeCompleted(targets, path, before, after);

        o2EditorSceneWindow.DoneActorPropertyChangeAction(path, before, after);
    }

	void PropertiesWindow::OnFocusedWindow()
	{
		o2EditorTree.OnSceneFocused();
	}

	void PropertiesWindow::SetTarget(IObject* target)
    {
        if (target == nullptr)
            SetTargets(Vector<IObject*>());
        else
            SetTargets({ target });
    }

    void PropertiesWindow::SetTargets(const Vector<IObject*>& targets, const Function<void()>& targetsChangedDelegate /*= Function<void()>()*/)
    {
        if (mTargetsChanged)
            mOnTargetsChangedDelegate();

        Ref<IPropertiesViewer> objectViewer = mViewersSelector.Select(targets);

        if (objectViewer != mCurrentViewer)
        {
            using thisclass = PropertiesWindow;
            
            if (mCurrentViewer)
            {
                mCurrentViewer->mContentWidget->Hide(true);
                mCurrentViewer->SetPropertiesEnabled(false);

                mCurrentViewer->onPropertyChanged -= THIS_FUNC(OnPropertyChanged);
                mCurrentViewer->onPropertyChangeCompleted -= THIS_FUNC(OnPropertyChangeCompleted);
            }

            mCurrentViewer = objectViewer;

            if (mCurrentViewer)
            {
                mCurrentViewer->mContentWidget->SetParent(mWindow);
                *mCurrentViewer->mContentWidget->layout = WidgetLayout::BothStretch();
                mCurrentViewer->mContentWidget->Show(true);

                mCurrentViewer->onPropertyChanged += THIS_FUNC(OnPropertyChanged);
                mCurrentViewer->onPropertyChangeCompleted += THIS_FUNC(OnPropertyChangeCompleted);
            }
        }

        mTargets = targets;

        if (mCurrentViewer)
        {
            mCurrentViewer->SetTargets(mTargets);
            mCurrentViewer->SetPropertiesEnabled(true);
        }

        mOnTargetsChangedDelegate = targetsChangedDelegate;
        mTargetsChanged = false;
    }

    Vector<IObject*> PropertiesWindow::GetTargets() const
    {
        return mTargets;
    }

    void PropertiesWindow::Update(float dt)
    {
        mRefreshRemainingTime -= dt;
        if (mRefreshRemainingTime < 0.0f)
        {
            mRefreshRemainingTime = mRefreshDelay;
            if (mCurrentViewer)
                mCurrentViewer->Refresh();
        }

        if (mCurrentViewer)
            mCurrentViewer->Update(dt);
    }

    void PropertiesWindow::Draw()
    {
        if (mCurrentViewer)
            mCurrentViewer->Draw();
    }

    bool PropertiesWindow::IsTargetsChanged() const
    {
        return mTargetsChanged;
    }

    Ref<RefCounterable> PropertiesWindow::CastToRefCounterable(const Ref<PropertiesWindow>& ref)
    {
        return DynamicCast<Singleton<PropertiesWindow>>(ref);
    }

}
// --- META ---

DECLARE_CLASS(Editor::PropertiesWindow, Editor__PropertiesWindow);
// --- END META ---
