#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "CodeToolApp.h"

namespace codetool_tests
{

namespace fs = std::filesystem;

inline fs::path FixturesDir()
{
    return fs::path(CODETOOL_TESTS_DIR) / "Fixtures";
}

inline std::string ReadTextFile(const fs::path& path)
{
    std::ifstream fin(path);
    if (!fin.is_open())
        return std::string();

    std::stringstream ss;
    ss << fin.rdbuf();
    return ss.str();
}

inline void WriteTextFile(const fs::path& path, const std::string& data)
{
    std::ofstream fout(path);
    fout.write(data.c_str(), data.length());
}

// Creates a fresh empty temp directory for one test case
inline fs::path MakeTempDir(const std::string& name)
{
    fs::path dir = fs::temp_directory_path() / "o2CodeToolTests" / name;
    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
}

inline void CopyDirContents(const fs::path& from, const fs::path& to)
{
    fs::create_directories(to);
    fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
}

// Runs the tool in-process the same way CMake codegen targets invoke it
inline void RunCodeTool(const std::string& project, const fs::path& sources,
                        const std::vector<std::string>& parentCaches = {})
{
    std::vector<std::string> args = { "o2CodeTool", "-project", project, "-sources", sources.string() };

    if (!parentCaches.empty())
    {
        args.push_back("-parent_projects");
        for (auto& cache : parentCaches)
            args.push_back(cache);
    }

    std::vector<char*> argv;
    for (auto& arg : args)
        argv.push_back(arg.data());

    CodeToolApplication app;
    app.SetArguments(argv.data(), (int)argv.size());
    app.Process();
}

// Compares every file in expected dir against the same-named file in actual dir,
// and checks the actual dir introduces no extra sources
inline void CompareWithExpected(const fs::path& actualDir, const fs::path& expectedDir)
{
    for (const auto& entry : fs::directory_iterator(expectedDir))
    {
        if (!entry.is_regular_file())
            continue;

        fs::path actualFile = actualDir / entry.path().filename();
        ASSERT_TRUE(fs::exists(actualFile)) << "Missing generated file: " << actualFile;

        EXPECT_EQ(ReadTextFile(entry.path()), ReadTextFile(actualFile))
            << "Generated file differs: " << actualFile;
    }

    for (const auto& entry : fs::directory_iterator(actualDir))
    {
        if (!entry.is_regular_file())
            continue;

        std::string ext = entry.path().extension().string();
        if (ext != ".h" && ext != ".cpp")
            continue;

        EXPECT_TRUE(fs::exists(expectedDir / entry.path().filename()))
            << "Unexpected generated file: " << entry.path();
    }
}

// Parses a source string and returns the syntax tree
inline std::unique_ptr<SyntaxFile> ParseString(const std::string& source)
{
    auto file = std::make_unique<SyntaxFile>();
    CppSyntaxParser parser;
    parser.ParseSource(*file, "test.h", source);
    return file;
}

// Finds a section (class or namespace) by full name in parsed file, depth-first
inline SyntaxSection* FindSection(SyntaxFile& file, const std::string& fullName)
{
    for (auto section : file.GetGlobalNamespace()->GetAllSections())
    {
        if (section->GetFullName() == fullName)
            return section;
    }
    return nullptr;
}

inline SyntaxClass* FindClass(SyntaxFile& file, const std::string& fullName)
{
    return dynamic_cast<SyntaxClass*>(FindSection(file, fullName));
}

inline SyntaxVariable* FindVariable(SyntaxSection* section, const std::string& name)
{
    if (!section)
        return nullptr;

    for (auto variable : section->GetVariables())
    {
        if (variable->GetName() == name)
            return variable;
    }
    return nullptr;
}

inline SyntaxFunction* FindFunction(SyntaxSection* section, const std::string& name)
{
    if (!section)
        return nullptr;

    for (auto function : section->GetFunctions())
    {
        if (function->GetName() == name)
            return function;
    }
    return nullptr;
}

inline SyntaxEnum* FindEnum(SyntaxFile& file, const std::string& fullName)
{
    for (auto enm : file.GetGlobalNamespace()->GetAllEnums())
    {
        if (enm->GetFullName() == fullName)
            return enm;
    }
    return nullptr;
}

}
