#pragma once

#include <string>

#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"

using namespace o2;

// ------------------------------------------------------------------------------------------------
// JavaScript semantics the UI layout port (AssetsLine shared/uiDoc, shared/uiKit.ts, shared/textStyle.ts)
// depends on: Math.round, Number() / String() of loosely typed values, truthiness, string slicing by
// UTF-16 units, Unicode whitespace, trimming and case mapping, ISO dates. Values of unknown shape (a
// node's config, an edit's fields, a model's answer) are UiLayout::Json: ordered objects, JS number
// formatting. Files of this port turn floating-point contraction off, so a*b+c rounds twice as in V8
// and the solver's numbers are the web's to the last bit.
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    using Json = UiLayout::Json;

    // JavaScript's Math.round: the nearest integer, halves toward +infinity (exact also for 0.49999999999999994)
    double JsRound(double v);

    // JavaScript's String(number): the shortest form that reads back the same, integers without ".0"
    String JsNum(double v);

    // JavaScript's Number(value) of a member that may be missing: NaN for a missing one (undefined)
    double JsNumber(const Json* v);

    // JavaScript's String(value): "undefined" for a missing member
    String JsString(const Json* v);

    // JavaScript's String(value ?? def): def for a missing member or null
    String JsStringOr(const Json* v, const String& def = "");

    // JavaScript's truthiness: false for a missing member, null, false, 0, NaN and ""
    bool JsTruthy(const Json* v);

    // True for a missing member or null (JavaScript's `v == null`)
    bool JsNullish(const Json* v);

    // True for what `typeof v === 'object' && v` accepts: objects and arrays
    bool JsIsObjectLike(const Json* v);

    // The member of an object, null when the value is no object or has no such member
    const Json* JsGet(const Json& obj, const char* key);
    const Json* JsGet(const Json* obj, const char* key);

    // A number as a JSON value, the way JSON.stringify writes it (NaN and infinities as null)
    inline Json JsonNum(double v) { return Json(v); }

    // a === b for JSON values: primitives by value, objects and arrays never (they are never the same object here)
    bool JsStrictEqual(const Json* a, const Json* b);

    // A list of strings as a JSON array
    Json JsonStrings(const Vector<String>& items);

    // The strings of a JSON array (anything else gives an empty list), each through String()
    Vector<String> JsonToStrings(const Json* v);

    // --- text --------------------------------------------------------------------------------

    // Code points of UTF-8 text (invalid bytes become U+FFFD)
    std::u32string Utf8ToCodePoints(const String& text);

    // UTF-8 text of code points
    String CodePointsToUtf8(const std::u32string& cps);

    // UTF-8 of one code point
    String CodePointToUtf8(char32_t cp);

    // The length JavaScript gives the text: UTF-16 code units
    int Utf16Length(const String& text);

    // JavaScript's text.slice(start, end) with indices in UTF-16 units; a surrogate pair cut in half is dropped
    // (JavaScript would keep a lone surrogate, which UTF-8 cannot carry)
    String JsSlice(const String& text, int start, int end = INT32_MAX);

    // True for what JavaScript's \s matches
    bool IsJsSpace(char32_t c);

    // JavaScript's trim / trimStart / trimEnd (Unicode whitespace and line terminators)
    String JsTrim(const String& text);
    String JsTrimStart(const String& text);
    String JsTrimEnd(const String& text);

    // JavaScript's toLowerCase / toUpperCase for Latin, Greek, Cyrillic, Armenian and full-width Latin; other
    // scripts are kept as they are
    String JsToLower(const String& text);
    String JsToUpper(const String& text);

    // True for what \p{L} or \p{N} matches (block-level approximation for symbols, punctuation and marks)
    bool IsLetterOrNumber(char32_t c);

    // text.split(/\s+/).filter(Boolean)
    Vector<String> SplitWords(const String& text);

    // text.replace(/\s+/g, with)
    String ReplaceSpaceRuns(const String& text, const String& with);

    // text.split(separator) with JavaScript's rules (an empty text gives one empty part)
    Vector<String> JsSplit(const String& text, const String& separator);

    // Parts joined with the separator
    String Join(const Vector<String>& parts, const String& separator);

    // True when the text starts with the prefix
    bool StartsWith(const String& text, const String& prefix);

    // ' '.repeat(count)
    String Spaces(int count);

    // --- time --------------------------------------------------------------------------------

    // Milliseconds since the epoch now
    Int64 NowMs();

    // new Date(ms).toISOString(): 2026-11-10T10:00:00.000Z
    String IsoTime(double ms);

    // Date.parse of an ISO date / date-time (Z, +-HH:MM or none - read as UTC); NaN for anything else
    double ParseIsoTime(const String& text);

    // A random lower-case base-36 text of the length (Math.random().toString(36).slice(2, 2 + length))
    String RandomBase36(int length);

    // n.toString(36) of a non-negative integer
    String ToBase36(Int64 n);

    // --- o2 data -------------------------------------------------------------------------------

    // The value of an o2 DataValue (a node's config) with its member order; numbers as doubles
    Json FromDataValue(const DataValue& value);

    // Writes the value into an o2 DataValue keeping member order; whole numbers become integers so o2's writer
    // prints them as JSON.stringify does (120, not 120.0)
    void ToDataValue(const Json& value, DataValue& out);
}
