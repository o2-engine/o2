#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Material.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scripts/ScriptEngine.h"
#include "o2/Scripts/ScriptsCastUtils.h"
#include "o2/Scripts/ScriptValue.h"

using namespace o2;

#if IS_SCRIPTING_SUPPORTED

// Scene-building script API: constructors, AddComponent, secondary-base methods copied onto
// class prototypes, and SerializableFunction fields assignable with JS functions
namespace
{
	ScriptValue EvalChecked(const char* code)
	{
		ScriptValue res = o2Scripts.Eval(code);
		EXPECT_NE(res.GetValueType(), ScriptValue::ValueType::Error) << res.GetError().Data();
		return res;
	}
}

TEST(ScriptSceneApi, ActorAddComponentFromScript)
{
	auto res = EvalChecked(
		"var sceneApi_actor = new o2.Actor(1);" // NotInScene
		"sceneApi_actor.SetName('fromJs');"
		"var sceneApi_emitter = new o2.ParticlesEmitterComponent();"
		"sceneApi_actor.AddComponent(sceneApi_emitter);"
		"sceneApi_actor.GetComponents().length");

	EXPECT_EQ(res.GetValue<int>(), 1);
}

TEST(ScriptSceneApi, SecondaryBaseMethodsReachClassPrototype)
{
	// ParticlesEmitter is the second base of ParticlesEmitterComponent: its methods must be
	// copied onto the component prototype by the post-registration pass
	auto res = EvalChecked(
		"var sceneApi_pe = new o2.ParticlesEmitterComponent();"
		"typeof sceneApi_pe.SetParticlesPerSecond === 'function' &&"
		"typeof sceneApi_pe.Play === 'function' &&"
		"typeof sceneApi_pe.SetShape === 'function'");

	EXPECT_TRUE(res.GetValue<bool>());

	EvalChecked(
		"sceneApi_pe.SetParticlesPerSecond(15);"
		"sceneApi_pe.SetMaxParticles(7);");

	auto perSecond = EvalChecked("sceneApi_pe.GetParticlesPerSecond()");
	auto maxParticles = EvalChecked("sceneApi_pe.GetMaxParticles()");
	EXPECT_FLOAT_EQ(perSecond.ToNumber(), 15.0f);
	EXPECT_FLOAT_EQ(maxParticles.ToNumber(), 7.0f);

	auto onOwnProto = EvalChecked(
		"Object.getOwnPropertyNames(Object.getPrototypeOf(sceneApi_pe)).indexOf('GetParticlesPerSecond') >= 0");
	EXPECT_TRUE(onOwnProto.GetValue<bool>()) << "copied method must be an own property of the class prototype";

	// Value readback through the reflection cast; a plain DynamicCast here would rely on RTTI
	// cross-casts, which are unreliable across the static libs of the test binary
	auto jsObject = o2Scripts.GetGlobal().GetProperty("sceneApi_pe");
	ASSERT_TRUE(jsObject.IsObjectContainer());

	auto component = jsObject.GetValue<Ref<Component>>();
	ASSERT_TRUE(component);

	auto emitterType = Reflection::GetType("o2::ParticlesEmitter");
	ASSERT_TRUE(emitterType != nullptr);

	auto emitter = (ParticlesEmitter*)CastThroughReflection(&component->GetType(), component.Get(), *emitterType);
	ASSERT_TRUE(emitter);
	EXPECT_FLOAT_EQ(emitter->GetParticlesPerSecond(), 15.0f) << "setter thunk must have applied";
}

TEST(ScriptSceneApi, ButtonOnClickAssignableWithJsFunction)
{
	auto res = EvalChecked(
		"var sceneApi_btn = new o2.Button(1);"
		"var sceneApi_clicks = 0;"
		"sceneApi_btn.onClick = function() { sceneApi_clicks++; };"
		"sceneApi_btn");

	auto button = res.GetValue<Ref<Button>>();
	ASSERT_TRUE(button);

	button->onClick();
	button->onClick();

	EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("sceneApi_clicks").GetValue<int>(), 2);
}

TEST(ScriptSceneApi, MaterialParamsBuildFromScript)
{
	auto res = EvalChecked(
		"var sceneApi_mat = new o2.Material();"
		"sceneApi_mat.AddParam(new o2.ShaderParamFloat('u_test', 0.25));"
		"var sceneApi_param = sceneApi_mat.GetShaderParam('u_test');"
		"sceneApi_param.SetValue(0.75);"
		"sceneApi_mat.InvalidateHash();"
		"sceneApi_mat");

	auto material = res.GetValue<Ref<Material>>();
	ASSERT_TRUE(material);

	auto param = DynamicCast<ShaderParamFloat>(material->GetShaderParam("u_test"));
	ASSERT_TRUE(param);
	EXPECT_FLOAT_EQ(param->GetValue(), 0.75f);
}

TEST(ScriptSceneApi, HeadlessFlagVisibleFromScript)
{
	auto res = EvalChecked("o2.Integration.IsHeadless()");
	EXPECT_TRUE(res.GetValue<bool>());
}

#endif // IS_SCRIPTING_SUPPORTED
