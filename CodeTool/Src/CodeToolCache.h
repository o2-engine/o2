#pragma once

#include <set>
#include <string>
#include <vector>

#include "SyntaxTree.h"

// Parsed type graph of the project plus its parent projects. Saved to XML next to the
// project sources so dependent projects can resolve base classes across project borders
class CodeToolCache
{
public:
    SyntaxFilesVec   files;           // All syntax files list, including parent projects
    SyntaxFilesVec   originalFiles;   // Syntax files of this project only
    SyntaxSection    globalNamespace; // Global syntax namespace, merged from all files
    SyntaxClassesVec attributes;      // All attribute classes (derived from o2::IAttribute)
    vector<string>   parentProjects;  // Parent projects' cache files used by this project

    // Takes ownership of a parsed file and adds it to the lists
    SyntaxFile* AddFile(unique_ptr<SyntaxFile> file, bool original);

    // Merges all files into the global namespace and resolves typedefs, usings,
    // base classes and attribute classes
    void UpdateGlobalNamespace();

    // Returns is class based on other class
    bool IsClassBasedOn(SyntaxClass* _class, SyntaxClass* baseClass);

    // Returns section by name in global space
    SyntaxSection* FindSection(const string& fullName, bool withTypedefs = true);

    // Returns section by name in where
    SyntaxSection* FindSection(const string& what, const string& where, bool withTypedefs = true);

    // Returns section by name in where
    SyntaxSection* FindSection(const string& what, SyntaxSection* where, bool withTypedefs = true);

    // Saves data to file
    void Save(const string& file) const;

    // Loads data from file; parent caches are loaded recursively
    void Load(const string& file, bool original = true);

protected:
    set<string> mLoadedCaches; // Normalized paths of already loaded caches, guards diamond parents from double-loading

    vector<unique_ptr<SyntaxFile>> mOwnedFiles; // Owns all files: parsed ones and those loaded from parent caches

    SyntaxNodeArena mArena; // Owns nodes created by the cache itself: merged namespaces and specialization proxies

protected:
    void AppendSection(SyntaxSection* currentSection, SyntaxSection* newSection);
    void ResolveDependencies(SyntaxSection* section);
    void ResolveBaseClassDependencies(SyntaxSection* section);
    SyntaxSection* FindSection(const string& what, SyntaxSection* where, SyntaxSectionsVec& processedSections,
                               bool withTypedefs = true);
    void SearchAttributes(SyntaxSection* section, SyntaxClass* attributeClass);
};
