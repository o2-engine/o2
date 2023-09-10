#include "o2Editor/stdafx.h"
#include "EditorApplication.h"

#include "o2/Animation/Tracks/AnimationColor4Track.h"
#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Animation/Tracks/AnimationVec2FTrack.h"
#include "o2/Application/Input.h"
#include "o2/Assets/Assets.h"
#include "o2/Events/EventSystem.h"
#include "o2/Render/Render.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/ImageComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/MenuPanel.h"
#include "o2/Scene/UI/Widgets/MenuPanel.h"
#include "o2/Scripts/ScriptEngine.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Function/ActorSubscription.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2/Utils/System/Time/Timer.h"
#include "o2/Utils/Tasks/TaskManager.h"
#include "o2Editor/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Core/Actions/IAction.h"
#include "o2Editor/Core/Actions/PropertyChange.h"
#include "o2Editor/Core/MenuPanel.h"
#include "o2Editor/Core/Properties/Properties.h"
#include "o2Editor/Core/ToolsPanel.h"
#include "o2Editor/Core/UIRoot.h"
#include "o2Editor/Core/UIStyle/EditorUIStyle.h"
#include "o2Editor/Core/WindowsSystem/WindowsManager.h"
#include "o2Editor/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"
#include "o2Editor/TreeWindow/TreeWindow.h"
#include "o2Editor/Utils/CommonTextures.h"

namespace Editor
{
	EditorApplication::EditorApplication()
	{
	}

	EditorApplication::~EditorApplication()
	{
	}

	const String& EditorApplication::GetLoadedSceneName() const
	{
		if (mLoadedScene)
			return mLoadedScene->GetPath();

		return String::empty;
	}

	void EditorApplication::LoadScene(const SceneAssetRef& scene)
	{
		ForcePopEditorScopeOnStack scope;

		if (scene)
			scene->Load();

		mLoadedScene = scene;

		ResetUndoActions();

		if (mLoadedScene)
			o2EditorConfig.projectConfig.mLastLoadedScene = mLoadedScene->GetPath();
	}

	void EditorApplication::SaveScene()
	{
		o2Scene.Save(mLoadedScene->GetFullPath());
		mLoadedScene->Save();

		if (mLoadedScene)
			o2EditorConfig.projectConfig.mLastLoadedScene = mLoadedScene->GetPath();
	}

	void EditorApplication::SaveSceneAs(const String& path)
	{
		String relativePath = o2FileSystem.GetPathRelativeToPath(path, ::GetAssetsPath());

		o2Scene.Save(path);

		mLoadedScene = SceneAssetRef::CreateAsset();
		mLoadedScene->SetPath(relativePath);
		mLoadedScene->Save();

		if (mLoadedScene)
			o2EditorConfig.projectConfig.mLastLoadedScene = mLoadedScene->GetPath();
	}

	void EditorApplication::MakeNewScene()
	{
		mLoadedScene = nullptr;
		o2Scene.Clear();

		ResetUndoActions();
		o2EditorPropertiesWindow.ResetTargets();
	}

	bool EditorApplication::IsSceneChanged() const
	{
		return GetUndoActionsCount() > 0;
	}

	void EditorApplication::SetPlaying(bool playing)
	{
		if (playing == mIsPlaying)
			return;

		mPlayingChanged = true;
		mIsPlaying = playing;
	}

	bool EditorApplication::IsPlaying() const
	{
		return mIsPlaying;
	}

	void EditorApplication::OnStarted()
	{
		PushEditorScopeOnStack enterScope;

		o2Application.SetWindowCaption("o2 Editor");

		mUIRoot = mnew UIRoot();

		mBackground = mnew Sprite("ui/UI4_Background.png");
		mBackSign = mnew Sprite("ui/UI4_o2_sign.png");

		CommonTextures::Initialize();

		mConfig = mnew EditorConfig();
		mConfig->LoadConfigs();

		LoadUIStyle();

		mProperties = mnew Properties();
		mWindowsManager = mnew WindowsManager();
		mMenuPanel = mnew MenuPanel();
		mToolsPanel = mnew ToolsPanel();

		if (mConfig->projectConfig.mMaximized)
			o2Application.Maximize();
		else
		{
			Vec2I pos = mConfig->projectConfig.mWindowPosition;
			o2Application.SetWindowSize(mConfig->projectConfig.mWindowSize);
			o2Application.SetWindowPosition(pos);
			mConfig->projectConfig.mWindowPosition = pos;
		}

		OnResizing();

		o2EditorApplication.LoadScene(SceneAssetRef(o2EditorConfig.projectConfig.mLastLoadedScene));

		//FreeConsole();

		auto widget = EditorUIRoot.GetRootWidget()->GetChildWidget("tools panel/play panel");
		o2EditorAnimationWindow.SetAnimation(&widget->GetStateObject("playing")->GetAnimationClip(),
											 &widget->GetStateObject("playing")->player);

		o2EditorAnimationWindow.SetTarget(widget);

		o2Scripts.CollectGarbage();

		//o2Scripts.GetGlobal().SetProperty("widget", mnew Widget());

		// 		ScriptValue tmp(Vector<int>({ 0, 1, 2, 3, 4 }));
		// 
		// 		auto typ = tmp.GetValueType();
		// 		o2Scripts.GetGlobal().SetProperty("tmp", tmp);
		// 		o2Scripts.Eval("print(JSON.stringify(tmp));");
		// 		auto tmp0str = tmp.GetElement(1).ToString();
		// 		o2Debug.Log(tmp0str);

				// 		ScriptValue tttConstruct;
				// 		tttConstruct.SetThisFunction(Function<void(ScriptValue, int)>([](ScriptValue xx, int x) { xx.SetProperty("x", x); }));
				// 		o2Scripts.GetGlobal().SetProperty("ttt", tttConstruct);
				// 
				// 		o2Scripts.Eval("let xttt = new ttt(5); print(JSON.stringify(xttt));");
						// 
						//float testValue = 0;
						//o2Scripts.GetGlobal().SetProperty("back", mBackground);
				// 		o2Scripts.GetGlobal().SetPropertyWrapper<float>(ScriptValue("test"),
				// 			Function<void(float)>([&](float xx) { testValue = xx; }),
				// 			Function<float()>([&]() { return testValue; }));
				// 
				// 		auto tp = o2Scripts.GetGlobal().GetProperty("test").GetValueType();
				// 
				// 		o2Scripts.Eval("print('test is ' + test + ', and its type is ' + typeof test);");

						//auto filename = GetAssetsPath() + String("test.js");
						//o2Scripts.Run(o2Scripts.Parse(o2FileSystem.ReadFile(filename), filename));

						//o2Debug.LogStr("---Dump global---\n" + o2Scripts.GetGlobal().Dump() + "\n---------------");

				// 		o2Scripts.Eval("let bbn = new Vec2(3, 5); print(Vec2.prototype); print(Dump(Vec2.prototype));");
				// 
				// 		ScriptValue tmp;
				// 		tmp.Accept(jerry_object_get_property_names(o2Scripts.GetGlobal().jvalue, JERRY_PROPERTY_FILTER_ALL));
				// 		o2Debug.Log(tmp.Dump());
				// 
				// 		ScriptValue v(Vec2F(3, 2));
				// 		o2Scripts.GetGlobal().SetProperty("v", v);
				// 		o2Scripts.Eval("print(v.Length());");
				// 
				// 		o2Scripts.GetGlobal().SetProperty("back", mBackground);
				// 
				// 		o2Debug.Log(o2Scripts.Eval("Vec2.prototype").Dump());

				// 		o2Scripts.GetGlobal().SetProperty("widget", ScriptValue(*widget));
				// 		o2Scripts.Eval("widget.layout.position = Vec2.New(2, 2); print(JSON.stringify(widget.GetChildren(), null, 2));");
				// 
				// 		o2Scripts.GetGlobal().SetProperty("array", ScriptValue(Vector<int>({ 0, 1, 2, 3, 4 ,5 })));
				// 		o2Scripts.Eval("print(JSON.stringify(array, null, 2))");
				// 
				// 		Vector<int> arr = o2Scripts.GetGlobal().GetProperty("array");
				// 		o2Scripts.Eval("print('hello world!');");
				// 		o2Scripts.Eval("var x = 5 + 5; function myf(a, b) { print(a); print(b); var sum = a + b; print(sum); return sum; }");
				// 		ScriptValue v2(Vec2F(3, 4));
				// 		o2Scripts.GetGlobal().SetProperty(ScriptValue("mm"), v2);
				// 		o2Scripts.Eval("print(JSON.stringify(mm));");
				// 		o2Scripts.CollectGarbage();
				// 		ScriptValue p(Function<float(int, float)>([](int a, float b) { o2Debug.Log("prived pidor " + (String)(a + b)); return a + b + 5; }));
				// 		o2Scripts.CollectGarbage();
				// 		o2Scripts.GetGlobal().SetProperty(ScriptValue("myfunc"), p);
				// 		o2Scripts.CollectGarbage();
				// 		o2Scripts.Eval("print(myfunc(1, 3.2));");
				// 		o2Scripts.CollectGarbage();
				// 
				// 		std::tuple<std::remove_reference<const String&>::type> t = { "as" };
				// 		auto fff = [](const String& x) { o2Debug.Log(x); };
				// 		std::apply(fff, t);
				// 		ScriptValue ff(Function<void(const String&)>([](const String& xx) { o2Debug.Log("called ff" + xx); }));
				// 		o2Scripts.GetGlobal().SetProperty(ScriptValue("ff"), ff);
				// 		o2Scripts.Eval("ff('asd');");
				// 
				// 		int testPtr = 2;
				// 		ScriptValue obj;
				//  		obj.SetPropertyWrapper(ScriptValue("ptrProp"), testPtr);
				//  		obj.SetPropertyWrapper<int>(ScriptValue("funcProp"), [](int v) { o2Debug.Log((String)v); }, []() { return 15; });
				// 		obj.SetProperty(ScriptValue("func"), ScriptValue(Function<float()>([]() { 
				// 			o2Debug.Log("ass"); 
				// 			return 5.0f;
				// 		})));
				// 		o2Scripts.GetGlobal().SetProperty(ScriptValue("obj"), obj);
				// 		o2Scripts.Eval("print(JSON.stringify(obj));");
				// 		o2Scripts.Eval("obj.ptrProp = obj.ptrProp + 5");
				// 		o2Scripts.Eval("obj.ptrProp = obj.func() + 5");
				// 		o2Scripts.Eval("obj.funcProp = obj.funcProp + 5");
				// 		o2Scripts.GetGlobal().SetProperty(ScriptValue("obj"), obj);
				// 		o2Scripts.Eval("print(JSON.stringify(obj));");
				// 
				// 		o2Scripts.GetGlobal().SetProperty(ScriptValue("scriptValueFunc"), ScriptValue(Function<ScriptValue()>([&]()
				// 		{
				// 			return mBackground->GetScriptValue();
				// 		})));
				// 
				// 		o2Debug.LogStr("---Dump global---\n" + o2Scripts.GetGlobal().Dump() + "\n---------------");
				// 
				// 		o2Scripts.Eval("print(JSON.stringify(scriptValueFunc()))");
				// 
				// 		o2Scripts.Eval("var testSprite = o2.Sprite.New('ui/UI4_Background.png'); testSprite.image = o2.RefImageAsset.New('ui/UI_Background.png');");
				// 
				// 		ScriptValue prot;
				// 		prot.SetProperty("a", 5);
				// 		prot.SetProperty("func", Function<void()>([]() { o2Debug.Log("privet"); }));
				// 
				// 		Vec2F tv2(3, 5);
				// 		o2Scripts.GetGlobal().SetProperty("gettt", Function<Vec2F(const Vec2F&)>([&](const Vec2F& in) { tv2 += in; return tv2; }));
				// 		o2Scripts.Eval("print(JSON.stringify(gettt(Vec2.New(2, 3))));");
				// 
				// 		ScriptValue exm = ScriptValue::EmptyObject();
				// 		exm.SetPrototype(prot);
				// 		o2Scripts.GetGlobal().SetProperty("exm", exm);
				// 		o2Scripts.Eval("exm.func();");
				// 
				// 		o2Scripts.Run(o2Scripts.Parse(o2FileSystem.ReadFile(GetAssetsPath() + String("test.js"))));
				// 
				// 		o2Scripts.Eval("yy.func();");
				// 
				// 		o2Scripts.Eval("function testDefault(x = 5) { print(Math.sqrt(x)); }; testDefault(); testDefault(10);");
				// 		o2Scripts.Eval("let vv = Vec2.New(3, 5); let gg = vv.Add(Vec2.New(1, 1)); print(gg.x + '; ' + gg.y);");
				// 
				// 		o2Debug.LogStr("---Dump mBackground---\n" + mBackground->GetScriptValue().Dump() + "\n---------------");
				// 
				// 		o2Scripts.GetGlobal().SetProperty(ScriptValue("wrp"), mBackground->GetScriptValue());
				// 		o2Scripts.Eval("wrp.Draw()");
				// 		o2Scripts.Eval("wrp.SetFill(wrp.GetFill() - 0.1)");
				// 		o2Scripts.Eval("wrp.position = wrp.position.Add(Vec2.New(2, 2));");
				// 		o2Scripts.Eval("print(wrp.GetImageName()); wrp.LoadFromImage('ui/UI_Background.png')");
				// 		auto pres = o2Scripts.Parse("vaddr str = 'Hello, World!';");
				// 		if (!pres.IsOk()) {
				// 			o2Debug.Log(pres.GetError());
				// 		}
				// 		else {
				// 			o2Scripts.Run(pres);
				// 		}
				// 
				// 		auto myfuncRef = o2Scripts.GetGlobal().GetProperty(ScriptValue("myfunc")).GetValue<Function<float(int, float)>>();
				// 		auto mffr = myfuncRef(2, 3.5f);
				// 
				// 		auto myfRef = o2Scripts.GetGlobal().GetProperty(ScriptValue("myf")).GetValue<Function<float(int, float)>>();
				// 		auto myfr = myfRef(2, 3.5f);
				// 
				// 		ScriptValue mb; mb.SetValue(*mBackground); mb.SetProperty(ScriptValue("a"), ScriptValue("b"));
				// 		ScriptValue xx = mb;
				// 		auto xxxx = o2Scripts.GetGlobal().GetProperty(ScriptValue("myf")).Invoke<float>(33, 56);
				// 		o2Scripts.GetGlobal().SetProperty(ScriptValue("myObj"), mb);
				// 		o2Scripts.Eval("print(JSON.stringify(myObj));");
				// 		o2Scripts.Eval("myObj.fill = 0.5");
				// 		o2Scripts.Eval("print(JSON.stringify(myObj));");
				// 
				// // 		{
				// // 			ScriptValue x([]() {
				// // 				o2Debug.Log("prived pidor");
				// // 			});
				// // 			x.Invoke();
				// // 
				// // 			ScriptValue y;
				// // 			y.SetValue([]() {
				// // 				o2Debug.Log("prived pidor ti");
				// // 			});
				// // 			y.Invoke();
				// // 		}
				// // 		o2Scripts.CollectGarbage();
				// // 		auto xx = x.ToString();
				//  		float res = o2Scripts.GetGlobal().GetProperty(ScriptValue("x")).ToNumber();


		o2Debug.LogStr("---Dump global---\n" + o2Scripts.GetGlobal().Dump() + "\n---------------");
	}

	void EditorApplication::OnClosing()
	{
		delete mConfig;
		delete mWindowsManager;
		delete mBackground;
		delete mBackSign;
		delete mToolsPanel;
		delete mMenuPanel;
		delete mUIRoot;
	}

	void EditorApplication::OnResizing()
	{
		mBackground->SetSize(o2Render.GetResolution() + Vec2F(20, 20));
		mBackSign->position = (Vec2F)(o2Render.GetResolution()).InvertedX() * 0.5f + Vec2F(40.0f, -85.0f);

		mConfig->OnWindowChange();
		mUIRoot->OnApplicationSized();
	}

	void EditorApplication::OnMoved()
	{
		mConfig->OnWindowChange();
	}

	void EditorApplication::ProcessFrame()
	{
		PushEditorScopeOnStack scope;

		mUpdateStep = mIsPlaying && (!isPaused || step);
		step = false;

		Application::ProcessFrame();

		mDrawCalls = mRender->GetDrawCallsCount();
		mDrawnPrimitives = mRender->GetDrawnPrimitives();
	}

	void EditorApplication::CheckPlayingSwitch()
	{
		if (!mPlayingChanged)
			return;

		ForcePopEditorScopeOnStack scope;

		if (mIsPlaying)
		{
			o2EditorSceneScreen.ClearSelection();

			mSceneDump.Clear();
			o2Scene.Save(mSceneDump);
			o2Scene.Load(mSceneDump);
		}
		else
		{
			o2EditorSceneScreen.ClearSelection();
			o2Scene.Load(mSceneDump);
		}

		mPlayingChanged = false;
	}

	void EditorApplication::LoadUIStyle()
	{
		EditorUIStyleBuilder builder;
		builder.RebuildEditorUIManager("Editor UI styles", true, true);
	}

	void EditorApplication::PreUpdatePhysics()
	{
		Application::PreUpdatePhysics();
	}

	void EditorApplication::UpdatePhysics(float dt)
	{
		if (mUpdateStep)
			Application::UpdatePhysics(dt);
	}

	void EditorApplication::PostUpdatePhysics()
	{
		if (mUpdateStep)
			Application::PostUpdatePhysics();
	}

	void EditorApplication::UpdateScene(float dt)
	{
		if (mUpdateStep)
		{
			mScene->Update(dt);
			o2EditorSceneScreen.NeedRedraw();
		}
		else
		{
			mScene->UpdateAddedEntities();
			mScene->UpdateDestroyingEntities();
		}

		CheckPlayingSwitch();
	}

	void EditorApplication::FixedUpdateScene(float dt)
	{
		if (mUpdateStep)
			mScene->FixedUpdate(dt);
	}

	void EditorApplication::DrawScene()
	{
	}

	void EditorApplication::DrawUIManager()
	{
		PushEditorScopeOnStack scope;
		Application::DrawUIManager();
	}

	void EditorApplication::DrawDebug()
	{
		o2Debug.Draw(true);
	}

	void EditorApplication::UpdateDebug(float dt)
	{
		if (mUpdateStep)
			o2Debug.Update(false, dt);

		o2Debug.Update(true, dt);
	}

	void EditorApplication::UpdateEventSystem()
	{
		PushEditorScopeOnStack scope;
		Application::UpdateEventSystem();
	}

	void EditorApplication::PostUpdateEventSystem()
	{
		PushEditorScopeOnStack scope;
		Application::PostUpdateEventSystem();
	}

	void EditorApplication::OnUpdate(float dt)
	{
		mWindowsManager->Update(dt);
		mUIRoot->Update(dt);
		mToolsPanel->Update(dt);

		String currentScene = mLoadedScene ? mLoadedScene->GetPath() : String("");

		o2Application.windowCaption = String("o2 Editor: ") + currentScene +
			"; FPS: " + (String)((int)o2Time.GetFPS()) +
			" DC: " + (String)mDrawCalls +
			" Tris: " + (String)mDrawnPrimitives +
			" Cursor: " + (String)o2Input.GetCursorPos() +
			" JS: " + (String)(o2Scripts.GetUsedMemory() / 1024) + "kb";

		if (o2Input.IsKeyPressed('K'))
			o2Memory.DumpInfo();
	}

#undef DrawText

	void EditorApplication::OnDraw()
	{
		PushEditorScopeOnStack scope;

		o2Render.Clear();

		mBackground->Draw();
		mBackSign->Draw();
		mWindowsManager->Draw();
		mUIRoot->Draw();

		// Debug draw undo actions
		if (o2Input.IsKeyDown(VK_F6))
		{
			for (int i = 0; i < mActions.Count(); i++)
				o2Debug.DrawText(Vec2F(0, (float)(20 * i)), (String)i + mActions[i]->GetName());
		}
	}

	void EditorApplication::OnActivated()
	{
		//o2Assets.RebuildAssets();
	}

	void EditorApplication::OnDeactivated()
	{
	}

}
