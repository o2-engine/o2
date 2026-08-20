#include "Lexer.h"

#include <cstring>

bool Token::Is(const string& source, const char* text) const
{
    int len = (int)strlen(text);
    if (Length() != len)
        return false;

    return source.compare(begin, len, text) == 0;
}

bool Token::IsPunct(const string& source, char c) const
{
    return type == TokenType::Punct && source[begin] == c;
}

static bool IsIdentifierStart(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool IsIdentifierChar(char c)
{
    return IsIdentifierStart(c) || (c >= '0' && c <= '9');
}

static bool IsDigit(char c)
{
    return c >= '0' && c <= '9';
}

// Only the classic preprocessor keywords make a directive token; anything else after '#'
// (like ObjC '#import') is left to the declaration parser, which skips it as unknown text
static bool IsDirectiveStart(const string& source, int i)
{
    static const char* keywords[] = { "pragma", "include", "define", "undef", "if", "endif", "elif", "else" };

    for (auto keyword : keywords)
    {
        if (source.compare(i, strlen(keyword), keyword) == 0)
            return true;
    }

    return false;
}

vector<Token> Tokenize(const string& source)
{
    vector<Token> tokens;
    tokens.reserve(source.length()/4);

    int length = (int)source.length();
    int line = 0;

    int i = 0;
    while (i < length)
    {
        char c = source[i];

        if (c == '\n')
        {
            line++;
            i++;
            continue;
        }

        if (c == ' ' || c == '\t' || c == '\r')
        {
            i++;
            continue;
        }

        Token token;
        token.begin = i;
        token.line = line;

        if (c == '/' && i + 1 < length && source[i + 1] == '/')
        {
            token.type = TokenType::LineComment;
            i += 2;
            while (i < length && source[i] != '\n')
                i++;
        }
        else if (c == '/' && i + 1 < length && source[i + 1] == '*')
        {
            token.type = TokenType::BlockComment;
            i += 2;
            while (i < length && !(source[i] == '*' && i + 1 < length && source[i + 1] == '/'))
            {
                if (source[i] == '\n')
                    line++;
                i++;
            }
            i = min(i + 2, length);
        }
        else if (c == '#' && i + 1 < length && IsDirectiveStart(source, i + 1))
        {
            token.type = TokenType::Directive;
            i++;
            while (i < length && source[i] != '\n')
                i++;
        }
        else if (IsIdentifierStart(c))
        {
            token.type = TokenType::Identifier;
            while (i < length && IsIdentifierChar(source[i]))
                i++;

            // Raw string literal R"delim( ... )delim" - the prefix ends with R right before the quote
            if (i < length && source[i] == '"' && source[i - 1] == 'R')
            {
                token.type = TokenType::String;
                i++;

                int delimBegin = i;
                while (i < length && source[i] != '(' && source[i] != '\n')
                    i++;

                string closer = ")" + source.substr(delimBegin, i - delimBegin) + "\"";
                size_t closePos = source.find(closer, i);
                int end = closePos == string::npos ? length : (int)(closePos + closer.length());

                for (; i < end; i++)
                {
                    if (source[i] == '\n')
                        line++;
                }
            }
        }
        else if (IsDigit(c) || (c == '.' && i + 1 < length && IsDigit(source[i + 1])))
        {
            token.type = TokenType::Number;
            while (i < length && (IsIdentifierChar(source[i]) || source[i] == '.'))
                i++;
        }
        else if (c == '"')
        {
            token.type = TokenType::String;
            i++;
            while (i < length && source[i] != '"')
            {
                if (source[i] == '\\' && i + 1 < length)
                {
                    if (source[i + 1] == '\n')
                        line++;
                    i++;
                }
                else if (source[i] == '\n')
                    line++;
                i++;
            }
            i = min(i + 1, length);
        }
        else if (c == '\'')
        {
            token.type = TokenType::CharLiteral;
            i++;
            while (i < length && source[i] != '\'')
            {
                if (source[i] == '\\' && i + 1 < length)
                {
                    if (source[i + 1] == '\n')
                        line++;
                    i++;
                }
                else if (source[i] == '\n')
                    line++;
                i++;
            }
            i = min(i + 1, length);
        }
        else
        {
            token.type = TokenType::Punct;
            i++;
        }

        token.end = i;
        tokens.push_back(token);
    }

    return tokens;
}
