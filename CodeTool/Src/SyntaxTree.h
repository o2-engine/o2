#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "pugixml/pugixml.hpp"

#undef GetClassName

using namespace std;

class SyntaxAttributes;
class SyntaxClass;
class SyntaxClassInheritance;
class SyntaxComment;
class SyntaxDefineIf;
class SyntaxEnum;
class SyntaxFile;
class SyntaxFunction;
class SyntaxNamespace;
class SyntaxSection;
class SyntaxType;
class SyntaxTypedef;
class SyntaxUsingNamespace;
class SyntaxVariable;
class ISyntaxExpression;

typedef map<string, string>           StringStringDict;
typedef vector<SyntaxClass*>          SyntaxClassesVec;
typedef vector<SyntaxEnum*>           SyntaxEnumsVec;
typedef vector<SyntaxFile*>           SyntaxFilesVec;
typedef vector<SyntaxFunction*>       SyntaxFunctionsVec;
typedef vector<SyntaxSection*>        SyntaxSectionsVec;
typedef vector<SyntaxTypedef*>        SyntaxTypedefsVec;
typedef vector<SyntaxUsingNamespace*> SyntaxUsingNamespacesVec;
typedef vector<SyntaxVariable*>       SyntaxVariablesVec;

enum class SyntaxProtectionSection { Public, Private, Protected };

// Owns all syntax nodes. The cache merges files into one global namespace by reparenting
// nodes between sections, so nodes can't own each other - the arena holds them all instead
class SyntaxNodeArena
{
public:
    template<typename T>
    T* Make()
    {
        auto node = make_unique<T>();
        T* raw = node.get();
        mNodes.push_back(std::move(node));
        return raw;
    }

private:
    vector<unique_ptr<ISyntaxExpression>> mNodes;
};

// Abstract syntax tree of one source file
class SyntaxFile
{
public:
    SyntaxFile();

    // Returns file path
    const string& GetPath() const;

    // Returns file's data
    const string& GetData() const;

    // Returns global syntax namespace in this file
    SyntaxNamespace* GetGlobalNamespace() const;

    // Returns arena that owns this file's nodes
    SyntaxNodeArena& GetArena();

    // Saves data to xml node
    void SaveTo(pugi::xml_node& node) const;

    // Loads data from xml node
    void LoadFrom(const pugi::xml_node& node);

protected:
    string mPath; // File path
    string mData; // File data

    SyntaxNodeArena mArena; // Owns all nodes of this file's tree

    SyntaxNamespace* mGlobalNamespace = nullptr; // Global syntax namespace in file

    friend class CppSyntaxParser;
    friend class CodeToolApplication;
};

// Syntax expression base
class ISyntaxExpression
{
public:
    virtual ~ISyntaxExpression() = default;

    // Returns start of expression in owner file data
    int GetBegin() const;

    // Returns length of expression text
    int GetLength() const;

    // Returns end of expression
    int GetEnd() const;

    // Returns line of expression end
    int GetLine() const;

    // Returns expression text
    const string& GetData() const;

    // Returns pointer to owner file
    SyntaxFile* GetOwnerFile() const;

    // Returns pointer to owner define
    SyntaxDefineIf* GetDefine() const;

    // Returns attached comment: trailing on the same line or directly above the expression
    SyntaxComment* GetComment() const;

    // Returns attached ATTRIBUTES(...) macro from the line directly above
    SyntaxAttributes* GetAttributesMacro() const;

protected:
    int mBegin = 0;  // Data begin position
    int mLength = 0; // Data length
    int mLine = 0;   // Line number of expression end

    string mData; // Expression text

    SyntaxFile* mFile = nullptr; // Owner file

    SyntaxDefineIf* mDefine = nullptr; // Owner define

    SyntaxComment*    mComment = nullptr;         // Attached comment (if any)
    SyntaxAttributes* mAttributesMacro = nullptr; // Attached ATTRIBUTES(...) macro (if any)

    friend class CppSyntaxParser;
};

// Syntax single/multiline comment
class SyntaxComment: public ISyntaxExpression
{
};

// Syntax using namespace ...
class SyntaxUsingNamespace: public ISyntaxExpression
{
public:
    // Returns using namespace name
    const string& GetUsingNamespaceName() const;

    // Returns using namespace (if found)
    SyntaxSection* GetUsingNamespace() const;

    // Saves data to xml node
    void SaveTo(pugi::xml_node& node) const;

    // Loads data from xml node
    void LoadFrom(const pugi::xml_node& node);

protected:
    string         mUsingNamespaceName;       // Using namespace name
    SyntaxSection* mUsingNamespace = nullptr; // Using namespace (if found)

    friend class CodeToolCache;
    friend class CppSyntaxParser;
};

// Syntax typedef X Y;
class SyntaxTypedef: public ISyntaxExpression
{
public:
    // Returns what was defined (X)
    const string& GetWhatName() const;

    // Returns new defined name (Y)
    const string& GetNewDefName() const;

    // Returns section of what was defined (X), if resolved
    SyntaxSection* GetWhat() const;

    // Saves data to xml node
    void SaveTo(pugi::xml_node& node) const;

    // Loads data from xml node
    void LoadFrom(const pugi::xml_node& node);

protected:
    string         mWhatName;              // What was used to defined name (X)
    string         mNewDefName;            // What was new defined name (Y)
    SyntaxSection* mWhatSection = nullptr; // What section used to define (X)

    friend class CodeToolCache;
    friend class CppSyntaxParser;
};

// Syntax names section, base for namespaces or classes
class SyntaxSection: public ISyntaxExpression
{
public:
    // Returns parent section
    virtual SyntaxSection* GetParentSection() const;

    // Returns section name
    virtual const string& GetName() const;

    // Returns full section name including all parent names (something like A::B::C)
    virtual const string& GetFullName() const;

    // Returns array of functions
    virtual const SyntaxFunctionsVec& GetFunctions() const;

    // Returns array of variables
    virtual const SyntaxVariablesVec& GetVariables() const;

    // Returns nested sections
    virtual const SyntaxSectionsVec& GetSections() const;

    // Returns nested enums
    virtual const SyntaxEnumsVec& GetEnums() const;

    // Returns typedefs defined in this section
    virtual const SyntaxTypedefsVec& GetTypedefs() const;

    // Returns using namespaces in this section
    virtual const SyntaxUsingNamespacesVec& GetUsingNamespaces() const;

    // Returns all inside sections
    virtual SyntaxSectionsVec GetAllSections() const;

    // Returns all inside classes
    virtual SyntaxClassesVec GetAllClasses() const;

    // Returns all enums
    virtual SyntaxEnumsVec GetAllEnums() const;

    // Returns is this section is class
    virtual bool IsClass() const;

    // Saves data to xml node
    virtual void SaveTo(pugi::xml_node& node) const;

    // Loads data from xml node
    virtual void LoadFrom(const pugi::xml_node& node, SyntaxNodeArena& arena);

protected:
    string mName;     // Short name of section
    string mFullName; // Full name of section with all parents names

    SyntaxSection* mParentSection = nullptr; // Pointer to parent section (nullptr for global section)

    SyntaxFunctionsVec       mFunctions;       // List of functions
    SyntaxVariablesVec       mVariables;       // List of variables
    SyntaxSectionsVec        mSections;        // List of nested sections (classes or namespaces)
    SyntaxEnumsVec           mEnums;           // List of enums
    SyntaxTypedefsVec        mTypedefs;        // List of typedefs
    SyntaxUsingNamespacesVec mUsingNamespaces; // List of using namespaces

    friend class CodeToolCache;
    friend class CppSyntaxParser;
};

// Syntax namespace
class SyntaxNamespace: public SyntaxSection
{
public:
    friend class CodeToolCache;
    friend class CppSyntaxParser;
};

// Syntax class inheritance definition
class SyntaxClassInheritance
{
public:
    SyntaxClassInheritance() {}
    SyntaxClassInheritance(const string& className, SyntaxProtectionSection type);

    // Returns class name
    const string& GetClassName() const;

    // Returns class
    SyntaxClass* GetClass() const;

    // Returns class inheritance protection type
    SyntaxProtectionSection GetInheritanceType() const;

    // Check equality operator
    bool operator==(const SyntaxClassInheritance& other) const;

    // Saves data to xml node
    void SaveTo(pugi::xml_node& node) const;

    // Loads data from xml node
    void LoadFrom(const pugi::xml_node& node);

protected:
    string                  mClassName;       // Inheritance class name
    SyntaxClass*            mClass = nullptr; // Inheritance class (if found)
    SyntaxProtectionSection mInheritanceType = SyntaxProtectionSection::Private; // Inheritance protection type

    friend class CodeToolCache;
    friend class CppSyntaxParser;
};
typedef vector<SyntaxClassInheritance> SyntaxClassInheritancesVec;

// Syntax class or struct
class SyntaxClass: public SyntaxSection
{
public:
    // Returns array of functions
    const SyntaxFunctionsVec& GetFunctions() const;

    // Returns array of variables
    const SyntaxVariablesVec& GetVariables() const;

    // Returns nested sections
    const SyntaxSectionsVec& GetSections() const;

    // Returns nested enums
    const SyntaxEnumsVec& GetEnums() const;

    // Returns typedefs defined in this section
    const SyntaxTypedefsVec& GetTypedefs() const;

    // Returns using namespaces in this section
    const SyntaxUsingNamespacesVec& GetUsingNamespaces() const;

    // Returns all inside sections
    SyntaxSectionsVec GetAllSections() const;

    // Returns all inside classes
    SyntaxClassesVec GetAllClasses() const;

    // Returns is this section is class
    bool IsClass() const;

    // Returns base classes
    const SyntaxClassInheritancesVec& GetBaseClasses() const;

    // Returns is class template, including nesting into template class
    bool IsTemplate() const;

    // Returns template parameters (if exist)
    const string& GetTemplateParameters() const;

    // Returns parent class protection section
    SyntaxProtectionSection GetClassSection() const;

    // Returns comment definition for attribute (empty for not attribute classes)
    const string& GetAttributeCommentDef() const;

    // Returns short definition for attribute (empty for not attribute classes)
    const string& GetAttributeShortDef() const;

    // Saves data to xml node
    void SaveTo(pugi::xml_node& node) const;

    // Loads data from xml node
    void LoadFrom(const pugi::xml_node& node, SyntaxNodeArena& arena);

protected:
    SyntaxClassInheritancesVec mBaseClasses; // Base classes

    string mTemplateParameters; // Template parameters (empty if class isn't template)

    SyntaxProtectionSection mClassSection = SyntaxProtectionSection::Public; // Protection section of parent class

    SyntaxClass* mSourceClass = nullptr; // Source class for template specialized classes

    string mAttributeCommentDef; // Attribute comment definition
    string mAttributeShortDef;   // Attribute short definition

    friend class CodeToolCache;
    friend class CppSyntaxParser;
};

// Syntax attributes list, from ATTRIBUTES(...) macro
class SyntaxAttributes: public ISyntaxExpression
{
public:
    // Returns list of attributes
    const vector<string>& GetAttributesList() const;

protected:
    vector<string> mAttributesList;

    friend class CodeToolCache;
    friend class CppSyntaxParser;
};

// Syntax variable type
class SyntaxType
{
public:
    // Returns name of type
    const string& GetName() const;

    // Returns is type is constant
    bool IsConstant() const;

    // Returns is type is reference
    bool IsReference() const;

    // Returns is type is pointer
    bool IsPointer() const;

protected:
    string mName;

    bool mIsConstant = false;
    bool mIsReference = false;
    bool mIsPointer = false;
    bool mIsMutable = false;

    friend class CppSyntaxParser;
};

// Syntax variable
class SyntaxVariable: public ISyntaxExpression
{
public:
    // Returns type of variable
    const SyntaxType& GetVariableType() const;

    // Returns name of variable
    const string& GetName() const;

    // Returns default value
    const string& GetDefaultValue() const;

    // Returns class definition section
    SyntaxProtectionSection GetClassSection() const;

    // Returns is variable is static
    bool IsStatic() const;

protected:
    SyntaxType mType; // Type of variable

    string mName;         // Name of variable
    string mDefaultValue; // Default variable value

    SyntaxProtectionSection mClassSection = SyntaxProtectionSection::Public; // Protection section

    bool mIsStatic = false; // Is variable static

    friend class CppSyntaxParser;
};

// Syntax function
class SyntaxFunction: public ISyntaxExpression
{
public:
    // Returns function's returning type
    const SyntaxType& GetReturnType() const;

    // Returns name of function
    const string& GetName() const;

    // Returns list of function's parameters
    const SyntaxVariablesVec& GetParameters() const;

    // Returns protection section
    SyntaxProtectionSection GetClassSection() const;

    // Returns is function template
    bool IsTemplate() const;

    // Returns function templates (if have)
    const string& GetTemplates() const;

    // Returns is function static
    bool IsStatic() const;

    // Returns is function virtual
    bool IsVirtual() const;

protected:
    SyntaxType mReturnType; // Returning type

    string mTemplates; // Function templates
    string mName;      // Name of function

    SyntaxVariablesVec mParameters; // List of parameters

    SyntaxProtectionSection mClassSection = SyntaxProtectionSection::Public; // Protection section

    bool mIsStatic = false;   // Is function static
    bool mIsVirtual = false;  // Is function virtual
    bool mIsConstant = false; // Is function constant

    friend class CppSyntaxParser;
};

// Syntax enum
class SyntaxEnum: public ISyntaxExpression
{
public:
    // Returns name of enum
    const string& GetName() const;

    // Returns full name of enum with all parent spaces names
    const string& GetFullName() const;

    // Returns enum entries
    const StringStringDict& GetEntries() const;

    // Returns protection section
    SyntaxProtectionSection GetClassSection() const;

    // Returns owner syntax section
    SyntaxSection* GetOwnerSyntaxSection() const;

protected:
    string mName;     // Name of enum
    string mFullName; // Full enum name with all parent spaces names

    StringStringDict mEntries; // Entries of enum

    SyntaxSection*          mOwnerSection = nullptr; // Owner syntax section
    SyntaxProtectionSection mClassSection = SyntaxProtectionSection::Public;

    friend class CppSyntaxParser;
};

// Syntax #if/#ifdef condition, attached to expressions inside conditional blocks
class SyntaxDefineIf: public ISyntaxExpression
{
public:
    const string& GetDefinition() const;

protected:
    string mDefinition; // Condition statement after #if

    friend class CppSyntaxParser;
};
