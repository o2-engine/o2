#include "CppSyntaxParser.h"

#include <cstring>

#include "TextUtils.h"

void CppSyntaxParser::ParseFile(SyntaxFile& file, const string& filePath)
{
    ParseSource(file, filePath, ReadFile(filePath));
}

void CppSyntaxParser::ParseSource(SyntaxFile& file, const string& filePath, const string& source)
{
    file.mPath = filePath;
    file.mData = source;

    if (source.find("@CODETOOLIGNORE") != string::npos)
        return;

    mSource = &file.mData;
    mTokens = Tokenize(file.mData);
    mPos = 0;
    mFile = &file;
    mArena = &file.GetArena();
    mDefinesStack.clear();

    SyntaxNamespace* global = file.mGlobalNamespace;
    global->mData = file.mData;
    global->mLength = (int)file.mData.length();
    global->mFile = &file;

    SectionContext ctx;
    ctx.section = global;
    ctx.protection = SyntaxProtectionSection::Public;

    ParseSectionBody(ctx);
}

string CppSyntaxParser::Slice(int beginTokenIdx, int endTokenIdx) const
{
    if (beginTokenIdx >= endTokenIdx || AtEnd(beginTokenIdx))
        return string();

    int begin = Tok(beginTokenIdx).begin;
    int end = Tok(min(endTokenIdx, (int)mTokens.size()) - 1).end;
    return mSource->substr(begin, end - begin);
}

int CppSyntaxParser::NextSignificant(int i) const
{
    while (!AtEnd(i) && (Tok(i).type == TokenType::LineComment || Tok(i).type == TokenType::BlockComment))
        i++;

    return i;
}

void CppSyntaxParser::SkipComments()
{
    mPos = NextSignificant(mPos);
}

int CppSyntaxParser::SkipBalancedAngles(int i) const
{
    int depth = 0;
    for (; !AtEnd(i); i++)
    {
        if (Tok(i).IsPunct(*mSource, '<'))
            depth++;
        else if (Tok(i).IsPunct(*mSource, '>'))
        {
            depth--;
            if (depth == 0)
                return i + 1;
        }
    }

    return i;
}

int CppSyntaxParser::FindMatchingParen(int i) const
{
    int depth = 0;
    for (; !AtEnd(i); i++)
    {
        if (Tok(i).IsPunct(*mSource, '('))
            depth++;
        else if (Tok(i).IsPunct(*mSource, ')'))
        {
            depth--;
            if (depth == 0)
                return i;
        }
    }

    return i;
}

SyntaxDefineIf* CppSyntaxParser::CurrentDefine() const
{
    return mDefinesStack.empty() ? nullptr : mDefinesStack.back();
}

void CppSyntaxParser::ParseSectionBody(SectionContext& ctx)
{
    while (!AtEnd())
    {
        const Token& token = Tok(mPos);

        if (token.type == TokenType::LineComment)
        {
            ParseLineComments(ctx);
            continue;
        }

        if (token.type == TokenType::BlockComment)
        {
            ParseBlockComment(ctx);
            continue;
        }

        if (token.type == TokenType::Directive)
        {
            HandleDirective(ctx);
            mPos++;
            continue;
        }

        if (token.type == TokenType::Punct)
        {
            char c = (*mSource)[token.begin];

            if (c == '}')
                return;

            if (c == ';')
            {
                mPos++;
                continue;
            }

            ParseMemberDeclaration(ctx);
            continue;
        }

        if (token.type == TokenType::Identifier)
        {
            if (token.Is(*mSource, "namespace"))
            {
                ParseNamespace(ctx);
                continue;
            }

            if (token.Is(*mSource, "class") || token.Is(*mSource, "struct"))
            {
                ParseClassOrStruct(ctx, token.Is(*mSource, "class"), "");
                continue;
            }

            if (token.Is(*mSource, "template"))
            {
                ParseTemplate(ctx);
                continue;
            }

            if (token.Is(*mSource, "enum"))
            {
                ParseEnum(ctx);
                continue;
            }

            if (token.Is(*mSource, "typedef"))
            {
                ParseTypedef(ctx);
                continue;
            }

            if (token.Is(*mSource, "using"))
            {
                ParseUsing(ctx);
                continue;
            }

            if (token.Is(*mSource, "friend"))
            {
                ParseFriend();
                continue;
            }

            if (token.Is(*mSource, "public") || token.Is(*mSource, "private") || token.Is(*mSource, "protected"))
            {
                int next = NextSignificant(mPos + 1);
                if (!AtEnd(next) && Tok(next).IsPunct(*mSource, ':'))
                {
                    if (token.Is(*mSource, "public"))
                        ctx.protection = SyntaxProtectionSection::Public;
                    else if (token.Is(*mSource, "private"))
                        ctx.protection = SyntaxProtectionSection::Private;
                    else
                        ctx.protection = SyntaxProtectionSection::Protected;

                    mPos = next + 1;
                    continue;
                }
            }

            if (token.Is(*mSource, "ATTRIBUTE_COMMENT_DEFINITION"))
            {
                ParseAttributeDefinition(ctx, false);
                continue;
            }

            if (token.Is(*mSource, "ATTRIBUTE_SHORT_DEFINITION"))
            {
                ParseAttributeDefinition(ctx, true);
                continue;
            }

            if (token.Is(*mSource, "ATTRIBUTES"))
            {
                ParseAttributesMacro(ctx);
                continue;
            }

            if (token.Is(*mSource, "PROPERTIES"))
            {
                mPos++;
                SkipToSemicolon();
                continue;
            }

            if (token.Is(*mSource, "PROPERTY") || token.Is(*mSource, "GETTER") ||
                token.Is(*mSource, "SETTER") || token.Is(*mSource, "ACCESSOR"))
            {
                ParsePropertyMacro(ctx);
                continue;
            }

            ParseMemberDeclaration(ctx);
            continue;
        }

        // Number, string or char literal at declaration level - consume as part of a member
        ParseMemberDeclaration(ctx);
    }
}

void CppSyntaxParser::ParseNamespace(SectionContext& ctx)
{
    int begin = mPos;
    mPos++;
    SkipComments();

    // Name: identifier, optionally a C++17 nested A::B::C chain
    int nameBegin = mPos;
    int nameEnd = mPos;
    while (!AtEnd(nameEnd) && Tok(nameEnd).type == TokenType::Identifier)
    {
        nameEnd++;
        if (!AtEnd(nameEnd + 1) && Tok(nameEnd).IsPunct(*mSource, ':') && Tok(nameEnd + 1).IsPunct(*mSource, ':'))
            nameEnd += 2;
        else
            break;
    }

    string name = Slice(nameBegin, nameEnd);
    mPos = nameEnd;

    // Skip to the namespace body; bail out on `namespace X = ...;` aliases
    while (!AtEnd() && !Tok(mPos).IsPunct(*mSource, '{'))
    {
        if (Tok(mPos).IsPunct(*mSource, ';'))
        {
            mPos++;
            return;
        }
        mPos++;
    }

    if (AtEnd())
        return;

    SyntaxSection& section = *ctx.section;

    SyntaxNamespace* newNamespace = mArena->Make<SyntaxNamespace>();
    newNamespace->mBegin = Tok(begin).begin;
    newNamespace->mLine = Tok(begin).line;
    newNamespace->mName = name;
    newNamespace->mFullName = section.mFullName.empty() ? name : section.mFullName + "::" + name;
    newNamespace->mFile = section.mFile;
    newNamespace->mParentSection = &section;
    section.mSections.push_back(newNamespace);

    int bodyBegin = mPos + 1;
    mPos++;

    SectionContext bodyCtx;
    bodyCtx.section = newNamespace;
    bodyCtx.protection = SyntaxProtectionSection::Public;
    ParseSectionBody(bodyCtx);

    string body = Slice(bodyBegin, mPos);
    newNamespace->mData = Trim(body, " \r\t\n");
    newNamespace->mLength = (AtEnd() ? (int)mSource->length() : Tok(mPos).end) - newNamespace->mBegin;

    if (!AtEnd())
        mPos++; // closing brace
}

void CppSyntaxParser::ParseClassOrStruct(SectionContext& ctx, bool isClass, const string& templates)
{
    int begin = mPos;
    mPos++;
    SkipComments();

    if (AtEnd() || Tok(mPos).type != TokenType::Identifier)
        return;

    int nameBegin = mPos;
    int nameLine = Tok(mPos).line;
    mPos++;

    // Template specialization: the name continues with template arguments,
    // like AnimationTrack<o2::Color4>
    if (!AtEnd() && Tok(mPos).IsPunct(*mSource, '<'))
        mPos = SkipBalancedAngles(mPos);

    string shortName = Slice(nameBegin, mPos);

    // Collect base classes list up to '{' or ';', tracking nesting for template arguments
    vector<pair<int, int>> baseRanges; // [begin, end) token ranges split by top-level commas
    int baseBegin = -1;
    int lastSignificant = -1;
    int depth = 0;
    bool sawColon = false;

    while (!AtEnd())
    {
        const Token& token = Tok(mPos);

        if (token.type == TokenType::LineComment || token.type == TokenType::BlockComment)
        {
            mPos++;
            continue;
        }

        if (token.type == TokenType::Punct)
        {
            char c = (*mSource)[token.begin];

            if (depth == 0 && (c == '{' || c == ';'))
                break;

            if (c == '<' || c == '(' || c == '[')
                depth++;
            else if (c == '>' || c == ')' || c == ']')
                depth = max(0, depth - 1);
            else if (c == ':' && depth == 0 && !sawColon)
            {
                sawColon = true;
                mPos++;
                continue;
            }
            else if (c == ',' && depth == 0)
            {
                if (baseBegin >= 0)
                    baseRanges.push_back({ baseBegin, lastSignificant + 1 });
                baseBegin = -1;
                mPos++;
                continue;
            }
        }

        if (sawColon)
        {
            if (baseBegin < 0)
                baseBegin = mPos;
            lastSignificant = mPos;
        }

        mPos++;
    }

    if (baseBegin >= 0)
        baseRanges.push_back({ baseBegin, lastSignificant + 1 });

    if (AtEnd() || Tok(mPos).IsPunct(*mSource, ';'))
    {
        // Forward declaration
        if (!AtEnd())
            mPos++;
        return;
    }

    SyntaxSection& section = *ctx.section;

    SyntaxClass* newClass = mArena->Make<SyntaxClass>();
    newClass->mBegin = Tok(begin).begin;
    newClass->mLine = nameLine;
    newClass->mName = shortName;
    newClass->mFullName = section.mFullName.empty() ? shortName : section.mFullName + "::" + shortName;
    newClass->mFile = section.mFile;
    newClass->mParentSection = &section;
    newClass->mClassSection = ctx.protection;
    newClass->mTemplateParameters = templates;
    newClass->mDefine = CurrentDefine();

    for (auto& range : baseRanges)
    {
        int i = range.first;

        if (Tok(i).Is(*mSource, "virtual"))
            i++;

        SyntaxProtectionSection baseProtection = SyntaxProtectionSection::Private;
        if (Tok(i).Is(*mSource, "public"))
        {
            baseProtection = SyntaxProtectionSection::Public;
            i++;
        }
        else if (Tok(i).Is(*mSource, "protected"))
        {
            baseProtection = SyntaxProtectionSection::Protected;
            i++;
        }
        else if (Tok(i).Is(*mSource, "private"))
            i++;

        if (Tok(i).Is(*mSource, "virtual"))
            i++;

        if (i >= range.second)
            continue;

        string baseName = Slice(i, range.second);
        newClass->mBaseClasses.push_back(SyntaxClassInheritance(Trim(baseName, " \r\n\t"), baseProtection));
    }

    section.mSections.push_back(newClass);

    int bodyBegin = mPos + 1;
    mPos++; // opening brace

    // Class members default to private, struct members to public
    SectionContext bodyCtx;
    bodyCtx.section = newClass;
    bodyCtx.protection = isClass ? SyntaxProtectionSection::Private : SyntaxProtectionSection::Public;
    ParseSectionBody(bodyCtx);

    string body = Slice(bodyBegin, mPos);
    newClass->mData = Trim(body, "{} \n\r\t");
    newClass->mLength = (AtEnd() ? (int)mSource->length() : Tok(mPos).end) - newClass->mBegin;

    if (!AtEnd())
        mPos++; // closing brace
}

void CppSyntaxParser::ParseTemplate(SectionContext& ctx)
{
    mPos++;
    SkipComments();

    if (AtEnd() || !Tok(mPos).IsPunct(*mSource, '<'))
        return;

    int paramsEnd = SkipBalancedAngles(mPos);
    string templates = mSource->substr(Tok(mPos).end, Tok(paramsEnd - 1).begin - Tok(mPos).end);
    mPos = paramsEnd;
    SkipComments();

    if (AtEnd())
        return;

    const Token& next = Tok(mPos);

    if (next.Is(*mSource, "class") || next.Is(*mSource, "struct"))
    {
        ParseClassOrStruct(ctx, next.Is(*mSource, "class"), templates);
        return;
    }

    if (next.Is(*mSource, "friend"))
    {
        ParseFriend();
        return;
    }

    if (next.Is(*mSource, "using") || next.Is(*mSource, "typedef"))
    {
        SkipToSemicolon();
        return;
    }

    ParseMemberDeclaration(ctx, templates);
}

void CppSyntaxParser::ParseEnum(SectionContext& ctx)
{
    int begin = mPos;
    mPos++;
    SkipComments();

    if (!AtEnd() && (Tok(mPos).Is(*mSource, "class") || Tok(mPos).Is(*mSource, "struct")))
    {
        mPos++;
        SkipComments();
    }

    string name;
    if (!AtEnd() && Tok(mPos).type == TokenType::Identifier)
    {
        name = Tok(mPos).Text(*mSource);
        mPos++;
    }

    // Skip optional underlying type up to the body
    while (!AtEnd() && !Tok(mPos).IsPunct(*mSource, '{'))
    {
        if (Tok(mPos).IsPunct(*mSource, ';'))
        {
            mPos++; // forward declaration
            return;
        }
        mPos++;
    }

    if (AtEnd())
        return;

    SyntaxSection& section = *ctx.section;

    SyntaxEnum* newEnum = mArena->Make<SyntaxEnum>();
    newEnum->mBegin = Tok(begin).begin;
    newEnum->mName = name;
    newEnum->mFullName = section.GetFullName().empty() ? name : section.GetFullName() + "::" + name;
    newEnum->mFile = section.mFile;
    newEnum->mOwnerSection = &section;
    newEnum->mClassSection = ctx.protection;

    mPos++; // opening brace

    while (!AtEnd() && !Tok(mPos).IsPunct(*mSource, '}'))
    {
        const Token& token = Tok(mPos);

        if (token.type == TokenType::LineComment || token.type == TokenType::BlockComment ||
            token.IsPunct(*mSource, ','))
        {
            mPos++;
            continue;
        }

        // Entry: name [= value] up to top-level ',' or '}'. Angle brackets are not
        // counted as nesting: values commonly contain shifts and comparisons like 1 << 4
        int entryBegin = mPos;
        int entryEnd = mPos; // one past the last non-comment token, keeps comments out of slices
        int eqPos = -1;
        int depth = 0;

        while (!AtEnd())
        {
            const Token& entryToken = Tok(mPos);

            if (entryToken.type == TokenType::Punct)
            {
                char c = (*mSource)[entryToken.begin];

                if (depth == 0 && (c == ',' || c == '}'))
                    break;

                if (c == '(' || c == '{' || c == '[')
                    depth++;
                else if (c == ')' || c == '}' || c == ']')
                    depth = max(0, depth - 1);
                else if (c == '=' && depth == 0 && eqPos < 0)
                    eqPos = mPos;
            }

            if (entryToken.type != TokenType::LineComment && entryToken.type != TokenType::BlockComment)
                entryEnd = mPos + 1;

            mPos++;
        }

        string entryName, entryValue;
        if (eqPos >= 0)
        {
            entryName = Slice(entryBegin, eqPos);
            entryValue = Slice(NextSignificant(eqPos + 1), entryEnd);
            Trim(entryValue, " \n\t\r");
        }
        else
            entryName = Slice(entryBegin, entryEnd);

        Trim(entryName, " \n\t\r");
        newEnum->mEntries[entryName] = entryValue;
    }

    if (!AtEnd())
    {
        newEnum->mLine = Tok(mPos).line;
        newEnum->mLength = Tok(mPos).end - newEnum->mBegin;
        mPos++; // closing brace
    }

    section.mEnums.push_back(newEnum);
}

void CppSyntaxParser::ParseTypedef(SectionContext& ctx)
{
    int begin = mPos;
    mPos++;

    int textBegin = NextSignificant(mPos);
    SkipToSemicolon();
    int textEnd = mPos - 1; // points after ';'

    string text = Slice(textBegin, textEnd);
    Trim(text, " \r\n\t");

    int lastSpace = (int)text.rfind(' ');
    if (lastSpace == (int)string::npos)
        return;

    string value = text.substr(0, lastSpace);
    Trim(value, " \r\t\n");

    string name = text.substr(lastSpace + 1);
    Trim(name, " \r\t\n;");

    if (StartsWith(value, "typename "))
        value.erase(0, strlen("typename "));

    SyntaxTypedef* newTypedef = mArena->Make<SyntaxTypedef>();
    newTypedef->mBegin = Tok(begin).begin;
    newTypedef->mLine = Tok(begin).line;
    newTypedef->mFile = mFile;
    newTypedef->mData = text;
    newTypedef->mWhatName = value;
    newTypedef->mNewDefName = name;

    ctx.section->mTypedefs.push_back(newTypedef);
}

void CppSyntaxParser::ParseUsing(SectionContext& ctx)
{
    int begin = mPos;
    mPos++;

    int textBegin = NextSignificant(mPos);
    SkipToSemicolon();
    int textEnd = mPos - 1;

    // Only plain `using namespace X;` matters for name resolution; qualified names
    // and alias declarations are ignored
    if (textEnd - textBegin != 2 || !Tok(textBegin).Is(*mSource, "namespace") ||
        Tok(textBegin + 1).type != TokenType::Identifier)
    {
        return;
    }

    SyntaxUsingNamespace* newUsing = mArena->Make<SyntaxUsingNamespace>();
    newUsing->mBegin = Tok(begin).begin;
    newUsing->mLine = Tok(begin).line;
    newUsing->mFile = mFile;
    newUsing->mUsingNamespaceName = Tok(textBegin + 1).Text(*mSource);

    ctx.section->mUsingNamespaces.push_back(newUsing);
}

void CppSyntaxParser::ParseFriend()
{
    mPos++;
    SkipToSemicolon();
}

void CppSyntaxParser::ParseLineComments(SectionContext& ctx)
{
    const Token& first = Tok(mPos);

    // Trailing comment on the last member's line is attached to it alone, without merging
    // with the following lines. It never becomes pending: it belongs to that member even
    // when the member already has a comment attached
    if (ctx.lastMember && first.line == ctx.lastMember->mLine)
    {
        if (!ctx.lastMember->mComment)
        {
            SyntaxComment* comment = mArena->Make<SyntaxComment>();
            string text = mSource->substr(first.begin + 2, first.end - first.begin - 2);
            comment->mData = Trim(text, " \r");
            comment->mBegin = first.begin;
            comment->mLength = first.Length();
            comment->mLine = first.line;
            comment->mFile = ctx.section->mFile;

            ctx.lastMember->mComment = comment;
        }

        mPos++;
        return;
    }

    SyntaxComment* comment = mArena->Make<SyntaxComment>();
    comment->mBegin = first.begin;

    string data;
    int line = 0;

    while (!AtEnd())
    {
        const Token& token = Tok(mPos);

        string text = mSource->substr(token.begin + 2, token.end - token.begin - 2);
        data += Trim(text, " \r");
        line = token.line;

        mPos++;

        // Merge directly adjacent comment lines into one comment
        if (!AtEnd() && Tok(mPos).type == TokenType::LineComment && Tok(mPos).line == token.line + 1)
            data += '\n';
        else
            break;
    }

    comment->mData = data;
    comment->mLine = line;
    comment->mFile = ctx.section->mFile;
    comment->mLength = (mPos > 0 ? Tok(mPos - 1).end : 0) - comment->mBegin;

    ctx.pendingComment = comment;
}

void CppSyntaxParser::ParseBlockComment(SectionContext& ctx)
{
    const Token& token = Tok(mPos);

    int textBegin = token.begin + 2;
    int textEnd = max(token.end - 2, textBegin);

    SyntaxComment* comment = mArena->Make<SyntaxComment>();
    string text = mSource->substr(textBegin, textEnd - textBegin);
    comment->mData = Trim(text, " \r\t\n");
    comment->mBegin = token.begin;
    comment->mLength = token.Length();
    comment->mFile = ctx.section->mFile;

    // End line matters for attachment-above; start line for trailing attachment
    int endLine = token.line;
    for (int i = token.begin; i < token.end; i++)
    {
        if ((*mSource)[i] == '\n')
            endLine++;
    }
    comment->mLine = endLine;

    PlaceComment(ctx, comment, token.line);

    mPos++;
}

void CppSyntaxParser::PlaceComment(SectionContext& ctx, SyntaxComment* comment, int startLine)
{
    if (ctx.lastMember && startLine == ctx.lastMember->mLine)
    {
        // Trailing comment belongs to the last member; dropped if one is already attached
        if (!ctx.lastMember->mComment)
            ctx.lastMember->mComment = comment;
    }
    else
        ctx.pendingComment = comment;
}

void CppSyntaxParser::HandleDirective(SectionContext& ctx)
{
    const Token& token = Tok(mPos);

    // Text right after '#'; condition text is taken verbatim to the end of line,
    // including original spacing - generated #if lines must match older output
    int kwBegin = token.begin + 1;
    auto restAfter = [&](int kwLength) {
        return mSource->substr(kwBegin + kwLength, token.end - (kwBegin + kwLength));
    };
    auto isKw = [&](const char* kw) {
        return mSource->compare(kwBegin, strlen(kw), kw) == 0;
    };

    auto makeDefine = [&](const string& definition) {
        SyntaxDefineIf* define = mArena->Make<SyntaxDefineIf>();
        define->mDefinition = definition;
        define->mFile = mFile;
        define->mLine = token.line;
        return define;
    };

    if (isKw("ifdef"))
        mDefinesStack.push_back(makeDefine("defined " + restAfter(5)));
    else if (isKw("ifndef"))
    {
        string name = restAfter(6);
        mDefinesStack.push_back(makeDefine("!defined(" + Trim(name, " \t\r") + ")"));
    }
    else if (isKw("if"))
        mDefinesStack.push_back(makeDefine(restAfter(2)));
    else if (isKw("endif"))
    {
        if (!mDefinesStack.empty())
            mDefinesStack.pop_back();
    }
    else if (isKw("elif"))
    {
        SyntaxDefineIf* define = makeDefine(restAfter(4));
        if (!mDefinesStack.empty())
            mDefinesStack.back() = define;
        else
            mDefinesStack.push_back(define);
    }
    else if (isKw("else"))
    {
        if (!mDefinesStack.empty())
            mDefinesStack.back() = makeDefine("!(" + mDefinesStack.back()->mDefinition + ")");
    }
    // #pragma, #include, #define, #undef are ignored
}

void CppSyntaxParser::ParseAttributeDefinition(SectionContext& ctx, bool shortDefinition)
{
    mPos++;

    string value;
    while (!AtEnd() && !Tok(mPos).IsPunct(*mSource, ';'))
    {
        if (Tok(mPos).IsPunct(*mSource, '}'))
            break;

        if (Tok(mPos).type == TokenType::String)
            value = mSource->substr(Tok(mPos).begin + 1, Tok(mPos).Length() - 2);

        mPos++;
    }

    if (!AtEnd() && Tok(mPos).IsPunct(*mSource, ';'))
        mPos++;

    if (SyntaxClass* classSection = dynamic_cast<SyntaxClass*>(ctx.section))
    {
        if (shortDefinition)
            classSection->mAttributeShortDef = value;
        else
            classSection->mAttributeCommentDef = value;
    }
}

bool CppSyntaxParser::ReadMacroArguments(vector<string>& args)
{
    SkipComments();
    if (AtEnd() || !Tok(mPos).IsPunct(*mSource, '('))
        return false;

    mPos++;

    int argBegin = mPos;
    int depth = 0;

    while (!AtEnd())
    {
        const Token& token = Tok(mPos);

        if (token.type == TokenType::Punct)
        {
            char c = (*mSource)[token.begin];

            if (depth == 0 && c == ')')
                break;

            if (c == '(' || c == '{' || c == '[' || c == '<')
                depth++;
            else if (c == ')' || c == '}' || c == ']' || c == '>')
                depth = max(0, depth - 1); // clamped: '>' shows up as '->' or comparison in arguments
            else if (c == ',' && depth == 0)
            {
                string arg = Slice(argBegin, mPos);
                args.push_back(Trim(arg, " \n\r\t,"));
                argBegin = mPos + 1;
            }
        }

        mPos++;
    }

    string arg = Slice(argBegin, mPos);
    args.push_back(Trim(arg, " \n\r\t,"));

    if (!AtEnd())
        mPos++; // closing parenthesis

    return true;
}

void CppSyntaxParser::SkipToSemicolon()
{
    int depth = 0;
    while (!AtEnd())
    {
        const Token& token = Tok(mPos);

        if (token.type == TokenType::Punct)
        {
            char c = (*mSource)[token.begin];

            if (depth == 0 && c == ';')
            {
                mPos++;
                return;
            }

            // Unmatched '}' closes the enclosing section - stop before it
            if (depth == 0 && c == '}')
                return;

            if (c == '(' || c == '{' || c == '[')
                depth++;
            else if (c == ')' || c == '}' || c == ']')
                depth = max(0, depth - 1);
        }

        mPos++;
    }
}

int CppSyntaxParser::SkipToSemicolonLine()
{
    SkipToSemicolon();
    int lastIdx = min(mPos, (int)mTokens.size()) - 1;
    return lastIdx >= 0 ? Tok(lastIdx).line : 0;
}

void CppSyntaxParser::ParseAttributesMacro(SectionContext& ctx)
{
    int begin = mPos;
    mPos++;

    vector<string> args;
    if (!ReadMacroArguments(args))
        return;

    int line = SkipToSemicolonLine();

    SyntaxAttributes* attributes = mArena->Make<SyntaxAttributes>();
    attributes->mBegin = Tok(begin).begin;
    attributes->mLine = line;
    attributes->mFile = ctx.section->mFile;
    attributes->mAttributesList = args;

    ctx.pendingAttributes = attributes;
}

void CppSyntaxParser::ParsePropertyMacro(SectionContext& ctx)
{
    int begin = mPos;
    int startLine = Tok(begin).line;
    mPos++;

    vector<string> args;
    if (!ReadMacroArguments(args))
        return;

    int line = SkipToSemicolonLine();

    if (args.size() < 2)
        return;

    SyntaxVariable* variable = mArena->Make<SyntaxVariable>();
    variable->mBegin = Tok(begin).begin;
    variable->mLine = line;
    variable->mFile = ctx.section->mFile;
    variable->mName = args[1];
    variable->mType.mName = args[0];
    variable->mDefine = CurrentDefine();

    ctx.section->mVariables.push_back(variable);

    RegisterMember(ctx, variable, startLine);
}

void CppSyntaxParser::RegisterMember(SectionContext& ctx, ISyntaxExpression* member, int startLine)
{
    // Lines only grow during parsing, so a pending item is either adjacent to this member
    // or will never be adjacent to any - it can be consumed in both cases
    if (ctx.pendingComment &&
        (ctx.pendingComment->mLine == startLine - 1 || ctx.pendingComment->mLine == startLine))
    {
        member->mComment = ctx.pendingComment;
    }
    ctx.pendingComment = nullptr;

    if (ctx.pendingAttributes && ctx.pendingAttributes->mLine == startLine - 1)
        member->mAttributesMacro = ctx.pendingAttributes;
    ctx.pendingAttributes = nullptr;

    ctx.lastMember = member;
}

void CppSyntaxParser::ParseMemberDeclaration(SectionContext& ctx, const string& templates /*= ""*/)
{
    int declBegin = mPos;

    // A stray brace block at declaration level - skip it whole
    if (Tok(mPos).IsPunct(*mSource, '{'))
    {
        int depth = 0;
        while (!AtEnd())
        {
            if (Tok(mPos).IsPunct(*mSource, '{'))
                depth++;
            else if (Tok(mPos).IsPunct(*mSource, '}'))
            {
                depth--;
                if (depth == 0)
                {
                    mPos++;
                    return;
                }
            }
            mPos++;
        }
        return;
    }

    // Declaration runs to the first top-level ';', or through a brace block
    // (function body or braced initializer)
    int stop = -1;
    int sigEnd = -1;
    bool consumeStop = true;

    int i = declBegin;
    for (; !AtEnd(i); i++)
    {
        if (Tok(i).type != TokenType::Punct)
            continue;

        char c = (*mSource)[Tok(i).begin];

        if (c == ';')
        {
            stop = i;
            sigEnd = i;
            break;
        }

        // Unmatched '}' closes the enclosing section - the declaration can't cross it
        if (c == '}')
        {
            stop = i;
            sigEnd = i;
            consumeStop = false;
            break;
        }

        if (c == '{')
        {
            sigEnd = i;

            int fgBraces = 0, braces = 0, sqBraces = 0;
            for (; !AtEnd(i); i++)
            {
                if (Tok(i).type != TokenType::Punct)
                    continue;

                char bc = (*mSource)[Tok(i).begin];
                switch (bc)
                {
                case '{': fgBraces++; break;
                case '}': fgBraces--; break;
                case '(': braces++; break;
                case ')': braces--; break;
                case '[': sqBraces++; break;
                case ']': sqBraces--; break;
                }

                if (bc == '}' && fgBraces == 0 && braces == 0 && sqBraces == 0)
                    break;
            }

            stop = min(i, (int)mTokens.size() - 1);
            break;
        }
    }

    int endLine;
    if (stop < 0)
    {
        stop = (int)mTokens.size() - 1;
        sigEnd = (int)mTokens.size();
        endLine = mTokens.empty() ? 0 : mTokens.back().line;
        mPos = (int)mTokens.size();
    }
    else
    {
        endLine = Tok(stop).line;
        mPos = consumeStop ? stop + 1 : stop;
    }

    ParseMemberSignature(ctx, declBegin, sigEnd, templates, declBegin, endLine);
}

void CppSyntaxParser::ParseMemberSignature(SectionContext& ctx, int first, int sigEnd, const string& templates,
                                           int declBegin, int endLine)
{
    int i = NextSignificant(first);
    if (i >= sigEnd)
        return;

    bool isVirtual = false, isStatic = false;

    while (i < sigEnd && Tok(i).type == TokenType::Identifier)
    {
        if (Tok(i).Is(*mSource, "virtual"))
            isVirtual = true;
        else if (Tok(i).Is(*mSource, "static"))
            isStatic = true;
        else if (!Tok(i).Is(*mSource, "inline") && !Tok(i).Is(*mSource, "explicit") &&
                 !Tok(i).Is(*mSource, "constexpr") && !Tok(i).Is(*mSource, "typename"))
        {
            break;
        }

        i = NextSignificant(i + 1);
    }

    if (i >= sigEnd)
        return;

    auto finalize = [&](ISyntaxExpression* member) {
        member->mBegin = Tok(declBegin).begin;
        member->mLength = Tok(min(sigEnd, (int)mTokens.size()) - 1).end - member->mBegin;
        member->mLine = endLine;
        member->mFile = ctx.section->mFile;
        member->mData = Slice(declBegin, sigEnd);
        RegisterMember(ctx, member, Tok(declBegin).line);
    };

    auto makeFunction = [&](const string& name, const ParsedType& returnType, int parenIdx) {
        SyntaxFunction* function = mArena->Make<SyntaxFunction>();
        function->mName = name;
        function->mReturnType.mName = returnType.text;
        function->mReturnType.mIsConstant = returnType.isConst;
        function->mReturnType.mIsReference = !returnType.text.empty() && returnType.text.back() == '&';
        function->mReturnType.mIsPointer = !returnType.text.empty() && returnType.text.back() == '*';
        function->mIsStatic = isStatic;
        function->mIsVirtual = isVirtual;
        function->mClassSection = ctx.protection;
        function->mDefine = CurrentDefine();
        function->mTemplates = templates;

        if (parenIdx >= 0 && parenIdx < sigEnd)
            ParseFunctionParameters(function, parenIdx, sigEnd);

        ctx.section->mFunctions.push_back(function);
        finalize(function);
    };

    auto findParen = [&](int from) {
        for (int k = from; k < sigEnd; k++)
        {
            if (Tok(k).IsPunct(*mSource, '('))
                return k;
        }
        return -1;
    };

    ParsedType voidType;
    voidType.text = "void";
    voidType.valid = true;

    // Destructor
    if (Tok(i).IsPunct(*mSource, '~'))
    {
        int nameIdx = NextSignificant(i + 1);
        string name = "~";
        if (nameIdx < sigEnd && Tok(nameIdx).type == TokenType::Identifier)
            name += Tok(nameIdx).Text(*mSource);

        makeFunction(name, voidType, findParen(i));
        return;
    }

    // Conversion operator: `operator bool() const`
    if (Tok(i).Is(*mSource, "operator"))
    {
        int parenIdx = findParen(i);
        string name = parenIdx > 0 ? Slice(i, parenIdx) : Slice(i, sigEnd);
        makeFunction(name, voidType, parenIdx);
        return;
    }

    ParsedType type = ParseType(i, sigEnd);
    if (!type.valid)
        return;

    int next = NextSignificant(i);
    if (next >= sigEnd)
        return; // type without a name - nothing to register

    if (Tok(next).IsPunct(*mSource, '('))
    {
        int inside = NextSignificant(next + 1);

        // Pointer to member like void(Class::*name)(): '*' right after '::' - not reflectable
        int close = min(FindMatchingParen(next), sigEnd);
        for (int k = next + 1; k < close; k++)
        {
            if (Tok(k).IsPunct(*mSource, '*') && k > next + 1 && Tok(k - 1).IsPunct(*mSource, ':'))
                return;
        }

        if (inside < sigEnd && Tok(inside).IsPunct(*mSource, '*'))
        {
            // Function pointer variable: type (*name)(params)
            if (!templates.empty())
                return;

            int close = FindMatchingParen(next);
            int nameIdx = NextSignificant(inside + 1);

            SyntaxVariable* variable = mArena->Make<SyntaxVariable>();
            if (nameIdx < close && Tok(nameIdx).type == TokenType::Identifier)
                variable->mName = Tok(nameIdx).Text(*mSource);

            string paramsInner;
            int paramsParen = NextSignificant(close + 1);
            if (paramsParen < sigEnd && Tok(paramsParen).IsPunct(*mSource, '('))
                paramsInner = Slice(paramsParen + 1, FindMatchingParen(paramsParen));

            variable->mType.mName = type.text + " (*)(" + paramsInner + ")";
            variable->mType.mIsConstant = type.isConst;
            variable->mIsStatic = isStatic;
            variable->mClassSection = ctx.protection;
            variable->mDefine = CurrentDefine();

            ctx.section->mVariables.push_back(variable);
            finalize(variable);
            return;
        }

        // Constructor or a macro marker like IOBJECT(...): function named as the type
        makeFunction(type.text, voidType, next);
        return;
    }

    if (Tok(next).type == TokenType::Identifier)
    {
        // Operator with return type: `bool operator==(...)`
        if (Tok(next).Is(*mSource, "operator"))
        {
            int parenIdx = findParen(next);
            string name = parenIdx > 0 ? Slice(next, parenIdx) : Slice(next, sigEnd);
            makeFunction(name, type, parenIdx);
            return;
        }

        string name = Tok(next).Text(*mSource);
        int j = NextSignificant(next + 1);

        if (j < sigEnd && Tok(j).IsPunct(*mSource, '('))
        {
            makeFunction(name, type, j);
            return;
        }

        // Template variables aren't registered
        if (!templates.empty())
            return;

        SyntaxVariable* variable = mArena->Make<SyntaxVariable>();
        variable->mName = name;
        variable->mType.mName = type.text;
        variable->mType.mIsConstant = type.isConst;
        variable->mType.mIsMutable = type.isMutable;
        variable->mType.mIsReference = !type.text.empty() && type.text.back() == '&';
        variable->mType.mIsPointer = !type.text.empty() && type.text.back() == '*';
        variable->mIsStatic = isStatic;
        variable->mClassSection = ctx.protection;
        variable->mDefine = CurrentDefine();

        // Default value after '='
        for (int k = j; k < sigEnd; k++)
        {
            if (Tok(k).IsPunct(*mSource, '='))
            {
                int valueBegin = NextSignificant(k + 1);
                if (valueBegin < sigEnd)
                    variable->mDefaultValue = Slice(valueBegin, sigEnd);
                break;
            }
        }

        ctx.section->mVariables.push_back(variable);
        finalize(variable);
        return;
    }

    // Anything else (unnamed bitfields, unrecognized constructs) - not registered
}

CppSyntaxParser::ParsedType CppSyntaxParser::ParseType(int& i, int endTok) const
{
    ParsedType res;

    // Leading const/mutable are flags, not part of the type text
    while (i < endTok && Tok(i).type == TokenType::Identifier)
    {
        if (Tok(i).Is(*mSource, "const"))
            res.isConst = true;
        else if (Tok(i).Is(*mSource, "mutable"))
            res.isMutable = true;
        else
            break;

        i = NextSignificant(i + 1);
    }

    if (i >= endTok || Tok(i).type != TokenType::Identifier)
        return res;

    static auto isFundamental = [](const string& word) {
        return word == "unsigned" || word == "signed" || word == "long" || word == "short" ||
            word == "int" || word == "char" || word == "double" || word == "float" || word == "bool";
    };

    int typeBegin = i;
    string firstWord = Tok(i).Text(*mSource);
    i++;

    // Multi-word fundamental types like `unsigned int`
    if (isFundamental(firstWord))
    {
        while (i < endTok && Tok(i).type == TokenType::Identifier && isFundamental(Tok(i).Text(*mSource)))
            i++;
    }

    // Qualified names and template arguments: A::B<...>::C
    while (i < endTok)
    {
        if (Tok(i).IsPunct(*mSource, '<'))
        {
            i = min(SkipBalancedAngles(i), endTok);
            continue;
        }

        if (i + 2 < endTok && Tok(i).IsPunct(*mSource, ':') && Tok(i + 1).IsPunct(*mSource, ':') &&
            Tok(i + 2).type == TokenType::Identifier)
        {
            i += 3;
            continue;
        }

        break;
    }

    // Pointer and reference suffixes; `const` after them (like `Foo* const`) is consumed
    // but stays out of the type text
    int typeEnd = i;
    while (i < endTok)
    {
        if (Tok(i).IsPunct(*mSource, '*') || Tok(i).IsPunct(*mSource, '&'))
        {
            i++;
            typeEnd = i;
            continue;
        }

        if (Tok(i).Is(*mSource, "const"))
        {
            i = NextSignificant(i + 1);
            continue;
        }

        break;
    }

    res.text = Slice(typeBegin, typeEnd);
    res.valid = true;

    return res;
}

void CppSyntaxParser::ParseFunctionParameters(SyntaxFunction* function, int parenIdx, int sigEnd)
{
    int close = FindMatchingParen(parenIdx);

    int rangeBegin = parenIdx + 1;
    int depth = 0;

    for (int i = parenIdx + 1; i <= close && !AtEnd(i); i++)
    {
        bool atClose = i == close;
        bool split = atClose;

        if (!atClose && Tok(i).type == TokenType::Punct)
        {
            char c = (*mSource)[Tok(i).begin];

            if (c == '(' || c == '{' || c == '[' || c == '<')
                depth++;
            else if (c == ')' || c == '}' || c == ']' || c == '>')
                depth = max(0, depth - 1); // clamped: '>' shows up as '->' or comparison in default arguments
            else if (c == ',' && depth == 0)
                split = true;
        }

        if (split)
        {
            if (NextSignificant(rangeBegin) < i)
                function->mParameters.push_back(ParseParameter(rangeBegin, i));

            rangeBegin = i + 1;
        }
    }

    // Trailing const
    int after = NextSignificant(close + 1);
    if (after < sigEnd && !AtEnd(after) && Tok(after).Is(*mSource, "const"))
        function->mIsConstant = true;
}

SyntaxVariable* CppSyntaxParser::ParseParameter(int first, int last)
{
    SyntaxVariable* param = mArena->Make<SyntaxVariable>();
    param->mFile = mFile;
    param->mData = Slice(first, last);

    int i = NextSignificant(first);
    ParsedType type = ParseType(i, last);

    if (!type.valid)
    {
        // Varargs and other unrecognized parameters - keep raw text as the type
        string text = Slice(first, last);
        param->mType.mName = Trim(text, " \r\n\t");
        return param;
    }

    int next = NextSignificant(i);

    if (next < last && Tok(next).IsPunct(*mSource, '('))
    {
        // Function pointer parameter: type (*name)(params)
        int close = min(FindMatchingParen(next), last);
        int nameIdx = NextSignificant(next + 2);

        if (nameIdx < close && Tok(nameIdx).type == TokenType::Identifier)
            param->mName = Tok(nameIdx).Text(*mSource);

        string paramsInner;
        int paramsParen = NextSignificant(close + 1);
        if (paramsParen < last && Tok(paramsParen).IsPunct(*mSource, '('))
            paramsInner = Slice(paramsParen + 1, min(FindMatchingParen(paramsParen), last));

        param->mType.mName = type.text + " (*)(" + paramsInner + ")";
        param->mType.mIsConstant = type.isConst;
        return param;
    }

    param->mType.mName = type.text;
    param->mType.mIsConstant = type.isConst;
    param->mType.mIsReference = !type.text.empty() && type.text.back() == '&';
    param->mType.mIsPointer = !type.text.empty() && type.text.back() == '*';

    if (next < last && Tok(next).type == TokenType::Identifier)
    {
        param->mName = Tok(next).Text(*mSource);
        next = NextSignificant(next + 1);
    }

    // Default value after '='
    for (int k = next; k < last; k++)
    {
        if (Tok(k).IsPunct(*mSource, '='))
        {
            int valueBegin = NextSignificant(k + 1);
            if (valueBegin < last)
                param->mDefaultValue = Slice(valueBegin, last);
            break;
        }
    }

    return param;
}
