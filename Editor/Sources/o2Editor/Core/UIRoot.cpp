#include "o2Editor/stdafx.h"
#include "UIRoot.h"

#include "o2/Application/Application.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"

DECLARE_SINGLETON(Editor::UIRoot);

namespace Editor
{
	UIRoot::UIRoot()
	{
		mRootWidget = mmake<Ref<Widget>>(ActorCreateMode::NotInScene);
	}

	UIRoot::~UIRoot()
	{
		delete mRootWidget;
	}

	Ref<Widget> UIRoot::AddWidget(const Ref<Widget>& widget)
	{
		mRootWidget->AddChild(widget);
		return widget;
	}

	void UIRoot::RemoveWidget(const Ref<Widget>& widget)
	{
		mRootWidget->RemoveChild(widget);
	}

	void UIRoot::RemoveAllWidgets()
	{
		mRootWidget->RemoveAllChildren();
	}

	Ref<Widget> UIRoot::GetRootWidget()
	{
		return mRootWidget;
	}

	void UIRoot::Draw()
	{
		mRootWidget->Draw();
	}

	void UIRoot::Update(float dt)
	{
		mRootWidget->Update(dt);
		mRootWidget->UpdateChildren(dt);
	}

	void UIRoot::OnApplicationSized()
	{
		*mRootWidget->layout = WidgetLayout::Based(BaseCorner::Center, (Vec2F)o2Application.GetContentSize()/o2Application.GetGraphicsScale());
	}
}