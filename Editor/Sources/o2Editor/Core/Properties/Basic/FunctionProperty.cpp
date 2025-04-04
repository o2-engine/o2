#include "o2Editor/stdafx.h"
#include "FunctionProperty.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Utils/Editor/Attributes/NoHeaderAttribute.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/StringUtils.h"
#include "o2Editor/Core/Properties/Basic/ActorProperty.h"
#include "o2Editor/Core/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Core/Properties/Properties.h"
#include <o2/Scene/Components/ScriptableComponent.h>

using namespace o2;

namespace Editor
{
    FunctionProperty::FunctionProperty(RefCounter* refCounter):
        IPropertyField(refCounter)
    {
        InitializeControls();
    }

    FunctionProperty::FunctionProperty(RefCounter* refCounter, const FunctionProperty& other):
        IPropertyField(refCounter, other)
    {
        InitializeControls();
    }

    FunctionProperty& FunctionProperty::operator=(const FunctionProperty& other)
    {
        IPropertyField::operator=(other);
        InitializeControls();
        return *this;
    }

    void FunctionProperty::OnFreeProperty()
    {
    }

    void FunctionProperty::InitializeControls()
    {
        // Spoiler
        mSpoiler = FindChildByType<Spoiler>(false);

        if (!mSpoiler)
        {
            mSpoiler = o2UI.CreateWidget<Spoiler>("expand with caption");
            AddChild(mSpoiler);
        }

        if (mSpoiler)
            mSpoiler->onExpand = THIS_FUNC(OnExpand);

        mSpoiler->borderTop = 5;
        mSpoiler->AddLayer("func icon", mmake<Sprite>("ui/function_icon.png"), Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(6, 0)));
        mSpoiler->GetLayer("caption")->layout.offsetLeft = 24;

        mHeaderContainer = mmake<HorizontalLayout>();
        *mHeaderContainer->layout = WidgetLayout::HorStretch(VerAlign::Top, 100, 0, 20, 0);
        mHeaderContainer->baseCorner = BaseCorner::Right;
        mHeaderContainer->expandHeight = false;
        mHeaderContainer->SetInternalParent(mSpoiler, false);

        // Add button
        mAddButton = mSpoiler->FindChildByTypeAndName<Button>("add button");
        if (!mAddButton)
        {
            mAddButton = o2UI.CreateWidget<Button>("add small");
            mAddButton->name = "add button";
            mAddButton->layout->maxWidth = 20;
            mAddButton->layout->minHeight = 20;
            mAddButton->onClick = THIS_FUNC(OnAddPressed);

            mHeaderContainer->AddChild(mAddButton);
        }

        // Widget sample
        mWidgetSample = o2UI.CreateHorLayout();
        mWidgetSample->layout->minHeight = 20;
        mWidgetSample->expandHeight = true;
        mWidgetSample->expandWidth = true;
        mWidgetSample->fitByChildren = false;

        auto sampleCaption = o2UI.CreateLabel("");
        sampleCaption->name = "caption";
        sampleCaption->layout->maxWidth = 50.0f;
        sampleCaption->horAlign = HorAlign::Left;
        mWidgetSample->AddChild(sampleCaption);

        auto controllersContainerWrapper = mmake<Widget>();
        *controllersContainerWrapper->layout = WidgetLayout::BothStretch();
        controllersContainerWrapper->name = "controls";
        mWidgetSample->AddChild(controllersContainerWrapper);

        auto controlsContainer = o2UI.CreateHorLayout();
        *controlsContainer->layout = WidgetLayout::BothStretch();
        controlsContainer->expandHeight = true;
        controlsContainer->expandWidth = true;
        controlsContainer->spacing = 5.0f;
        controlsContainer->name = "layout";
        controllersContainerWrapper->AddChild(controlsContainer);

        auto sampleRefProperty = o2UI.CreateWidget<ActorProperty>();
        sampleRefProperty->name = "actor ref";
        controlsContainer->AddChild(sampleRefProperty);

        auto sampleFuncDropdown = o2UI.CreateCustomDropdown();
        sampleFuncDropdown->name = "drop down";
        sampleFuncDropdown->SetMaxListSizeInItems(15);
        sampleFuncDropdown->GetItemSample()->layout->minHeight = 20;
        controlsContainer->AddChild(sampleFuncDropdown);

        auto sampleRemoveBtn = o2UI.CreateWidget<Button>("remove small");
        sampleRemoveBtn->name = "remove btn";
        sampleRemoveBtn->layout->maxWidth = 20;
        controlsContainer->AddChild(sampleRemoveBtn);

        // Other
        expandHeight = true;
        expandWidth = true;
        fitByChildren = true;
    }

    void FunctionProperty::OnExpand()
    {
        Refresh();
    }

    FunctionProperty::~FunctionProperty()
    {}

    void FunctionProperty::SetValueAndPrototypeProxy(const TargetsVec& targets)
    {
        mValuesProxies = targets;
        Refresh();
    }

    void FunctionProperty::RefreshInstances()
    {
        mInstances.Clear();

        int proxyIdx = 0;
        for (auto& valueProxy : mValuesProxies)
        {
            auto funcProxy = DynamicCast<PointerValueProxy<AbstractFunction>>(valueProxy.first);
            auto funcProxyProto = DynamicCast<PointerValueProxy<AbstractFunction>>(valueProxy.first);

            if (!funcProxy)
                continue;

            int idx = 0;
            funcProxy->GetValuePointer()->ForEachAbstract(
                Function<void(const IAbstractFunction*)>(
                    [&](const IAbstractFunction* f)
            {
                if (auto actorSubscription = dynamic_cast<const IActorSubscription*>(f))
                {
                    if (idx > mInstances.Count() - 1)
                        mInstances.Add(mmake<FunctionInstance>());

                    mInstances[idx++]->values.Add({ const_cast<IActorSubscription*>(actorSubscription), nullptr });
                }
            }));

            if (funcProxyProto)
            {
                idx = 0;
                funcProxyProto->GetValuePointer()->ForEachAbstract(
                    Function<void(const IAbstractFunction*)>(
                        [&](const IAbstractFunction* f)
                {
                    if (auto actorSubscription = dynamic_cast<const IActorSubscription*>(f))
                    {
                        if (idx < mInstances.Count() && proxyIdx < mInstances[idx]->values.Count())
                            mInstances[idx]->values[proxyIdx].second = const_cast<IActorSubscription*>(actorSubscription);
                    }
                }));
            }

            proxyIdx++;
        }
    }

    void FunctionProperty::Refresh(bool forcible /*= false*/)
    {
        PushEditorScopeOnStack scope;

        auto prevInstances = mInstances;
        RefreshInstances();

        mSpoiler->RemoveAllChildren();

        int idx = 0;
        for (auto& instance : mInstances)
        {
            auto prevInstanceIdx = prevInstances.IndexOf([&](auto& x) { return x->values == instance->values; });
            if (prevInstanceIdx >= 0)
            {
                instance = prevInstances[prevInstanceIdx];
                prevInstances.RemoveAt(prevInstanceIdx);
            }
            else
            {
                auto widget = CreateWidget();

                instance->layout = widget->layout;
                instance->refProperty = widget->refProperty;
                instance->removeBtn = widget->removeBtn;
                instance->caption = widget->caption;
                instance->funcDropDown = widget->funcDropDown;
            }

            instance->refProperty->SelectValueAndPrototypePointers<Ref<Actor>, IActorSubscription>(
                instance->values.Convert<IActorSubscription*>([](auto p) { return p.first; }),
                instance->values.Convert<IActorSubscription*>([](auto p) { return p.second; }),
                [](IActorSubscription* s)
            {
                if (!s->actorRef && s->componentRef)
                    s->actorRef = s->componentRef->GetActor();

                return &s->actorRef;
            }
            );

            instance->refProperty->onChangeCompleted =
                [=](const String&, const Vector<DataDocument>&, const Vector<DataDocument>&)
            {
                instance->UpdateFunctionsList(instance->refProperty->GetCommonValue(),
                                              !instance->values.IsEmpty() ? instance->values[0].first->componentRef : Ref<Component>(),
                                              !instance->values.IsEmpty() ? instance->values[0].first->method : String());
            };

            instance->funcDropDown->onSelectedPos = [=](int idx) { instance->OnFunctionSelected(idx); };

            instance->removeBtn->onClick = [=]() { OnRemovePressed(instance); };

            instance->UpdateFunctionsList(instance->refProperty->GetCommonValue(),
                                          !instance->values.IsEmpty() ? instance->values[0].first->componentRef : Ref<Component>(),
                                          !instance->values.IsEmpty() ? instance->values[0].first->method : String());

            instance->caption->text = "#" + (String)idx;
            mSpoiler->AddChild(instance->layout);
            idx++;
        }
    }

    void FunctionProperty::OnAddPressed()
    {
        for (auto& valueProxy : mValuesProxies)
        {
            if (auto funcProxy = DynamicCast<PointerValueProxy<AbstractFunction>>(valueProxy.first))
                funcProxy->GetValuePointer()->AddActorSubscription();
        }

        Expand();
        Refresh();
    }

    void FunctionProperty::OnRemovePressed(const Ref<FunctionInstance>& instance)
    {
        for (auto& valueProxy : mValuesProxies)
        {
            if (auto funcProxy = DynamicCast<PointerValueProxy<AbstractFunction>>(valueProxy.first))
            {
                for (auto& value : instance->values)
                    funcProxy->GetValuePointer()->RemoveFunction(dynamic_cast<IAbstractFunction*>(value.first));
            }
        }

        Refresh();
    }

    Ref<FunctionProperty::FunctionInstance> FunctionProperty::CreateWidget()
    {
        auto res = mmake<FunctionInstance>();

        if (mWidgetsBuffer.IsEmpty())
            res->layout = mWidgetSample->CloneAsRef<HorizontalLayout>();
        else
            res->layout = mWidgetsBuffer.PopBack();

        res->caption = res->layout->GetChildByType<Label>("caption");
        res->funcDropDown = res->layout->GetChildByType<CustomDropDown>("controls/layout/drop down");
        res->refProperty = res->layout->GetChildByType<ActorProperty>("controls/layout/actor ref");
        res->removeBtn = res->layout->GetChildByType<Button>("controls/layout/remove btn");

        return res;
    }

    void FunctionProperty::FunctionInstance::UpdateFunctionsList(const Ref<Actor>& actor, const Ref<Component>& selectedComponent,
                                                                 const String& selectedMethod)
    {
        funcDropDown->RemoveAllItems();
        functionsDropdownMap.Clear();

        if (!actor)
            return;

        int selectedIdx = -1;

        auto collectFunctions = [&](const String& typeName, const String& iconName, const Vector<String>& functionsList, 
                                    const Ref<Component>& comp)
        {
            auto typeItem = funcDropDown->AddItem();
            functionsDropdownMap.Add({ nullptr, "" });

            typeItem->AddLayer("back", mmake<Sprite>(Color4(0, 0, 0, 0)))->transparency = 0.05f;

            auto icon = o2UI.CreateImage(iconName);
            *icon->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20));
            icon->GetImage()->color = Color4(0, 156, 141, 255);
            typeItem->AddChild(icon);

            auto text = o2UI.CreateLabel(typeName);
            *text->layout = WidgetLayout::BothStretch(20, 0, 0, 0);
            text->horAlign = HorAlign::Left;
            typeItem->AddChild(text);

            for (auto& funcName : functionsList)
            {
                auto funcItem = funcDropDown->AddItem();

                auto text = o2UI.CreateLabel(funcName);
                *text->layout = WidgetLayout::BothStretch();
                text->horAlign = HorAlign::Left;
                text->transparency = 0.8f;
                funcItem->AddChild(text);

                functionsDropdownMap.Add({ comp, funcName });

                if (comp == selectedComponent.Get() && selectedMethod == funcName)
                    selectedIdx = funcDropDown->GetItemsCount() - 1;
            }
        };

        auto collectFunctionsByType = [&](const String& typeName, const String& iconName, const Type& type, 
                                          const Ref<Component>& comp)
        {
            Vector<String> functionsList;

            for (auto& funcInfo : type.GetFunctions())
            {
                if (funcInfo->GetProtectionSection() == ProtectSection::Public && funcInfo->GetParameters().IsEmpty() &&
                    funcInfo->GetReturnType() == &TypeOf(void))
                {
                    functionsList.Add(funcInfo->GetName());
                }
            }

            collectFunctions(typeName, iconName, functionsList, comp);
        };

        auto getTypeName = [](const Type& type)
        {
            String typeName = type.InvokeStatic<String>("GetName");
            if (typeName.IsEmpty())
                typeName = GetSmartName(type.GetName());

            return typeName;
        };

        auto getIconName = [](const Type& type)
        {
            String iconName = type.InvokeStatic<String>("GetIcon");
            if (iconName.IsEmpty())
                iconName = "ui/UI4_component_icon.png";

            return iconName;
        };

        auto& actorType = actor->GetType();
        collectFunctionsByType(getTypeName(actorType), getIconName(actorType), actorType, nullptr);

        for (auto& component : actor->GetComponents())
        {
            auto& componentType = component->GetType();
            if (auto scriptingComponent = DynamicCast<ScriptableComponent>(component))
            {
                Vector<String> functionsList;

                static Vector<String> hiddenFunctions = { 
                    String("constructor"), String("OnStart"), String("Update"), String("toString"), String("valueOf"), 
                    String("toLocaleString"), String("hasOwnProperty"), String("isPrototypeOf"), String("propertyIsEnumerable") };

                ScriptValue instance = scriptingComponent->GetInstance();
                Vector<ScriptValue> properties = instance.GetPropertyNames();
                for (auto& prop : properties)
                {
                    ScriptValue propValue = instance.GetProperty(prop);
                    if (propValue.IsFunction())
                    {
                        String functionName = prop.ToString();
                        bool hidden = hiddenFunctions.Contains(functionName);
                        if (!functionName.StartsWith("_") && !hidden)
                            functionsList.Add(functionName);
                    }
                }

                collectFunctions(getTypeName(componentType), getIconName(componentType), functionsList, component);
            }
            else
            {
                collectFunctionsByType(getTypeName(componentType), getIconName(componentType), componentType, component);
            }
        }

        if (funcDropDown->GetSelectedItemPosition() != selectedIdx)
            funcDropDown->SelectItemAt(selectedIdx);
    }

    void FunctionProperty::FunctionInstance::OnFunctionSelected(int idx)
    {
        for (auto& value : values)
        {
            value.first->componentRef = functionsDropdownMap[idx].first;
            value.first->method = functionsDropdownMap[idx].second;
        }
    }

    const Type* FunctionProperty::GetValueType() const
    {
        return GetValueTypeStatic();
    }

    const Type* FunctionProperty::GetValueTypeStatic()
    {
        return FunctionType::serializableType;
    }

    void FunctionProperty::SetFieldInfo(const FieldInfo* fieldInfo)
    {
        IPropertyField::SetFieldInfo(fieldInfo);

        if (fieldInfo)
        {
            if (fieldInfo->HasAttribute<ExpandedByDefaultAttribute>())
                mSpoiler->Expand();
        }
    }

    void FunctionProperty::SetCaption(const WString& text)
    {
        mSpoiler->SetCaption(text);

        auto spoilerCaptionLayer = mSpoiler->GetLayerDrawable<Text>("caption");
        if (spoilerCaptionLayer)
        {
            Vec2F captionSize = Text::GetTextSize(text, spoilerCaptionLayer->GetFont(), spoilerCaptionLayer->GetFontHeight());
            *mHeaderContainer->layout = WidgetLayout::HorStretch(VerAlign::Top, captionSize.x + 20.0f, 0, 17, 0);
        }
    }

    WString FunctionProperty::GetCaption() const
    {
        return mSpoiler->GetCaption();
    }

    Ref<Button> FunctionProperty::GetRemoveButton()
    {
        if (!mRemoveBtn)
        {
            mRemoveBtn = o2UI.CreateWidget<Button>("remove small");
            *mRemoveBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(2, 0));
            AddInternalWidget(mRemoveBtn);
        }

        return mRemoveBtn;
    }

    void FunctionProperty::Expand()
    {
        SetExpanded(true);
    }

    void FunctionProperty::Collapse()
    {
        SetExpanded(false);
    }

    void FunctionProperty::SetExpanded(bool expanded)
    {
        mSpoiler->SetExpanded(expanded);
    }

    bool FunctionProperty::IsExpanded() const
    {
        return mSpoiler->IsExpanded();
    }

    void FunctionProperty::OnPropertyChanged(const String& path, const Vector<DataDocument>& before,
                                             const Vector<DataDocument>& after)
    {
        //         for (auto& pair : mTargetObjects)
        //             pair.first.SetValue();

        onChangeCompleted(path, before, after);
    }

    bool FunctionProperty::FunctionInstance::operator==(const FunctionInstance& other) const
    {
        return values == other.values;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::FunctionProperty>);
// --- META ---

DECLARE_CLASS(Editor::FunctionProperty, Editor__FunctionProperty);
// --- END META ---
