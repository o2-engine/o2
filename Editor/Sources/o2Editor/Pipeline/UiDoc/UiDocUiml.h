#pragma once

#include "o2Editor/Pipeline/UiDoc/UiDocModel.h"

// ------------------------------------------------------------------------------------------------
// UIML - the layout as indented text (port of AssetsLine shared/uiDoc/uiml.ts). One line per block,
// children indented under their parent:
//
//   screen MainMenu 1080x1920
//     background bg anchor=stretch kit=bg_forest
//     column menu anchor=center size=640x900 gap=28 align=stretch
//       text title h=150 style=title "FOREST QUEST"
//
// A line is `<type> <id> key=value... "text"`. Shorthands: size=WxH, pos=X,Y, cell=WxH, gap=X,Y (grid),
// margin / pad = 1, 2 or 4 numbers in CSS order. DocToUiml and ParseUiml round-trip exactly.
// Edits (the agent's answer) use the same lines: @update, @add, @remove, @move, @replace, @screen
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // ------------------------------------------------------------------------
    // One token of a line: a bare word, a "quoted text", key=value or key="text"
    // ------------------------------------------------------------------------
    struct UimlToken
    {
        String key;           // "" = none (an empty key counts as none, as in the web)
        String value;
        bool   quoted = false;
    };

    // One line -> tokens (# starts a comment; "...", "..." and <<...>> quote)
    Vector<UimlToken> TokenizeUiml(const String& line);

    // ---------------------------------------
    // A document parsed from text, with problems
    // ---------------------------------------
    struct UimlParseResult
    {
        UiDoc          doc;
        Vector<String> errors;
    };

    // Text -> document. Without a `screen` line the blocks go into an empty screen of the default size (or of base:
    // its name, size and dpi)
    UimlParseResult ParseUiml(const String& src, const UiDoc* base = nullptr);

    // Text of one or more blocks -> clean nodes, ids kept unique against book.used (which takes them)
    std::vector<UiNode> ParseUimlNodes(const String& src, UiIdBook& book, Vector<String>& errors);

    // The props of a node as key=value tokens, in the canonical order (skipLayout: the layout is the line's type word)
    Vector<String> PropTokens(const UiNode& node, bool skipLayout = false);

    // One block and its children as text, indented by depth (two spaces a level), no trailing newline
    String NodeToUiml(const UiNode& node, int depth = 0);

    // The whole screen as text, ending with a newline
    String DocToUiml(const UiDoc& doc);

    // -----------------------------------------
    // The agent's edit lines as ops, with problems
    // -----------------------------------------
    struct UimlEditsResult
    {
        Vector<UiOp>   ops;
        Vector<String> errors;
    };

    // The agent's edit lines -> ops (UiDocOps.h) for the document they apply to (new blocks get ids unique against it).
    // Lines that make no sense are reported, the rest still apply
    UimlEditsResult ParseEdits(const String& src, const UiDoc& doc);

    // The fields of an update (a JSON object), each cleaned the way a node's are (no layout pruning), as a node
    UiNode CleanSet(const Json& set);

    // Ops -> the edit lines that would produce them (history view, the model's memory)
    String OpsToText(const Vector<UiOp>& ops);
}
