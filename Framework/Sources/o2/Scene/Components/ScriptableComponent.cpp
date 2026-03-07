#include "o2/Utils/Debug/Debug.h"
#include "o2/stdafx.h"
#include "ScriptableComponent.h"

#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    ScriptableComponent::ScriptableComponent()
    {

    }

    ScriptableComponent::ScriptableComponent(const ScriptableComponent& other):
        Component(other)
    {
        SetScript(other.mScript);
    }

    ScriptableComponent::~ScriptableComponent()
    {

    }

    ScriptableComponent& ScriptableComponent::operator=(const ScriptableComponent& other)
    {
        Component::operator=(other);
        return *this;
    }

    void ScriptableComponent::SetScript(const AssetRef<JavaScriptAsset>& script)
    {
        if (mScript == script)
            return;

        mScript = script;
        LoadScriptAndCreateObject();
    }

    const AssetRef<JavaScriptAsset>& ScriptableComponent::GetScript() const
    {
        return mScript;
    }

    ScriptValue ScriptableComponent::GetInstance() const
    {
        return mInstance;
    }

    ScriptValue ScriptableComponent::GetClass() const
    {
        return mClass;
    }

    String ScriptableComponent::GetName()
    {
        return "Scriptable";
    }

    String ScriptableComponent::GetCategory()
    {
        return "Basic";
    }

    String ScriptableComponent::GetIcon()
    {
        return "ui/UI4_component_icon.png";
    }

    void ScriptableComponent::LoadScriptAndCreateObject()
    {
        if (!mScript)
            return;

        auto parseRes = mScript->Parse();
        o2Scripts.Run(parseRes);

        auto className = o2FileSystem.GetPathWithoutDirectories(o2FileSystem.GetFileNameWithoutExtension(mScript->GetPath()));
        auto classObj = o2Scripts.GetGlobal().GetOwnProperty(ScriptValue(className));
        if (classObj.IsConstructor())
        {
            mInstance = classObj.Construct({});
            mClass = classObj;

            //o2Debug.LogStr(L"Instance: " + mInstance.Dump());

            mOnStartFunc = mInstance.GetProperty("OnStart");
            mOnEnabledFunc = mInstance.GetProperty("OnEnabled");
            mOnDisabledFunc = mInstance.GetProperty("OnDisabled");
            mUpdateFunc = mInstance.GetProperty("Update");
        }
        else
        {
            o2Debug.LogError("Can't find scripting class '" + className + "' in component file - " + mScript->GetPath());

            mInstance = ScriptValue();
            mClass = ScriptValue();
            mOnStartFunc = ScriptValue();
            mOnEnabledFunc = ScriptValue();
            mOnDisabledFunc = ScriptValue();
            mUpdateFunc = ScriptValue();
        }
    }

    void ScriptableComponent::OnSerialize(DataValue& node) const
    {
        Component::OnSerialize(node);

        node["mScript"].Set(mScript);

        if (mInstance.IsObject())
            node["mInstance"].Set(mInstance);
    }

    void ScriptableComponent::OnDeserialized(const DataValue& node)
    {
        Component::OnDeserialized(node);

        node["mScript"].Get(mScript);
        LoadScriptAndCreateObject();

        if (mInstance.IsObject())
        {
            if (auto objectNode = node.FindMember("mInstance"))
            {
                objectNode->Get(mInstance);
                //o2Debug.LogStr(L"Instance: " + mInstance.Dump());
            }
        }
    }

    void ScriptableComponent::SetOwnerActor(const Ref<Actor>& actor)
    {
        Component::SetOwnerActor(actor);

        //if (mInstance.IsObject())
            mInstance.SetProperty("_actor", actor);
    }

    void ScriptableComponent::OnAddToScene()
    {
        Component::OnAddToScene();
    }

    void ScriptableComponent::OnRemoveFromScene()
    {
        Component::OnRemoveFromScene();
    }

    void ScriptableComponent::OnStart()
    {
        if (mOnStartFunc.IsFunction())
            mOnStartFunc.InvokeRaw(mInstance, {});
    }

    void ScriptableComponent::OnUpdate(float dt)
    {
        PROFILE_SAMPLE_FUNC();

        if (mUpdateFunc.IsFunction())
            mUpdateFunc.Invoke<void, float>(mInstance, dt);
    }

    void ScriptableComponent::OnEnabled()
    {
        if (mOnEnabledFunc.IsFunction())
            mOnEnabledFunc.InvokeRaw(mInstance, {});
    }

    void ScriptableComponent::OnDisabled()
    {
        if (mOnDisabledFunc.IsFunction())
            mOnDisabledFunc.InvokeRaw(mInstance, {});
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::ScriptableComponent>);
// --- META ---

DECLARE_CLASS(o2::ScriptableComponent, o2__ScriptableComponent);
// --- END META ---
