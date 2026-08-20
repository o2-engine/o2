#include "CodeToolCache.h"

#include <algorithm>

#include "Log.h"

SyntaxFile* CodeToolCache::AddFile(unique_ptr<SyntaxFile> file, bool original)
{
    SyntaxFile* raw = file.get();
    mOwnedFiles.push_back(std::move(file));

    files.push_back(raw);
    if (original)
        originalFiles.push_back(raw);

    return raw;
}

void CodeToolCache::UpdateGlobalNamespace()
{
    for (auto file : files)
    {
        SyntaxSection* fileGlobalNamespace = file->GetGlobalNamespace();

        globalNamespace.mVariables.insert(globalNamespace.mVariables.end(),
                                          fileGlobalNamespace->mVariables.begin(),
                                          fileGlobalNamespace->mVariables.end());

        globalNamespace.mFunctions.insert(globalNamespace.mFunctions.end(),
                                          fileGlobalNamespace->mFunctions.begin(),
                                          fileGlobalNamespace->mFunctions.end());

        globalNamespace.mTypedefs.insert(globalNamespace.mTypedefs.end(),
                                         fileGlobalNamespace->mTypedefs.begin(),
                                         fileGlobalNamespace->mTypedefs.end());

        globalNamespace.mUsingNamespaces.insert(globalNamespace.mUsingNamespaces.end(),
                                                fileGlobalNamespace->mUsingNamespaces.begin(),
                                                fileGlobalNamespace->mUsingNamespaces.end());

        globalNamespace.mEnums.insert(globalNamespace.mEnums.end(),
                                      fileGlobalNamespace->mEnums.begin(),
                                      fileGlobalNamespace->mEnums.end());

        for (auto childSection : fileGlobalNamespace->mSections)
            AppendSection(&globalNamespace, childSection);
    }

    ResolveDependencies(&globalNamespace);
    ResolveBaseClassDependencies(&globalNamespace);

    SyntaxClass* attributeClass = dynamic_cast<SyntaxClass*>(FindSection("o2::IAttribute"));
    SearchAttributes(&globalNamespace, attributeClass);
}

bool CodeToolCache::IsClassBasedOn(SyntaxClass* _class, SyntaxClass* baseClass)
{
    if (!_class || !baseClass)
        return false;

    if (_class->mSourceClass)
        _class = _class->mSourceClass;

    if (_class == baseClass)
        return true;

    for (auto baseClassDef : _class->GetBaseClasses())
        if (IsClassBasedOn(baseClassDef.GetClass(), baseClass))
            return true;

    return false;
}

SyntaxSection* CodeToolCache::FindSection(const string& fullName, bool withTypedefs /*= true*/)
{
    return FindSection(fullName, &globalNamespace, withTypedefs);
}

SyntaxSection* CodeToolCache::FindSection(const string& what, const string& where, bool withTypedefs /*= true*/)
{
    return FindSection(what, FindSection(where), withTypedefs);
}

SyntaxSection* CodeToolCache::FindSection(const string& what, SyntaxSection* where, bool withTypedefs /*= true*/)
{
    SyntaxSectionsVec passed;
    return FindSection(what, where, passed, withTypedefs);
}

SyntaxSection* CodeToolCache::FindSection(const string& what, SyntaxSection* where,
                                          SyntaxSectionsVec& processedSections, bool withTypedefs /*= true*/)
{
    if (!where)
        return nullptr;

    if (find(processedSections.begin(), processedSections.end(), where) != processedSections.end())
        return nullptr;

    processedSections.push_back(where);

    // Split leading name from A::B::C, respecting template brackets
    int braces = 0, trBraces = 0, sqBraces = 0;
    int delPos = -1;

    int whatLength = (int)what.length();
    for (int i = 0; i < whatLength - 1; i++)
    {
        switch (what[i])
        {
        case '(': braces++; break;
        case ')': braces--; break;
        case '<': trBraces++; break;
        case '>': trBraces--; break;
        case '[': sqBraces++; break;
        case ']': sqBraces--; break;
        }

        if (what[i] == ':' && what[i + 1] == ':' && braces == 0 && trBraces == 0 && sqBraces == 0)
        {
            delPos = i;
            break;
        }
    }

    string searchName;
    if (delPos < 0)
        searchName = what;
    else
        searchName = what.substr(0, delPos);

    int templatesPos = (int)searchName.find('<');
    if (templatesPos != (int)string::npos)
        searchName.erase(templatesPos);

    for (auto child : where->mSections)
    {
        if (child->mName != searchName)
            continue;

        if (delPos < 0)
        {
            if (templatesPos >= 0)
            {
                // Template specialization: register a proxy class that redirects to the source
                SyntaxClass* newSpecializedClass = mArena.Make<SyntaxClass>();
                newSpecializedClass->mName = what;
                if (child->mParentSection)
                {
                    newSpecializedClass->mFullName = child->mParentSection->mFullName + "::" + what;
                    child->mParentSection->mSections.push_back(newSpecializedClass);
                }
                else
                    newSpecializedClass->mFullName = what;

                newSpecializedClass->mParentSection = child->mParentSection;
                newSpecializedClass->mSourceClass = (SyntaxClass*)child;

                return newSpecializedClass;
            }

            return child;
        }

        if (auto res = FindSection(what.substr(delPos + 2), child, processedSections))
            return res;
    }

    if (auto res = FindSection(what, where->mParentSection, processedSections))
        return res;

    if (where->IsClass())
    {
        SyntaxClass* whereClass = dynamic_cast<SyntaxClass*>(where);
        for (auto& baseClass : whereClass->mBaseClasses)
        {
            if (baseClass.GetClass())
            {
                if (auto res = FindSection(what, baseClass.GetClass(), processedSections))
                    return res;
            }
        }
    }

    if (withTypedefs)
    {
        for (auto tdef : where->mTypedefs)
        {
            if (tdef->GetNewDefName() != searchName)
                continue;

            if (delPos < 0)
                return tdef->GetWhat();

            if (auto res = FindSection(what.substr(delPos + 2), tdef->GetWhat(), processedSections))
                return res;
        }

        for (auto nspace : where->mUsingNamespaces)
        {
            if (auto res = FindSection(what, nspace->GetUsingNamespace(), processedSections))
                return res;
        }
    }

    return nullptr;
}

void CodeToolCache::SearchAttributes(SyntaxSection* section, SyntaxClass* attributeClass)
{
    for (auto childSection : section->mSections)
    {
        if (childSection->IsClass())
        {
            SyntaxClass* childClass = dynamic_cast<SyntaxClass*>(childSection);
            if (IsClassBasedOn(childClass, attributeClass))
                attributes.push_back(childClass);
        }

        SearchAttributes(childSection, attributeClass);
    }
}

void CodeToolCache::Save(const string& file) const
{
    pugi::xml_document doc;

    pugi::xml_node filesNode = doc.append_child("files");
    for (auto syntaxFile : originalFiles)
    {
        auto fileNode = filesNode.append_child("file");
        syntaxFile->SaveTo(fileNode);
    }

    pugi::xml_node parentProjsNode = doc.append_child("parentProjects");
    for (auto& proj : parentProjects)
        parentProjsNode.append_child("project").append_attribute("path") = proj.c_str();

    doc.save_file(file.c_str());
}

void CodeToolCache::Load(const string& file, bool original /*= true*/)
{
    string normalized = file;
    for (auto& c : normalized)
    {
        if (c == '\\')
            c = '/';
    }

    if (!mLoadedCaches.insert(normalized).second)
        return;

    pugi::xml_document doc;
    doc.load_file(file.c_str());

    pugi::xml_node filesNode = doc.child("files");
    for (auto fileNode : filesNode)
    {
        auto newFile = make_unique<SyntaxFile>();
        newFile->LoadFrom(fileNode);
        AddFile(std::move(newFile), original);
    }

    if (original)
    {
        for (auto& parent : parentProjects)
            Load(parent, false);
    }
    else
    {
        pugi::xml_node parentProjsNode = doc.child("parentProjects");
        for (auto projectNode : parentProjsNode)
        {
            string path = projectNode.attribute("path").as_string();
            parentProjects.push_back(path);
            Load(path, false);
        }
    }
}

void CodeToolCache::AppendSection(SyntaxSection* currentSection, SyntaxSection* newSection)
{
    if (newSection->IsClass())
    {
        SyntaxClass* childClass = (SyntaxClass*)newSection;

        currentSection->mSections.push_back(childClass);
        childClass->mParentSection = currentSection;

        auto oldSections = newSection->mSections;
        newSection->mSections.clear();
        for (auto childSection : oldSections)
            AppendSection(childClass, childSection);
    }
    else
    {
        // Namespaces with the same name are merged into one
        SyntaxSection* childNamespace = nullptr;

        auto fnd = find_if(currentSection->mSections.begin(), currentSection->mSections.end(),
                           [=](SyntaxSection* x) {
                               return !x->IsClass() && x->GetName() == newSection->GetName();
                           });

        if (fnd != currentSection->mSections.end())
            childNamespace = *fnd;

        if (!childNamespace)
        {
            childNamespace = mArena.Make<SyntaxNamespace>();
            childNamespace->mName = newSection->mName;
            childNamespace->mFullName = newSection->mFullName;

            currentSection->mSections.push_back(childNamespace);
            childNamespace->mParentSection = currentSection;
        }

        childNamespace->mVariables.insert(childNamespace->mVariables.end(),
                                          newSection->mVariables.begin(),
                                          newSection->mVariables.end());

        childNamespace->mFunctions.insert(childNamespace->mFunctions.end(),
                                          newSection->mFunctions.begin(),
                                          newSection->mFunctions.end());

        childNamespace->mTypedefs.insert(childNamespace->mTypedefs.end(),
                                         newSection->mTypedefs.begin(),
                                         newSection->mTypedefs.end());

        childNamespace->mUsingNamespaces.insert(childNamespace->mUsingNamespaces.end(),
                                                newSection->mUsingNamespaces.begin(),
                                                newSection->mUsingNamespaces.end());

        childNamespace->mEnums.insert(childNamespace->mEnums.end(),
                                      newSection->mEnums.begin(),
                                      newSection->mEnums.end());

        for (auto childSection : newSection->mSections)
            AppendSection(childNamespace, childSection);
    }
}

void CodeToolCache::ResolveDependencies(SyntaxSection* section)
{
    for (auto tdef : section->mTypedefs)
    {
        tdef->mWhatSection = FindSection(tdef->mWhatName, section);

        if (!tdef->mWhatSection)
            VerboseLog("Not found section for typedef: %s\n", tdef->mWhatName.c_str());
    }

    for (auto nspace : section->mUsingNamespaces)
    {
        nspace->mUsingNamespace = FindSection(nspace->mUsingNamespaceName);

        if (!nspace->mUsingNamespace)
            VerboseLog("Not found section for using namespace: %s\n", nspace->mUsingNamespaceName.c_str());
    }

    auto sections = section->mSections;
    for (auto childSection : sections)
        ResolveDependencies(childSection);
}

void CodeToolCache::ResolveBaseClassDependencies(SyntaxSection* section)
{
    if (section->IsClass())
    {
        SyntaxClass* cls = (SyntaxClass*)section;
        for (auto& baseClass : cls->mBaseClasses)
        {
            baseClass.mClass = (SyntaxClass*)FindSection(baseClass.mClassName, section);

            if (!baseClass.mClass)
                VerboseLog("Not found base class: %s\n", baseClass.mClassName.c_str());
        }
    }

    auto sections = section->mSections;
    for (auto childSection : sections)
        ResolveBaseClassDependencies(childSection);
}
