#include "MetaGenerator.h"

#include <algorithm>

#include "Log.h"
#include "TextUtils.h"

MetaGenerator::MetaGenerator(CodeToolCache& cache):
    mCache(cache)
{}

const vector<string>& MetaGenerator::GetClassRegistrators() const
{
    return mClassRegistrators;
}

void MetaGenerator::UpdateSourceReflection(SyntaxFile* file)
{
    string cppSource, cppSourceInitial;
    bool cppLoaded = false;

    string hSource = file->GetData();

    if (hSource.find("@CODETOOLIGNORE") != string::npos)
        return;

    bool hasHeaderMeta = false;
    bool hasSourceMeta = false;

    RemoveMetas(hSource, "META_TEMPLATES(", "END_META;");
    RemoveMetas(hSource, "CLASS_BASES_META(", "END_META;");
    RemoveMetas(hSource, "CLASS_FIELDS_META(", "END_META;");
    RemoveMetas(hSource, "CLASS_METHODS_META(", "END_META;");
    RemoveMetas(hSource, "DECLARE_CLASS(", ");", false);
    RemoveMetas(hSource, "PRE_ENUM_META(", ");", false);
    RemoveMetas(hSource, "// --- META ---", "// --- END META ---");

    string cppSourcePath = file->GetPath().substr(0, file->GetPath().rfind('.')) + ".cpp";

    SyntaxClass* baseObjectClass = (SyntaxClass*)(mCache.FindSection("o2::IObject"));

    auto checkCppLoad = [&]()
    {
        if (cppLoaded)
            return;

        if (IsFileExist(cppSourcePath))
        {
            cppSource = ReadFile(cppSourcePath);
            cppSourceInitial = cppSource;
            RemoveMetas(cppSource, "ENUM_META(", "END_ENUM_META;");
            RemoveMetas(cppSource, "ENUM_META_(", "END_ENUM_META;");
            RemoveMetas(cppSource, "CLASS_META(", "END_META;");
            RemoveMetas(cppSource, "CLASS_TEMPLATE_META(", "END_META;");
            RemoveMetas(cppSource, "CLASS_BASES_META(", "END_META;");
            RemoveMetas(cppSource, "CLASS_FIELDS_META(", "END_META;");
            RemoveMetas(cppSource, "CLASS_METHODS_META(", "END_META;");
            RemoveMetas(cppSource, "DECLARE_CLASS(", ");", false);
            RemoveMetas(cppSource, "PRE_ENUM_META(", ");", false);
            RemoveMetas(cppSource, "// --- META ---", "// --- END META ---");
        }
        else
            cppSource = "#include \"" + GetFileName(file->GetPath()) + "\"\n\n";

        cppLoaded = true;
    };

    // Enums
    SyntaxEnumsVec allEnums = file->GetGlobalNamespace()->GetAllEnums();
    SyntaxEnumsVec metaEnums;

    for (auto enm : allEnums)
    {
        auto owner = enm->GetOwnerSyntaxSection();

        // Enums of template classes can't be registered by full name
        if (owner && owner->IsClass() && ((SyntaxClass*)owner)->IsTemplate())
            continue;

        // Non-public enums inside classes can't be accessed from outside
        if (owner && owner->IsClass() && enm->GetClassSection() != SyntaxProtectionSection::Public)
            continue;

        metaEnums.push_back(enm);
    }

    if (!metaEnums.empty())
        checkCppLoad();

    for (auto enm : metaEnums)
    {
        AddBeginMeta(hasSourceMeta, cppSource);
        cppSource += GetEnumMeta(enm);

        AddBeginMeta(hasHeaderMeta, hSource);
        hSource += GetEnumPreMeta(enm);

        VerboseLog("Generated meta for enum:%s\n", enm->GetFullName().c_str());
    }

    // Classes
    auto classes = file->GetGlobalNamespace()->GetAllClasses();

    for (auto cls : classes)
    {
        bool hasIObject = std::find_if(cls->GetFunctions().begin(), cls->GetFunctions().end(),
                                       [](SyntaxFunction* x) {
                                           return x->GetName() == "IOBJECT" || x->GetName() == "SERIALIZABLE" ||
                                               x->GetName() == "ASSET_TYPE";
                                       }) != cls->GetFunctions().end();

        if (!mCache.IsClassBasedOn(cls, baseObjectClass) || !hasIObject || cls == baseObjectClass)
            continue;

        if (!cls->IsTemplate())
        {
            checkCppLoad();

            AddBeginMeta(hasSourceMeta, cppSource);
            cppSource += GetClassDeclaration(cls);
        }

        AddBeginMeta(hasHeaderMeta, hSource);
        hSource += GetClassMeta(cls);

        VerboseLog("Generated meta for class:%s\n", cls->GetFullName().c_str());
    }

    AddEndMeta(hasSourceMeta, cppSource);
    AddEndMeta(hasHeaderMeta, hSource);

    // Write
    if (cppLoaded && cppSource != cppSourceInitial)
        WriteFile(cppSourcePath, cppSource);

    if (hSource != file->GetData())
        WriteFile(file->GetPath(), hSource);

    VerboseLog("Reflection generated for %s\n", file->GetPath().c_str());
}

void MetaGenerator::AddBeginMeta(bool& hasMeta, string& res)
{
    if (!hasMeta)
        res += "// --- META ---\n";

    hasMeta = true;
}

void MetaGenerator::AddEndMeta(bool hasMeta, string& res)
{
    if (hasMeta)
        res += "// --- END META ---\n";
}

string MetaGenerator::GetClassDeclaration(SyntaxClass* cls)
{
    string res = "\n";

    string nspace;
    int nspaceDelimer = (int)cls->GetFullName().rfind("::");
    if (nspaceDelimer != (int)string::npos)
        nspace = cls->GetFullName().substr(0, nspaceDelimer);

    if (cls->GetDefine())
        res += "#if " + cls->GetDefine()->GetDefinition() + "\n";

    string className = GetClassNormalizedTemplates(cls->GetFullName(), nspace);

    string classRegisterId = className;
    for (auto& c : classRegisterId)
    {
        if (c == '<' || c == '>' || c == ':')
            c = '_';
    }

    mClassRegistrators.push_back(classRegisterId);

    res += "DECLARE_CLASS(" + className + ", " + classRegisterId + ");\n";

    if (cls->GetDefine())
        res += "#endif\n";

    return res;
}

string MetaGenerator::GetClassMeta(SyntaxClass* cls)
{
    string res = "\n";
    res.reserve(cls->GetData().length()*2);

    string nspace;
    int nspaceDelimer = (int)cls->GetFullName().rfind("::");
    if (nspaceDelimer != (int)string::npos)
        nspace = cls->GetFullName().substr(0, nspaceDelimer);

    string classDef;
    string templates;

    if (!cls->IsTemplate())
        classDef = GetClassNormalizedTemplates(cls->GetFullName(), nspace);
    else
        AggregateTemplates(cls, templates, classDef);

    // Defines
    if (cls->GetDefine())
        res += "#if " + cls->GetDefine()->GetDefinition() + "\n";

    // Base classes
    res += templates;
    res += "CLASS_BASES_META(" + classDef + ")\n{\n";

    int typedefs = 0;
    for (auto baseClass : cls->GetBaseClasses())
    {
        auto classInfo = mCache.FindSection(baseClass.GetClassName(), nspace, false);
        auto className = classInfo ? classInfo->GetFullName() : baseClass.GetClassName();

        // Commas inside template arguments break the macro - hide them behind a typedef
        if (className.find(',') != string::npos)
        {
            typedefs++;
            auto newClassName = string("_tmp") + to_string(typedefs);
            res += string("    typedef ") + className + ' ' + newClassName + ";\n";
            className = newClassName;
        }

        res += string("    BASE_CLASS(") + className + ");\n";
    }
    res += "}\nEND_META;\n";

    // Fields
    res += templates;
    res += "CLASS_FIELDS_META(" + classDef + ")\n{\n";

    SyntaxDefineIf* currentIf = nullptr;

    for (auto variable : cls->GetVariables())
    {
        if (variable->IsStatic() || variable->GetName().empty())
            continue;

        if (IsIgnoreComment(variable->GetComment()))
            continue;

        CheckIfDefines(variable, currentIf, res);

        res += "    FIELD()";

        if (variable->GetClassSection() == SyntaxProtectionSection::Public)
            res += ".PUBLIC()";
        else if (variable->GetClassSection() == SyntaxProtectionSection::Private)
            res += ".PRIVATE()";
        else if (variable->GetClassSection() == SyntaxProtectionSection::Protected)
            res += ".PROTECTED()";

        res += GetAttributes(cls, variable);

        if (!variable->GetDefaultValue().empty() && variable->GetDefaultValue().find("this") == string::npos)
            res += ".DEFAULT_VALUE(" + variable->GetDefaultValue() + ")";

        res += ".NAME(" + variable->GetName() + ");\n";
    }

    CompleteIfDefines(currentIf, res);
    res += "}\nEND_META;\n";

    // Functions
    res += templates;
    res += "CLASS_METHODS_META(" + classDef + ")\n{\n";

    int supportingTypedefsPos = (int)res.length();
    vector<string> supportingTypedefs;

    bool firstFunction = true;
    for (auto function : cls->GetFunctions())
    {
        if (!IsFunctionReflectable(function, cls))
            continue;

        if (IsIgnoreComment(function->GetComment()))
            continue;

        if (firstFunction)
        {
            firstFunction = false;
            res += "\n";
        }

        CheckIfDefines(function, currentIf, res);

        res += "    FUNCTION()";

        if (function->GetClassSection() == SyntaxProtectionSection::Public)
            res += ".PUBLIC()";
        else if (function->GetClassSection() == SyntaxProtectionSection::Private)
            res += ".PRIVATE()";
        else if (function->GetClassSection() == SyntaxProtectionSection::Protected)
            res += ".PROTECTED()";

        res += GetAttributes(cls, function);

        // Constructor name matches the class name; for specializations the class name
        // continues with template arguments
        bool isConstructor = function->GetName() == cls->GetName() ||
            StartsWith(cls->GetName(), function->GetName() + "<");

        if (function->IsStatic())
            res += ".SIGNATURE_STATIC(";
        else if (isConstructor)
            res += ".CONSTRUCTOR(";
        else
            res += ".SIGNATURE(";

        if (!isConstructor)
        {
            auto returnTypeName = (function->GetReturnType().IsConstant() ? "const " : "") +
                function->GetReturnType().GetName();

            if (returnTypeName.find(',') != string::npos)
            {
                supportingTypedefs.push_back(returnTypeName);
                returnTypeName = (string)"_tmp" + to_string((int)supportingTypedefs.size());
            }

            res += returnTypeName;
            res += string(", ") + function->GetName();
        }

        bool first = isConstructor;
        for (auto param : function->GetParameters())
        {
            string parameterName = (param->GetVariableType().IsConstant() ? "const " : "") +
                param->GetVariableType().GetName();

            if (parameterName.find(',') != string::npos)
            {
                supportingTypedefs.push_back(parameterName);
                parameterName = string("_tmp") + to_string((int)supportingTypedefs.size());
            }

            if (!first)
                res += string(", ");
            else
                first = false;

            res += parameterName;
        }

        res += ");\n";
    }

    CompleteIfDefines(currentIf, res);

    // Supporting typedefs for types with commas
    if (!supportingTypedefs.empty())
    {
        string supportingTypedefsStr = "\n";
        for (int i = 0; i < (int)supportingTypedefs.size(); i++)
            supportingTypedefsStr += (string)"    typedef " + supportingTypedefs[i] + " _tmp" + to_string(i + 1) + ";\n";

        res.insert(supportingTypedefsPos, supportingTypedefsStr);
    }

    res += "}\nEND_META;\n";

    if (cls->GetDefine())
        res += "#endif\n";

    return res;
}

bool MetaGenerator::IsIgnoreComment(SyntaxComment* synComment)
{
    if (!synComment)
        return false;

    const string ignore = "@IGNORE";
    auto fnd = synComment->GetData().find(ignore);
    if (fnd == string::npos)
        return false;

    auto nextSymbol = synComment->GetData()[fnd + ignore.size()];
    return nextSymbol == ' ' || nextSymbol == '\t' || nextSymbol == '\n' || nextSymbol == '\0';
}

void MetaGenerator::CheckIfDefines(ISyntaxExpression* item, SyntaxDefineIf*& prevDefine, string& data)
{
    if (item->GetDefine() != prevDefine)
    {
        if (prevDefine)
            data += "#endif\n";

        if (item->GetDefine())
            data += "#if " + item->GetDefine()->GetDefinition() + "\n";

        prevDefine = item->GetDefine();
    }
}

void MetaGenerator::CompleteIfDefines(SyntaxDefineIf*& prevDefine, string& data)
{
    if (prevDefine)
        data += "#endif\n";

    prevDefine = nullptr;
}

string MetaGenerator::GetAttributes(SyntaxClass* cls, ISyntaxExpression* member)
{
    string attributes;

    SyntaxAttributes* synAttributes = member->GetAttributesMacro();
    SyntaxComment* synComment = member->GetComment();

    if (synAttributes)
    {
        for (auto& attributeEntry : synAttributes->GetAttributesList())
        {
            SyntaxClass* attributeClass = dynamic_cast<SyntaxClass*>(mCache.FindSection(attributeEntry, cls));
            if (attributeClass)
            {
                if (!attributeClass->GetAttributeShortDef().empty())
                    attributes += string(".") + attributeClass->GetAttributeShortDef();
                else
                    attributes += string(".ATTRIBUTE(") + attributeClass->GetFullName() + ")";
            }
            else
                attributes += string(".ATTRIBUTE(") + attributeEntry + ")";
        }
    }

    if (synComment)
    {
        for (auto attributeClass : mCache.attributes)
        {
            const string& commentDef = attributeClass->GetAttributeCommentDef();
            if (commentDef.empty())
                continue;

            auto fnd = synComment->GetData().find(commentDef);
            if (fnd == string::npos)
                continue;

            auto nextSymbol = synComment->GetData()[fnd + commentDef.length()];
            bool validEnd = nextSymbol == ' ' || nextSymbol == '\t' || nextSymbol == '\n' ||
                nextSymbol == '\0' || nextSymbol == '(';

            if (!validEnd || fnd == 0 || synComment->GetData()[fnd - 1] != '@')
                continue;

            string parameters = "()";
            auto parametersBegin = fnd + commentDef.length();
            if (synComment->GetData()[parametersBegin] == '(')
            {
                auto parametersEnd = synComment->GetData().find(')', parametersBegin) + 1;
                parameters = synComment->GetData().substr(parametersBegin, parametersEnd - parametersBegin);
            }

            if (!attributeClass->GetAttributeShortDef().empty())
                attributes += string(".") + attributeClass->GetAttributeShortDef() + parameters;
            else
                attributes += string(".ATTRIBUTE(") + attributeClass->GetFullName() + parameters + ")";
        }
    }

    return attributes;
}

string MetaGenerator::GetEnumMeta(SyntaxEnum* enm)
{
    string res;
    res.reserve(enm->GetEntries().size()*15);

    string enumFullName = enm->GetFullName();

    string enumRegisterId = enumFullName;
    for (auto& c : enumRegisterId)
    {
        if (c == '<' || c == '>' || c == ':')
            c = '_';
    }

    res += "\nENUM_META(" + enumFullName + ", " + enumRegisterId + ")\n{\n";

    for (auto& e : enm->GetEntries())
    {
        if (!e.first.empty())
            res += "    ENUM_ENTRY(" + e.first + ");\n";
    }

    res += "}\nEND_ENUM_META;\n";

    return res;
}

string MetaGenerator::GetEnumPreMeta(SyntaxEnum* enm)
{
    return "\nPRE_ENUM_META(" + enm->GetFullName() + ");\n";
}

static void RemoveSubstrs(string& s, const string& p)
{
    string::size_type n = p.length();
    for (string::size_type i = s.find(p); i != string::npos; i = s.find(p))
        s.erase(i, n);
}

void MetaGenerator::AggregateTemplates(SyntaxSection* sec, string& templates, string& fullName)
{
    if (sec->GetParentSection())
        AggregateTemplates(sec->GetParentSection(), templates, fullName);

    if (fullName.empty())
        fullName = sec->GetName();
    else
        fullName += "::" + sec->GetName();

    if (sec->IsClass())
    {
        SyntaxClass* cls = dynamic_cast<SyntaxClass*>(sec);
        if (!cls->GetTemplateParameters().empty())
        {
            templates += "META_TEMPLATES(" + cls->GetTemplateParameters() + ")\n";

            string classTemplates = cls->GetTemplateParameters();
            RemoveSubstrs(classTemplates, "typename ");

            fullName += "<" + classTemplates + ">";
        }
    }
}

string MetaGenerator::GetClassNormalizedTemplates(const string& name, const string& nspace)
{
    string fullName;
    int nameLength = (int)name.length();
    int fnd = 0;
    int lastFnd = fnd;

    while (fnd >= 0)
    {
        lastFnd = fnd;
        fnd = (int)name.find('<', fnd);

        if (fnd == (int)string::npos)
            break;

        int begin = fnd + 1;
        int braces = 0, trBraces = 0, sqBraces = 0;
        for (bool stop = false; !stop && fnd < nameLength; fnd++)
        {
            switch (name[fnd])
            {
            case '(': braces++; break;
            case ')': braces--; break;
            case '[': sqBraces++; break;
            case ']': sqBraces--; break;
            case '<': trBraces++; break;
            case '>':
                trBraces--;
                if (trBraces == 0 && braces == 0 && sqBraces == 0)
                {
                    stop = true;
                    fnd--;
                }
                break;
            }
        }

        string templateParamsStr = name.substr(begin, fnd - begin);
        vector<string> templateParams = SplitOutsideBraces(templateParamsStr, ',');

        fullName += name.substr(lastFnd, begin - lastFnd);
        bool firstParam = true;
        for (auto& templateParam : templateParams)
        {
            Trim(templateParam);

            const string typenameStr = "typename ";
            if (StartsWith(templateParam, typenameStr))
                templateParam.erase(0, typenameStr.size());

            if (!firstParam)
                fullName += ", ";
            else
                firstParam = false;

            auto classInfo = mCache.FindSection(templateParam, nspace);
            fullName += classInfo ? classInfo->GetFullName() : templateParam;
        }
    }

    fullName += name.substr(lastFnd);
    return fullName;
}

void MetaGenerator::RemoveMetas(string& data, const char* keyword, const char* endword,
                                bool allowMultiline /*= true*/)
{
    auto isSkippingChar = [](char x) { return x == '\n' || x == '\r' || x == '\t' || x == '\0' || x == ' '; };

    auto caret = data.find(keyword);
    while (caret != string::npos)
    {
        auto end = data.find(endword, caret);
        if (end == string::npos)
            break;

        while (caret > 0 && isSkippingChar(data[caret - 1]))
            caret--;

        if (caret > 0 && isSkippingChar(data[caret]))
            caret--;

        if (!allowMultiline)
        {
            string keywordStr{ keyword };
            auto newLinePos = data.find("\n", caret + keywordStr.size());
            if (newLinePos != string::npos && newLinePos < end)
                return;
        }

        string endwordStr{ endword };
        data.erase(caret + 1, end + endwordStr.size() - caret - 1);
        caret = data.find(keyword);
    }

    caret = data.length();
    while (caret > 0 && isSkippingChar(data[caret - 1]))
        caret--;

    data.erase(caret);
    data += '\n';
}

bool MetaGenerator::IsFunctionReflectable(SyntaxFunction* function, SyntaxSection* owner) const
{
    static vector<string> ignoringNames = { "SERIALIZABLE", "PROPERTY", "GETTER", "SETTER", "IOBJECT", "ASSET_TYPE",
        "ATTRIBUTE_COMMENT_DEFINITION", "ATTRIBUTE_SHORT_DEFINITION", "BASE_REF_IMPLEMETATION", "FRIEND_REF_MAKE",
        "CLONEABLE_REF", "REF_COUNTERABLE_IMPL" };

    return !StartsWith(function->GetName(), string("~") + owner->GetName()) &&
        function->GetName().find('~') == string::npos &&
        function->GetName().find("operator") == string::npos &&
        !function->IsTemplate() &&
        find(ignoringNames.begin(), ignoringNames.end(), function->GetName()) == ignoringNames.end();
}
