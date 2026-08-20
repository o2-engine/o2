#include "CodeToolTestUtils.h"

using namespace codetool_tests;

// End-to-end characterization: run the tool over fixture sources and compare every
// generated file with the committed expected output. The tool is run twice because the
// first run creates missing .cpp files which only get scanned for registrators on the
// next run (matching how real builds reach steady state).
TEST(CodeToolGenerator, BasicProject)
{
    fs::path dir = MakeTempDir("BasicProject");
    CopyDirContents(FixturesDir() / "Basic", dir);

    RunCodeTool("TestProj", dir);
    RunCodeTool("TestProj", dir);

    CompareWithExpected(dir, FixturesDir() / "Basic.expected");
}

TEST(CodeToolGenerator, GenerationIsIdempotent)
{
    fs::path dir = MakeTempDir("Idempotent");
    CopyDirContents(FixturesDir() / "Basic", dir);

    RunCodeTool("TestProj", dir);
    RunCodeTool("TestProj", dir);

    std::map<std::string, std::string> snapshot;
    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (entry.is_regular_file())
            snapshot[entry.path().filename().string()] = ReadTextFile(entry.path());
    }

    RunCodeTool("TestProj", dir);

    for (const auto& [name, data] : snapshot)
    {
        if (name == "CodeToolCache.xml")
            continue;

        EXPECT_EQ(data, ReadTextFile(dir / name)) << "File changed on repeated run: " << name;
    }
}

TEST(CodeToolGenerator, ChildProjectUsesParentCache)
{
    fs::path parentDir = MakeTempDir("ChildProject_parent");
    CopyDirContents(FixturesDir() / "Basic", parentDir);
    RunCodeTool("TestProj", parentDir);
    RunCodeTool("TestProj", parentDir);

    fs::path childDir = MakeTempDir("ChildProject_child");
    CopyDirContents(FixturesDir() / "Child", childDir);
    RunCodeTool("ChildProj", childDir, { (parentDir / "CodeToolCache.xml").string() });
    RunCodeTool("ChildProj", childDir, { (parentDir / "CodeToolCache.xml").string() });

    CompareWithExpected(childDir, FixturesDir() / "Child.expected");
}
