#include "o2Editor/stdafx.h"
#include "IObjectPropertiesViewer.h"

#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Properties/IPropertyField.h"

namespace Editor
{
    FORWARD_REF_IMPL(IPropertyField);

    IObjectPropertiesViewer::IObjectPropertiesViewer()
    {
        mPropertiesContext = mmake<PropertiesContext>();

        mOnChildFieldChangeCompleted =
            MakeFunction<IObjectPropertiesViewer, void, const String&,
            const Vector<DataDocument>&, const Vector<DataDocument>&>(this, &IObjectPropertiesViewer::OnFieldChangeCompleted);
    }

    void IObjectPropertiesViewer::SetHeaderEnabled(bool enabled)
    {
        mHeaderEnabled = enabled;

        auto spoiler = GetSpoiler();
        if (mHeaderEnabled)
        {
            spoiler->SetHeadHeight(20);
            spoiler->GetLayerDrawable<Text>("caption")->enabled = true;
            spoiler->GetInternalWidget("expand")->enabledForcibly = true;
            spoiler->borderLeft = 10;
            spoiler->borderTop = 2;
        }
        else
        {
            spoiler->SetHeadHeight(0);
            spoiler->GetLayerDrawable<Text>("caption")->enabled = false;
            spoiler->GetInternalWidget("expand")->enabledForcibly = false;
            spoiler->borderLeft = 0;
            spoiler->borderTop = 0;
            spoiler->Expand(true);
        }

        OnHeaderEnableChanged(enabled);
    }

    bool IObjectPropertiesViewer::IsHeaderEnabled() const
    {
        return mHeaderEnabled;
    }

    void IObjectPropertiesViewer::Refresh(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        mTargetObjects = targetObjets;
        auto spoiler = GetSpoiler();

        if (!mHeaderEnabled || spoiler->IsExpanded())
        {
            bool force = CheckBuildProperties(targetObjets);
            mPropertiesContext->Set(targetObjets, force);
        }

        if (!targetObjets.IsEmpty())
            mRealObjectType = &(targetObjets[0].first)->GetType();

        OnRefreshed(targetObjets);
    }

    const Type* IObjectPropertiesViewer::GetViewingObjectType() const
    {
        if (mRealObjectType)
            return mRealObjectType;

        return GetViewingObjectTypeStatic();
    }

    const Type* IObjectPropertiesViewer::GetViewingObjectTypeStatic()
    {
        return nullptr;
    }

    void IObjectPropertiesViewer::SetParentContext(const Ref<PropertiesContext>& context)
    {
        mPropertiesContext->parent = context;
    }

    const Ref<Spoiler>& IObjectPropertiesViewer::GetSpoiler()
    {
        return mSpoiler;
    }

	Ref<Spoiler> IObjectPropertiesViewer::CheckCreateSpoiler(const Ref<Widget>& parent)
	{
        if (mSpoiler)
        {
			parent->AddChild(mSpoiler);
            return mSpoiler;
        }

		return CreateSpoiler(parent);
	}

	void IObjectPropertiesViewer::SetExpanded(bool expanded, bool forcible /*= false*/)
    {
        GetSpoiler()->SetExpanded(expanded, forcible);
    }

    bool IObjectPropertiesViewer::IsExpanded() const
    {
        return const_cast<IObjectPropertiesViewer*>(this)->GetSpoiler()->IsExpanded();
    }

    void IObjectPropertiesViewer::SetCaption(const WString& caption)
    {
        GetSpoiler()->SetCaption(caption);
    }

    const WString& IObjectPropertiesViewer::GetCaption() const
    {
        return const_cast<IObjectPropertiesViewer*>(this)->GetSpoiler()->GetCaption();
    }

    bool IObjectPropertiesViewer::IsEmpty() const
    {
        return mSpoiler->GetChildren().IsEmpty();
    }

    void IObjectPropertiesViewer::OnPropertiesEnabled()
    {
        mPropertiesContext->SetPropertiesEnabled(true);
    }

    void IObjectPropertiesViewer::OnPropertiesDisabled()
    {
        mPropertiesContext->SetPropertiesEnabled(false);
    }

    Ref<Spoiler> IObjectPropertiesViewer::CreateSpoiler(const Ref<Widget>& parent)
    {
        mSpoiler = o2UI.CreateWidget<Spoiler>("expand with caption");
        mSpoiler->onExpand = [&]() { Refresh(mTargetObjects); };
        parent->AddChild(mSpoiler);

        return mSpoiler;
    }

    bool IObjectPropertiesViewer::CheckBuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        if (mPropertiesBuilt)
            return false;

        PushEditorScopeOnStack scope;
        RebuildProperties(targetObjets);

        mPropertiesBuilt = true;

        return true;
    }

    void IObjectPropertiesViewer::OnFieldChangeCompleted(const String& path, const Vector<DataDocument>& before,
                                                         const Vector<DataDocument>& after)
    {
        onChangeCompleted(this->path + "/" + path, before, after);
    }

}
// --- META ---

DECLARE_CLASS(Editor::IObjectPropertiesViewer, Editor__IObjectPropertiesViewer);
// --- END META ---
