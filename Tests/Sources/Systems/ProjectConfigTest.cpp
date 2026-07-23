#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "o2/Config/ProjectConfig.h"
#include "o2/Utils/Types/String.h"

using namespace o2;
namespace fs = std::filesystem;

// Save() must never clobber the project settings file: a process that failed to
// load it (wrong cwd, file being rewritten by a parallel test process) used to
// write pure defaults over the real settings on exit.

namespace
{
	const char* kTempPath = "TestTemp/ProjectConfigTest.json";

	// Restores the shared o2Config: values and the loaded-state snapshot from the real file
	struct ConfigGuard
	{
		PhysicsConfig physics;
		String projectName;

		ConfigGuard():
			physics(o2Config.physics), projectName(o2Config.GetProjectName())
		{
			fs::create_directories("TestTemp");
			fs::remove(kTempPath);
		}

		~ConfigGuard()
		{
			o2Config.physics = physics;
			o2Config.SetProjectName(projectName);
			o2Config.Load();
			fs::remove(kTempPath);
		}
	};

	void WriteTempConfig(float scale)
	{
		std::ofstream out(kTempPath);
		out << "{ \"physics\": { \"scale\": " << scale << " } }";
	}
}

TEST(ProjectConfig, SaveAfterFailedLoadWithDefaultsDoesNotCreateFile)
{
	ConfigGuard guard;

	o2Config.physics = PhysicsConfig();
	o2Config.SetProjectName("");
	o2Config.Load(kTempPath);

	EXPECT_TRUE(o2Config.IsDefault());

	o2Config.Save(kTempPath);
	EXPECT_FALSE(fs::exists(kTempPath)) << "defaults after a failed load must not be written";
}

TEST(ProjectConfig, SaveWithoutChangesDoesNotRewriteFile)
{
	ConfigGuard guard;

	WriteTempConfig(33.0f);
	o2Config.Load(kTempPath);
	EXPECT_FLOAT_EQ(o2Config.physics.scale, 33.0f);

	fs::remove(kTempPath);
	o2Config.Save(kTempPath);
	EXPECT_FALSE(fs::exists(kTempPath)) << "unchanged config must not rewrite the file";
}

TEST(ProjectConfig, SaveWritesChangedValues)
{
	ConfigGuard guard;

	WriteTempConfig(33.0f);
	o2Config.Load(kTempPath);

	o2Config.physics.scale = 55.0f;
	o2Config.Save(kTempPath);
	ASSERT_TRUE(fs::exists(kTempPath));

	o2Config.physics.scale = 1.0f;
	o2Config.Load(kTempPath);
	EXPECT_FLOAT_EQ(o2Config.physics.scale, 55.0f);
}

// A fresh project has no settings file yet; edits made in the editor must still be saved
TEST(ProjectConfig, SaveCreatesFileWhenChangedAfterFailedLoad)
{
	ConfigGuard guard;

	o2Config.physics = PhysicsConfig();
	o2Config.SetProjectName("");
	o2Config.Load(kTempPath);

	o2Config.physics.scale = 77.0f;
	o2Config.Save(kTempPath);
	ASSERT_TRUE(fs::exists(kTempPath));

	o2Config.physics.scale = 1.0f;
	o2Config.Load(kTempPath);
	EXPECT_FLOAT_EQ(o2Config.physics.scale, 77.0f);
}

TEST(ProjectConfig, FailedLoadKeepsCurrentValues)
{
	ConfigGuard guard;

	o2Config.physics.scale = 42.0f;
	o2Config.Load(kTempPath);
	EXPECT_FLOAT_EQ(o2Config.physics.scale, 42.0f);
}
