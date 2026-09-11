#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Assets/AssetsTree.h"
#include "o2/Assets/Types/DataAsset.h"
#include "o2/Scripts/ScriptEngine.h"
#include "o2/Scripts/ScriptValue.h"
#include "o2/Utils/FileSystem/FileSystem.h"

#include <chrono>
#include <filesystem>
#include <fstream>

using namespace o2;

#if IS_SCRIPTING_SUPPORTED

namespace
{
	namespace fs = std::filesystem;

	ScriptValue EvalChecked(const String& code)
	{
		ScriptValue res = o2Scripts.Eval(code);
		EXPECT_NE(res.GetValueType(), ScriptValue::ValueType::Error) << res.GetError().Data();
		return res;
	}

	// Points the built assets of the main tree to a temporary folder for the test's lifetime
	struct TempBuiltAssets
	{
		fs::path dir;
		String previous;

		TempBuiltAssets()
		{
			UID uid;
			uid.Randomize();
			dir = fs::temp_directory_path() / ("o2test_include_" + std::string((String)uid));
			fs::create_directories(dir);

			auto& tree = const_cast<AssetsTree&>(o2Assets.GetAssetsTree());
			previous = tree.builtAssetsPath;
			String path(dir.string().c_str());
			path.ReplaceAll("\\", "/");
			tree.builtAssetsPath = path + "/";
		}

		~TempBuiltAssets()
		{
			const_cast<AssetsTree&>(o2Assets.GetAssetsTree()).builtAssetsPath = previous;
			std::error_code ec;
			fs::remove_all(dir, ec);
		}

		void Write(const std::string& name, const std::string& text)
		{
			fs::create_directories((dir / name).parent_path());
			std::ofstream(dir / name) << text;
		}
	};
}

TEST(ScriptInclude, RunsLibraryOnceUntilTheFileChanges)
{
	TempBuiltAssets assets;
	assets.Write("Lib/Counter.js", "includeTest_runs = (typeof includeTest_runs === 'number' ? includeTest_runs : 0) + 1;");

	EXPECT_TRUE(EvalChecked("include('Lib/Counter.js')").GetValue<bool>());
	EXPECT_TRUE(EvalChecked("include('Lib/Counter.js')").GetValue<bool>());
	EXPECT_EQ(EvalChecked("includeTest_runs").GetValue<int>(), 1);

	auto file = assets.dir / "Lib/Counter.js";
	fs::last_write_time(file, fs::last_write_time(file) + std::chrono::seconds(5));

	EXPECT_TRUE(EvalChecked("include('Lib/Counter.js')").GetValue<bool>());
	EXPECT_EQ(EvalChecked("includeTest_runs").GetValue<int>(), 2);
}

TEST(ScriptInclude, LibrariesIncludingEachOtherDoNotLoop)
{
	TempBuiltAssets assets;
	assets.Write("A.js", "include('B.js'); includeTest_a = 1;");
	assets.Write("B.js", "include('A.js'); includeTest_b = 2;");

	EXPECT_TRUE(EvalChecked("include('A.js')").GetValue<bool>());
	EXPECT_EQ(EvalChecked("includeTest_a + includeTest_b").GetValue<int>(), 3);
}

TEST(ScriptInclude, MissingOrBrokenFileReturnsFalse)
{
	TempBuiltAssets assets;
	assets.Write("Broken.js", "var = ;");

	EXPECT_FALSE(EvalChecked("include('Missing.js')").GetValue<bool>());
	EXPECT_FALSE(EvalChecked("include('Broken.js')").GetValue<bool>());
}

TEST(ScriptFileSystem, WritesReadsAndChecksFiles)
{
	TempBuiltAssets assets;
	String path = String((assets.dir / "progress.json").string().c_str()).ReplacedAll("\\", "/");
	o2Scripts.GetGlobal().SetProperty("fileSystemTest_path", ScriptValue(path));

	EXPECT_FALSE(EvalChecked("o2.FileSystem.IsFileExist(fileSystemTest_path)").GetValue<bool>());
	EXPECT_TRUE(EvalChecked("o2.FileSystem.ReadFile(fileSystemTest_path) === undefined").GetValue<bool>());

	EXPECT_TRUE(EvalChecked("o2.FileSystem.WriteFile(fileSystemTest_path, JSON.stringify({ level: 3 }))").GetValue<bool>());
	EXPECT_TRUE(EvalChecked("o2.FileSystem.IsFileExist(fileSystemTest_path)").GetValue<bool>());
	EXPECT_EQ(EvalChecked("JSON.parse(o2.FileSystem.ReadFile(fileSystemTest_path)).level").GetValue<int>(), 3);

	EXPECT_TRUE(EvalChecked("o2.FileSystem.FileDelete(fileSystemTest_path)").GetValue<bool>());
	EXPECT_FALSE(EvalChecked("o2.FileSystem.IsFileExist(fileSystemTest_path)").GetValue<bool>());
}

TEST(ScriptDataAsset, JsonReachesScripts)
{
	auto asset = mmake<DataAsset>();
	asset->data["levels"].AddElement()["moves"] = 12;
	asset->data["name"] = String("Кампания");

	o2Scripts.GetGlobal().SetProperty("dataAssetTest_asset", ScriptValue(asset));
	EXPECT_EQ(EvalChecked("JSON.parse(dataAssetTest_asset.GetJson()).levels[0].moves").GetValue<int>(), 12);
	EXPECT_EQ(EvalChecked("JSON.parse(dataAssetTest_asset.GetJson()).name").GetValue<String>(), String("Кампания"));
}

#endif // IS_SCRIPTING_SUPPORTED
