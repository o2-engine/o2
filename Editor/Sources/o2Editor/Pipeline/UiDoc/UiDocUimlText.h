#pragma once

#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocUiml.h"

// ------------------------------------------------------------------------------------------------
// The text helpers the UIML reader (UiDocUiml.cpp) and the edit commands (UiDocUimlEdits.cpp) share;
// not for other users
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc::UimlText
{
    // ------------------------------------------------------------
    // One meaningful line: its indent (code points), text and 1-based number
    // ------------------------------------------------------------
    struct UimlLine
    {
        int    indent = 0;
        String text;
        int    no = 0;
    };

    // Fields a key=value gives: a field with no value (JavaScript's undefined) removes the key when assigned
    using UimlProps = Vector<Pair<String, std::optional<Json>>>;

    // A key alias (width -> w...) by the key's lower case; the key as written when it has none
    String AliasKey(const String& key);

    // Splits on any of the code points (A-Z matched as a-z)
    Vector<String> SplitOn(const String& text, const std::u32string& separators);

    // A key=value from the text -> node fields (several for the shorthands)
    UimlProps PropsOf(const String& key, const String& value);

    // Object.assign(target, props) with undefined values dropping the key
    void AssignProps(Json& target, const UimlProps& props);

    // The meaningful lines of a text (no blank lines, # comments or ``` fences), tabs as two spaces
    Vector<UimlLine> LinesOf(const String& src);

    // /^\d+\s*[x\u00d7]\s*\d+$/i: the two numbers
    bool MatchDims(const String& text, double& w, double& h);

    // A text in double quotes with \\, \" and \n escaped
    String Quote(const String& s);
}
