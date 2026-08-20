#pragma once

#include <string>
#include <vector>

using namespace std;

enum class TokenType
{
    Identifier,   // Identifiers and keywords
    Number,       // Numeric literal
    String,       // "..." literal, text includes quotes
    CharLiteral,  // '...' literal, text includes quotes
    Punct,        // Single punctuation character
    LineComment,  // // to end of line, text excludes newline
    BlockComment, // /* ... */
    Directive     // # to end of line; no line continuations, matching how metas were generated before
};

struct Token
{
    TokenType type = TokenType::Punct;

    int begin = 0; // Offset of first character in source
    int end = 0;   // Offset after last character
    int line = 0;  // 0-based line of token start

    string Text(const string& source) const { return source.substr(begin, end - begin); }
    int Length() const { return end - begin; }

    bool Is(const string& source, const char* text) const;
    bool IsPunct(const string& source, char c) const;
};

// Single-pass tokenizer. Comments and preprocessor directives are kept as tokens:
// the parser needs comments for meta attributes and directives for #if tracking.
vector<Token> Tokenize(const string& source);
