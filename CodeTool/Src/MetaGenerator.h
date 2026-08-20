#pragma once

#include "CodeToolCache.h"
#include "SyntaxTree.h"

// Generates reflection meta sections in sources: CLASS_*_META and PRE_ENUM_META blocks
// at the end of headers, DECLARE_CLASS and ENUM_META blocks at the end of cpp files
class MetaGenerator
{
public:
    explicit MetaGenerator(CodeToolCache& cache);

    // Regenerates meta blocks for the header file and its sibling cpp, writing them
    // only when the content changed. Collects register ids of declared classes
    void UpdateSourceReflection(SyntaxFile* file);

    // Returns register ids of all declared classes, in generation order
    const vector<string>& GetClassRegistrators() const;

protected:
    CodeToolCache& mCache;

    vector<string> mClassRegistrators; // Register ids of DECLARE_CLASS'ed classes

protected:
    // Adds meta comment begin section
    void AddBeginMeta(bool& hasMeta, string& res);

    // Adds meta comment end section
    void AddEndMeta(bool hasMeta, string& res);

    // Returns class declaration meta
    string GetClassDeclaration(SyntaxClass* cls);

    // Returns class reflection meta
    string GetClassMeta(SyntaxClass* cls);

    // Returns is comment marked with @IGNORE
    bool IsIgnoreComment(SyntaxComment* synComment);

    // Adds #if if item's condition differs from the previous one
    void CheckIfDefines(ISyntaxExpression* item, SyntaxDefineIf*& prevDefine, string& data);

    // Adds #endif if a condition block is open
    void CompleteIfDefines(SyntaxDefineIf*& prevDefine, string& data);

    // Returns member attributes chain from the attached ATTRIBUTES macro and @-comment markers
    string GetAttributes(SyntaxClass* cls, ISyntaxExpression* member);

    // Returns enum reflection meta for cpp
    string GetEnumMeta(SyntaxEnum* enm);

    // Returns enum reflection meta for header
    string GetEnumPreMeta(SyntaxEnum* enm);

    // Builds meta templates parameters for template classes
    void AggregateTemplates(SyntaxSection* sec, string& templates, string& fullName);

    // Returns class full name with template parameters resolved in global space
    string GetClassNormalizedTemplates(const string& name, const string& nspace);

    // Removes generated metas from source
    void RemoveMetas(string& data, const char* keyword, const char* endword, bool allowMultiline = true);

    // Returns is function reflectable
    bool IsFunctionReflectable(SyntaxFunction* function, SyntaxSection* owner) const;
};
