#include "SyntaxTree.h"

#include <algorithm>

SyntaxFile::SyntaxFile()
{
    mGlobalNamespace = mArena.Make<SyntaxNamespace>();
}

const string& SyntaxFile::GetPath() const
{
    return mPath;
}

const string& SyntaxFile::GetData() const
{
    return mData;
}

SyntaxNamespace* SyntaxFile::GetGlobalNamespace() const
{
    return mGlobalNamespace;
}

SyntaxNodeArena& SyntaxFile::GetArena()
{
    return mArena;
}

void SyntaxFile::SaveTo(pugi::xml_node& node) const
{
    node.append_attribute("path") = mPath.c_str();
    auto globalNamespace = node.append_child("globalNamespace");
    mGlobalNamespace->SaveTo(globalNamespace);
}

void SyntaxFile::LoadFrom(const pugi::xml_node& node)
{
    mPath = node.attribute("path").as_string();

    mGlobalNamespace = mArena.Make<SyntaxNamespace>();
    mGlobalNamespace->LoadFrom(node.child("globalNamespace"), mArena);
}

int ISyntaxExpression::GetBegin() const
{
    return mBegin;
}

int ISyntaxExpression::GetLength() const
{
    return mLength;
}

int ISyntaxExpression::GetEnd() const
{
    return mBegin + mLength;
}

int ISyntaxExpression::GetLine() const
{
    return mLine;
}

const string& ISyntaxExpression::GetData() const
{
    return mData;
}

SyntaxFile* ISyntaxExpression::GetOwnerFile() const
{
    return mFile;
}

SyntaxDefineIf* ISyntaxExpression::GetDefine() const
{
    return mDefine;
}

SyntaxComment* ISyntaxExpression::GetComment() const
{
    return mComment;
}

SyntaxAttributes* ISyntaxExpression::GetAttributesMacro() const
{
    return mAttributesMacro;
}

SyntaxSection* SyntaxSection::GetParentSection() const
{
    return mParentSection;
}

const string& SyntaxSection::GetName() const
{
    return mName;
}

const string& SyntaxSection::GetFullName() const
{
    return mFullName;
}

const SyntaxFunctionsVec& SyntaxSection::GetFunctions() const
{
    return mFunctions;
}

const SyntaxVariablesVec& SyntaxSection::GetVariables() const
{
    return mVariables;
}

const SyntaxSectionsVec& SyntaxSection::GetSections() const
{
    return mSections;
}

const SyntaxEnumsVec& SyntaxSection::GetEnums() const
{
    return mEnums;
}

const SyntaxTypedefsVec& SyntaxSection::GetTypedefs() const
{
    return mTypedefs;
}

const SyntaxUsingNamespacesVec& SyntaxSection::GetUsingNamespaces() const
{
    return mUsingNamespaces;
}

SyntaxSectionsVec SyntaxSection::GetAllSections() const
{
    SyntaxSectionsVec res = mSections;

    for (auto section : mSections)
    {
        for (auto nested : section->GetAllSections())
            res.push_back(nested);
    }

    return res;
}

SyntaxClassesVec SyntaxSection::GetAllClasses() const
{
    SyntaxClassesVec res;

    for (auto section : mSections)
    {
        if (section->IsClass())
            res.push_back(dynamic_cast<SyntaxClass*>(section));
    }

    for (auto section : mSections)
    {
        for (auto nested : section->GetAllClasses())
            res.push_back(nested);
    }

    return res;
}

SyntaxEnumsVec SyntaxSection::GetAllEnums() const
{
    SyntaxEnumsVec res = mEnums;

    for (auto section : mSections)
    {
        for (auto nested : section->GetAllEnums())
            res.push_back(nested);
    }

    return res;
}

bool SyntaxSection::IsClass() const
{
    return false;
}

void SyntaxSection::SaveTo(pugi::xml_node& node) const
{
    node.append_attribute("name") = mName.c_str();
    node.append_attribute("fullname") = mFullName.c_str();

    pugi::xml_node sectionsNode = node.append_child("sections");
    for (auto section : mSections)
    {
        auto sectionNode = sectionsNode.append_child("section");
        section->SaveTo(sectionNode);
    }

    pugi::xml_node typedefsNode = node.append_child("typedefs");
    for (auto typedefItem : mTypedefs)
    {
        auto typedefNode = typedefsNode.append_child("typedef");
        typedefItem->SaveTo(typedefNode);
    }

    pugi::xml_node usingsNode = node.append_child("usings");
    for (auto usingItem : mUsingNamespaces)
    {
        auto usingNode = usingsNode.append_child("using");
        usingItem->SaveTo(usingNode);
    }
}

void SyntaxSection::LoadFrom(const pugi::xml_node& node, SyntaxNodeArena& arena)
{
    mName = node.attribute("name").as_string();
    mFullName = node.attribute("fullname").as_string();

    for (auto sectionNode : node.child("sections"))
    {
        if (sectionNode.type() != pugi::node_element)
            continue;

        SyntaxSection* newSection;
        if (sectionNode.attribute("isClass"))
        {
            SyntaxClass* newClass = arena.Make<SyntaxClass>();
            newClass->LoadFrom(sectionNode, arena);
            newSection = newClass;
        }
        else
        {
            newSection = arena.Make<SyntaxNamespace>();
            newSection->LoadFrom(sectionNode, arena);
        }

        newSection->mParentSection = this;
        mSections.push_back(newSection);
    }

    for (auto typedefNode : node.child("typedefs"))
    {
        if (typedefNode.type() != pugi::node_element)
            continue;

        SyntaxTypedef* newTypedef = arena.Make<SyntaxTypedef>();
        newTypedef->LoadFrom(typedefNode);
        mTypedefs.push_back(newTypedef);
    }

    for (auto usingNode : node.child("usings"))
    {
        if (usingNode.type() != pugi::node_element)
            continue;

        SyntaxUsingNamespace* newUsing = arena.Make<SyntaxUsingNamespace>();
        newUsing->LoadFrom(usingNode);
        mUsingNamespaces.push_back(newUsing);
    }
}

const SyntaxFunctionsVec& SyntaxClass::GetFunctions() const
{
    if (mSourceClass)
        return mSourceClass->GetFunctions();

    return SyntaxSection::GetFunctions();
}

const SyntaxVariablesVec& SyntaxClass::GetVariables() const
{
    if (mSourceClass)
        return mSourceClass->GetVariables();

    return SyntaxSection::GetVariables();
}

const SyntaxSectionsVec& SyntaxClass::GetSections() const
{
    if (mSourceClass)
        return mSourceClass->GetSections();

    return SyntaxSection::GetSections();
}

const SyntaxEnumsVec& SyntaxClass::GetEnums() const
{
    if (mSourceClass)
        return mSourceClass->GetEnums();

    return SyntaxSection::GetEnums();
}

const SyntaxTypedefsVec& SyntaxClass::GetTypedefs() const
{
    if (mSourceClass)
        return mSourceClass->GetTypedefs();

    return SyntaxSection::GetTypedefs();
}

const SyntaxUsingNamespacesVec& SyntaxClass::GetUsingNamespaces() const
{
    if (mSourceClass)
        return mSourceClass->GetUsingNamespaces();

    return SyntaxSection::GetUsingNamespaces();
}

SyntaxSectionsVec SyntaxClass::GetAllSections() const
{
    if (mSourceClass)
        return mSourceClass->GetAllSections();

    return SyntaxSection::GetAllSections();
}

SyntaxClassesVec SyntaxClass::GetAllClasses() const
{
    if (mSourceClass)
        return mSourceClass->GetAllClasses();

    return SyntaxSection::GetAllClasses();
}

bool SyntaxClass::IsClass() const
{
    return true;
}

const SyntaxClassInheritancesVec& SyntaxClass::GetBaseClasses() const
{
    return mBaseClasses;
}

bool SyntaxClass::IsTemplate() const
{
    if (!mTemplateParameters.empty())
        return true;

    if (mParentSection)
    {
        SyntaxClass* parentClass = dynamic_cast<SyntaxClass*>(mParentSection);
        return parentClass && parentClass->IsTemplate();
    }

    return false;
}

const string& SyntaxClass::GetTemplateParameters() const
{
    return mTemplateParameters;
}

SyntaxProtectionSection SyntaxClass::GetClassSection() const
{
    return mClassSection;
}

const string& SyntaxClass::GetAttributeCommentDef() const
{
    return mAttributeCommentDef;
}

const string& SyntaxClass::GetAttributeShortDef() const
{
    return mAttributeShortDef;
}

void SyntaxClass::SaveTo(pugi::xml_node& node) const
{
    SyntaxSection::SaveTo(node);

    node.append_attribute("isClass") = true;
    node.append_attribute("templates") = mTemplateParameters.c_str();
    node.append_attribute("protection") = (int)mClassSection;
    node.append_attribute("attributeCommentDef") = mAttributeCommentDef.c_str();
    node.append_attribute("attributeShortDef") = mAttributeShortDef.c_str();

    pugi::xml_node baseClassesNode = node.append_child("baseClasses");
    for (auto& baseClass : mBaseClasses)
    {
        auto baseClassNode = baseClassesNode.append_child("class");
        baseClass.SaveTo(baseClassNode);
    }
}

void SyntaxClass::LoadFrom(const pugi::xml_node& node, SyntaxNodeArena& arena)
{
    SyntaxSection::LoadFrom(node, arena);

    mTemplateParameters = node.attribute("templates").as_string();
    mClassSection = (SyntaxProtectionSection)node.attribute("protection").as_int();
    mAttributeCommentDef = node.attribute("attributeCommentDef").as_string();
    mAttributeShortDef = node.attribute("attributeShortDef").as_string();

    for (auto baseClassNode : node.child("baseClasses"))
    {
        SyntaxClassInheritance baseClass;
        baseClass.LoadFrom(baseClassNode);
        mBaseClasses.push_back(baseClass);
    }
}

const string& SyntaxType::GetName() const
{
    return mName;
}

bool SyntaxType::IsConstant() const
{
    return mIsConstant;
}

bool SyntaxType::IsReference() const
{
    return mIsReference;
}

bool SyntaxType::IsPointer() const
{
    return mIsPointer;
}

const SyntaxType& SyntaxVariable::GetVariableType() const
{
    return mType;
}

const string& SyntaxVariable::GetName() const
{
    return mName;
}

const string& SyntaxVariable::GetDefaultValue() const
{
    return mDefaultValue;
}

SyntaxProtectionSection SyntaxVariable::GetClassSection() const
{
    return mClassSection;
}

bool SyntaxVariable::IsStatic() const
{
    return mIsStatic;
}

const SyntaxType& SyntaxFunction::GetReturnType() const
{
    return mReturnType;
}

const string& SyntaxFunction::GetName() const
{
    return mName;
}

const SyntaxVariablesVec& SyntaxFunction::GetParameters() const
{
    return mParameters;
}

SyntaxProtectionSection SyntaxFunction::GetClassSection() const
{
    return mClassSection;
}

bool SyntaxFunction::IsTemplate() const
{
    return !mTemplates.empty();
}

const string& SyntaxFunction::GetTemplates() const
{
    return mTemplates;
}

bool SyntaxFunction::IsStatic() const
{
    return mIsStatic;
}

bool SyntaxFunction::IsVirtual() const
{
    return mIsVirtual;
}

const string& SyntaxEnum::GetName() const
{
    return mName;
}

const string& SyntaxEnum::GetFullName() const
{
    return mFullName;
}

const StringStringDict& SyntaxEnum::GetEntries() const
{
    return mEntries;
}

SyntaxProtectionSection SyntaxEnum::GetClassSection() const
{
    return mClassSection;
}

SyntaxSection* SyntaxEnum::GetOwnerSyntaxSection() const
{
    return mOwnerSection;
}

SyntaxClassInheritance::SyntaxClassInheritance(const string& className, SyntaxProtectionSection type):
    mClassName(className), mInheritanceType(type)
{}

const string& SyntaxClassInheritance::GetClassName() const
{
    return mClassName;
}

SyntaxClass* SyntaxClassInheritance::GetClass() const
{
    return mClass;
}

SyntaxProtectionSection SyntaxClassInheritance::GetInheritanceType() const
{
    return mInheritanceType;
}

void SyntaxClassInheritance::SaveTo(pugi::xml_node& node) const
{
    node.append_attribute("name") = mClassName.c_str();
    node.append_attribute("protection") = (int)mInheritanceType;
}

void SyntaxClassInheritance::LoadFrom(const pugi::xml_node& node)
{
    mClassName = node.attribute("name").as_string();
    mInheritanceType = (SyntaxProtectionSection)node.attribute("protection").as_int();
}

bool SyntaxClassInheritance::operator==(const SyntaxClassInheritance& other) const
{
    return mInheritanceType == other.mInheritanceType && mClassName == other.mClassName;
}

const string& SyntaxUsingNamespace::GetUsingNamespaceName() const
{
    return mUsingNamespaceName;
}

SyntaxSection* SyntaxUsingNamespace::GetUsingNamespace() const
{
    return mUsingNamespace;
}

void SyntaxUsingNamespace::SaveTo(pugi::xml_node& node) const
{
    node.append_attribute("name") = mUsingNamespaceName.c_str();
}

void SyntaxUsingNamespace::LoadFrom(const pugi::xml_node& node)
{
    mUsingNamespaceName = node.attribute("name").as_string();
}

const string& SyntaxTypedef::GetWhatName() const
{
    return mWhatName;
}

const string& SyntaxTypedef::GetNewDefName() const
{
    return mNewDefName;
}

SyntaxSection* SyntaxTypedef::GetWhat() const
{
    return mWhatSection;
}

void SyntaxTypedef::SaveTo(pugi::xml_node& node) const
{
    node.append_attribute("what") = mWhatName.c_str();
    node.append_attribute("newDef") = mNewDefName.c_str();
}

void SyntaxTypedef::LoadFrom(const pugi::xml_node& node)
{
    mWhatName = node.attribute("what").as_string();
    mNewDefName = node.attribute("newDef").as_string();
}

const vector<string>& SyntaxAttributes::GetAttributesList() const
{
    return mAttributesList;
}

const string& SyntaxDefineIf::GetDefinition() const
{
    return mDefinition;
}
