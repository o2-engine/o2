#pragma once

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Singleton.h"
#include "o2Editor/Core/WindowsSystem/WindowsLayout.h"

using namespace o2;

namespace o2
{
	class Widget;
}

// Editor configuration access macros
#define o2EditorConfig EditorConfig::Instance()

namespace Editor
{
	class DockWindowPlace;

	// -------------------------
	// Application configuration
	// -------------------------
	class EditorConfig: public ISerializable, public Singleton<EditorConfig>
	{
	public:
		// Default constructor. Loads data and applies to application 
		EditorConfig();

		// Destructor. Saves application configuration
		~EditorConfig();

		SERIALIZABLE(EditorConfig);

	public:
		class GlobalConfig: public ISerializable
		{
		public:
			WindowsLayout  mDefaultLayout; // Default windows layout, using in resetting @SERIALIZABLE

			Map<String, WindowsLayout> mAvailableLayouts; // Available windows layouts @SERIALIZABLE

			SERIALIZABLE(GlobalConfig);
		};

		class ProjectConfig: public ISerializable
		{
		public:
			Vec2I         mWindowSize = Vec2I(800, 600); // Application window size @SERIALIZABLE
			Vec2I         mWindowPosition;               // Application window position @SERIALIZABLE
			bool          mMaximized = true;             // Is application window is maximized @SERIALIZABLE
			WindowsLayout mLayout;                       // Windows layout @SERIALIZABLE

			SERIALIZABLE(ProjectConfig);
		};
	protected:
		String mConfigPath = "../../EditorConfig.json";
		String mGlobalConfigPath = "../../Config.json";

		bool          mConfigsLoaded = false; // True if configurations were loaded
		ProjectConfig mProjectConfig;         // Project editor config
		GlobalConfig  mGlobalConfig;          // Global editor config for all projects

	protected:
		// Saves global configs
		void SaveGlobalConfigs();

		// Saves project configs
		void SaveProjectConfigs();

		// Loads and applies configs
		void LoadConfigs();

		// Loads project configs
		void LoadProjectConfig();

		// Loads global configs
		void LoadGlobalConfig();

		// Updates window configs
		void OnWindowChange();

		friend class EditorApplication;
		friend class MenuPanel;
		friend class WindowsManager;
	};
}

CLASS_BASES_META(Editor::EditorConfig)
{
	BASE_CLASS(o2::ISerializable);
	BASE_CLASS(o2::Singleton<EditorConfig>);
}
END_META;
CLASS_FIELDS_META(Editor::EditorConfig)
{
	FIELD().PROTECTED().DEFAULT_VALUE("../../EditorConfig.json").NAME(mConfigPath);
	FIELD().PROTECTED().DEFAULT_VALUE("../../Config.json").NAME(mGlobalConfigPath);
	FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mConfigsLoaded);
	FIELD().PROTECTED().NAME(mProjectConfig);
	FIELD().PROTECTED().NAME(mGlobalConfig);
}
END_META;
CLASS_METHODS_META(Editor::EditorConfig)
{

	FUNCTION().PUBLIC().CONSTRUCTOR();
	FUNCTION().PROTECTED().SIGNATURE(void, SaveGlobalConfigs);
	FUNCTION().PROTECTED().SIGNATURE(void, SaveProjectConfigs);
	FUNCTION().PROTECTED().SIGNATURE(void, LoadConfigs);
	FUNCTION().PROTECTED().SIGNATURE(void, LoadProjectConfig);
	FUNCTION().PROTECTED().SIGNATURE(void, LoadGlobalConfig);
	FUNCTION().PROTECTED().SIGNATURE(void, OnWindowChange);
}
END_META;

CLASS_BASES_META(Editor::EditorConfig::GlobalConfig)
{
	BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::EditorConfig::GlobalConfig)
{
	FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mDefaultLayout);
	FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mAvailableLayouts);
}
END_META;
CLASS_METHODS_META(Editor::EditorConfig::GlobalConfig)
{
}
END_META;

CLASS_BASES_META(Editor::EditorConfig::ProjectConfig)
{
	BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::EditorConfig::ProjectConfig)
{
	FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Vec2I(800, 600)).NAME(mWindowSize);
	FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mWindowPosition);
	FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mMaximized);
	FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mLayout);
}
END_META;
CLASS_METHODS_META(Editor::EditorConfig::ProjectConfig)
{
}
END_META;
