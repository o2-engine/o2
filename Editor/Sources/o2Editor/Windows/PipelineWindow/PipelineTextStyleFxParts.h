#pragma once

#include "o2/Utils/Math/Color.h"
#include "o2Editor/Pipeline/UiDoc/UiTextStyle.h"

// What the two halves of the Text style card's drawn rows share (PipelineTextStyleFx.cpp: the fill and the controls,
// PipelineTextStyleFxStack.cpp: the effects block): the sliders of each kind of effect and the colour helpers
namespace Editor::TextFx
{
    constexpr float wellW = 26.0f, gap = 5.0f, buttonW = 18.0f, switchW = 32.0f;

    // ------------------------------------------------------------------
    // A number of an effect on a slider: its range, step and how it reads
    // ------------------------------------------------------------------
    struct Slider
    {
        const char* field;
        double      min, max, step;
        const char* prefix;
        const char* suffix;
        bool        percent = false;
    };

    // The colours of an effect's head row, its sliders there and on its second row
    struct EffectSpec
    {
        Vector<const char*> colors;
        Vector<Slider>      head;
        Vector<Slider>      sub;
    };

    // The controls of a kind of effect
    const EffectSpec& SpecOf(const String& type);

    // The gradient outline's angle slider
    const Slider& AngleSlider();

    // The effect's number behind a slider's field, null for none
    double* FieldOf(UiDoc::TextEffect& e, const String& field);

    // The effect's colour behind a well: highlight, shade, else its colour
    String* ColorOf(UiDoc::TextEffect& e, const String& field);

    // The kind's name and what it is
    String FxName(const String& type);
    String FxHint(const String& type);

    // A slider's value as it reads
    String Format(const Slider& s, double v);

    // A value on a slider's steps, within its range
    double Snap(double v, double min, double max, double step);

    // Two #RRGGBB colours mixed, k from a to b
    String Mix(const String& a, const String& b, double k);

    // A #RRGGBB colour, opaque (white for anything else)
    Color4 ColorOfHex(const String& hex);

    // The place in a hot spot's id: "fx3:width" -> 3, -1 for none
    int FxIndexOf(const String& id);
}
