#pragma once

#include "Lexer.h"
#include "SyntaxTree.h"

// Fast single-pass C++ parser: tokenizes the source, then does recursive descent over
// the tokens. Deliberately not a full C++ parser — it recognizes only the constructs
// the reflection generator needs: namespaces, classes and structs, enums, class members,
// reflection macros (PROPERTY, ATTRIBUTES, ...) and #if/#ifdef conditions.
//
// Comments and ATTRIBUTES(...) macros are attached to members during parsing: a comment
// on the member's last line or directly above it, an ATTRIBUTES macro from the line above.
class CppSyntaxParser
{
public:
    // Reads and parses file from disk into syntax tree
    void ParseFile(SyntaxFile& file, const string& filePath);

    // Parses source text into syntax tree
    void ParseSource(SyntaxFile& file, const string& filePath, const string& source);

private:
    // Parsing state of one section body: protection section and pending attachments
    struct SectionContext
    {
        SyntaxSection* section = nullptr;

        SyntaxProtectionSection protection = SyntaxProtectionSection::Public;

        ISyntaxExpression* lastMember = nullptr;        // Last registered member, target for trailing comments
        SyntaxComment*     pendingComment = nullptr;    // Comment waiting for the next member
        SyntaxAttributes*  pendingAttributes = nullptr; // ATTRIBUTES(...) waiting for the next member
    };

    // Parsed type: text is the raw source slice, const is stripped into a flag
    struct ParsedType
    {
        string text;

        bool isConst = false;
        bool isMutable = false;
        bool valid = false;
    };

private:
    const string* mSource = nullptr; // Parsed source text, owned by the current SyntaxFile
    vector<Token> mTokens;           // Token stream of the current source
    int           mPos = 0;          // Current token index

    SyntaxFile*      mFile = nullptr;  // Currently parsed file
    SyntaxNodeArena* mArena = nullptr; // Arena of the currently parsed file

    vector<SyntaxDefineIf*> mDefinesStack; // Stack of nested #if conditions; back() is the active one

private:
    const Token& Tok(int i) const { return mTokens[i]; }
    bool AtEnd(int i) const { return i >= (int)mTokens.size(); }
    bool AtEnd() const { return AtEnd(mPos); }

    // Returns raw source text from first token begin to last token end (exclusive index)
    string Slice(int beginTokenIdx, int endTokenIdx) const;

    // Returns index of the next non-comment token starting from i
    int NextSignificant(int i) const;

    // Advances mPos over comment tokens
    void SkipComments();

    // Returns index of the token after the matching closing angle bracket; i points at '<'
    int SkipBalancedAngles(int i) const;

    // Returns index of the matching closing parenthesis; i points at '('
    int FindMatchingParen(int i) const;

    // Returns the active #if condition or nullptr
    SyntaxDefineIf* CurrentDefine() const;

    // Parses tokens of one section body; stops at matching '}' (not consumed) or at end
    void ParseSectionBody(SectionContext& ctx);

    void ParseNamespace(SectionContext& ctx);

    void ParseClassOrStruct(SectionContext& ctx, bool isClass, const string& templates);

    void ParseTemplate(SectionContext& ctx);

    void ParseEnum(SectionContext& ctx);

    void ParseTypedef(SectionContext& ctx);

    void ParseUsing(SectionContext& ctx);

    void ParseFriend();

    void ParseLineComments(SectionContext& ctx);

    void ParseBlockComment(SectionContext& ctx);

    // Attaches a just-parsed comment: trailing to the last member or pending for the next one
    void PlaceComment(SectionContext& ctx, SyntaxComment* comment, int startLine);

    void HandleDirective(SectionContext& ctx);

    void ParseAttributeDefinition(SectionContext& ctx, bool shortDefinition);

    void ParseAttributesMacro(SectionContext& ctx);

    // Parses PROPERTY/GETTER/SETTER/ACCESSOR macro into a variable named by the second argument
    void ParsePropertyMacro(SectionContext& ctx);

    // Reads macro arguments between parentheses, splitting by top-level commas;
    // on return mPos points after the closing parenthesis. Returns false if no parentheses
    bool ReadMacroArguments(vector<string>& args);

    // Skips tokens until top-level ';' inclusive; stops before an unmatched '}'
    void SkipToSemicolon();

    // Returns ';' token line after skipping, for expressions whose line is their end line
    int SkipToSemicolonLine();

    // Parses one member declaration (variable or function), consuming its tokens.
    // A non-function templated declaration is skipped
    void ParseMemberDeclaration(SectionContext& ctx, const string& templates = "");

    // Parses declaration signature tokens [first, sigEnd) into a variable or function
    void ParseMemberSignature(SectionContext& ctx, int first, int sigEnd, const string& templates,
                              int declBegin, int endLine);

    // Parses a type at token index i, advancing it: [const|mutable]* core<...>::chain [*&]*
    ParsedType ParseType(int& i, int endTok) const;

    // Parses function parameters from '(' at parenIdx into the function
    void ParseFunctionParameters(SyntaxFunction* function, int parenIdx, int sigEnd);

    // Parses one function parameter from token range [first, last)
    SyntaxVariable* ParseParameter(int first, int last);

    // Attaches pending comment and attributes to a newly registered member
    void RegisterMember(SectionContext& ctx, ISyntaxExpression* member, int startLine);
};
