#pragma once

#include "SyntaxTree.h"

bool EndsWith(const string& str, const string& ends);
bool StartsWith(const string& str, const string& starts);
string& Trim(string& str, const string& chars = " ");
string& TrimEnd(string& str, const string& chars = " ");
string& TrimStart(string& str, const string& chars = " ");
void Split(const string& s, char delim, vector<string>& elems);
vector<string> Split(const string& s, char delim);

class CppSyntaxParser
{
public:
    CppSyntaxParser();
    ~CppSyntaxParser();

    void ParseFile(SyntaxFile& file, const string& filePath, const TimeStamp& fileEditDate, bool excludeMode);

protected:
    typedef void(CppSyntaxParser::* ParserDelegate)(SyntaxSection&, int&, SyntaxProtectionSection&);

    struct ExpressionParser
    {
        const char* keyWord = nullptr;
        bool           isPossibleInClass = true;
        bool           isPossibleInNamespace = true;
        ParserDelegate parser;

        ExpressionParser() {}
        ExpressionParser(const char* keyWord, ParserDelegate parser, bool isPossibleInClass = true,
                         bool isPossibleInNamespace = true);
    };
    typedef vector<ExpressionParser*> ParsersVec;

protected:
    string      mSourcesPath;
    ParsersVec  mParsers;
	vector<pair<string, string>> mReplaces;

    SyntaxDefineIf* mCurrentDefine = nullptr;

protected:
    void InitializeParsers();
	void InitializeReplaces();

	void ProcessReplaces(string& data);

    void ParseSyntaxSection(SyntaxSection& section, const string& source, SyntaxFile& file,
                            SyntaxProtectionSection protectionSection);

    int GetLineNumber(const string& data, int caret);

    string ReadWord(const string& data, int& caret,
                    const char* breakSymbols = " \n\r(){}.,;+-*/=@!|&*:~\\",
                    const char* skipSymbols = " \n\r",
                    bool checkBraces = true, bool checkFgBraces = true, bool checkTrBraces = true);

    string ReadBlock(const string& data, int& caret);

    string ReadBraces(const string& data, int& caret);

    char GetNextSymbol(const string& data, int begin, const char* skipSymbols = " \n\r\t()[]{}");

    vector<string> Split(const string& data, char splitSymbol);

    void RemoveComments(string& input);

    void TryParseBlock(SyntaxSection& section, const string& block, int blockBegin, int& caret,
                       SyntaxProtectionSection& protectionSection);

    bool IsFunction(const string& data);

    SyntaxVariable* ParseVariable(const string& data, SyntaxProtectionSection& protectionSection, int begin, int end);

    SyntaxFunction* ParseFunction(const string& data, SyntaxProtectionSection& protectionSection, int begin, int end);

    void ParseNamespace(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseComment(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseMultilineComment(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParsePragma(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseInclude(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseDefine(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseUndef(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseIfdefMacros(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseIfMacros(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseEndIfMacros(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseElifMacros(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseElseMacros(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseMetaClass(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseClass(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseStruct(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseClassOrStruct(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection,
                            bool isClass, bool isMeta, const string& templates);

    void ParseTemplate(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseTypedef(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseEnum(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseUsing(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParsePublicSection(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParsePrivateSection(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseProtectedSection(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseFriend(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseAttributeCommentDef(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseAttributeShortDef(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseAttributes(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseProperties(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseProperty(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseGetter(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseSetter(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

    void ParseAccessor(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);

	void ParseCocosNamespace(SyntaxSection& section, int& caret, SyntaxProtectionSection& protectionSection);
};
