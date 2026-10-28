#include "o2Editor/stdafx.h"
#include "EditorThemeDark.h"

#include <cfloat>
#include <initializer_list>

namespace Editor
{
    namespace EditorThemeDark
    {
        namespace
        {
            // State an image of the art draws, by the end of its file name
            enum class ArtState { Regular, Hover, Pressed, Focus, Dim };

            String FileOf(const String& path)
            {
                int slash = path.FindLast("/");
                String name = slash >= 0 ? path.SubStr(slash + 1) : path;
                name = name.EndsWith(".svg") ? name.SubStr(0, name.Length() - 4) : name;
                return name.Trimed(" ");
            }

            bool Has(const String& file, std::initializer_list<const char*> parts)
            {
                for (auto part : parts)
                {
                    if (file.Contains(part))
                        return true;
                }
                return false;
            }

            bool StartsWith(const String& file, std::initializer_list<const char*> prefixes)
            {
                for (auto prefix : prefixes)
                {
                    if (file.StartsWith(prefix))
                        return true;
                }
                return false;
            }

            ArtState StateOf(const String& file)
            {
                auto ends = [&](const char* suffix) { return file.EndsWith(String("_") + suffix) || file.EndsWith(String(" ") + suffix); };

                if (ends("pressed"))
                    return ArtState::Pressed;
                if (ends("select") || ends("selected") || ends("hover"))
                    return ArtState::Hover;
                if (ends("focus") || ends("focused"))
                    return ArtState::Focus;
                if (ends("inactive") || file.Contains("_inactive_"))
                    return ArtState::Dim;
                return ArtState::Regular;
            }

            float Lum(const Color4& c) { return (0.2126f*c.r + 0.7152f*c.g + 0.0722f*c.b)/255.0f; }

            // Hue in degrees and saturation of HSL
            void HueSat(const Color4& c, float& hue, float& sat)
            {
                float r = c.r/255.0f, g = c.g/255.0f, b = c.b/255.0f;
                float mx = Math::Max(r, Math::Max(g, b)), mn = Math::Min(r, Math::Min(g, b));
                float l = (mx + mn)*0.5f, d = mx - mn;
                sat = d < 0.0001f ? 0.0f : d/(1.0f - Math::Abs(2.0f*l - 1.0f));
                if (d < 0.0001f)
                    hue = 0.0f;
                else if (mx == r)
                    hue = 60.0f*fmodf((g - b)/d + 6.0f, 6.0f);
                else if (mx == g)
                    hue = 60.0f*((b - r)/d + 2.0f);
                else
                    hue = 60.0f*((r - g)/d + 4.0f);
            }

            // Spread of the channels: near-white and near-black tints have a high saturation but hardly any colour
            float Chroma(const Color4& c)
            {
                return (Math::Max(c.r, Math::Max(c.g, c.b)) - Math::Min(c.r, Math::Min(c.g, c.b)))/255.0f;
            }

            bool IsTeal(const Color4& c)
            {
                float hue, sat;
                HueSat(c, hue, sat);
                return hue > 158.0f && hue < 192.0f && Chroma(c) > 0.15f;
            }

            // A colour that means something by itself: errors, warnings, axes, the play green
            bool IsSemantic(const Color4& c)
            {
                float hue, sat;
                HueSat(c, hue, sat);
                return Chroma(c) > 0.3f && !IsTeal(c);
            }

            bool IsBlackish(const Color4& c) { return c.r < 26 && c.g < 26 && c.b < 26 && Math::Abs(c.r - c.b) < 12; }

            // Shadows are black at any opacity, solid or fading: they stay as they are
            bool IsShadow(const VectorPaint& paint)
            {
                if (paint.type == VectorPaintType::None)
                    return false;

                if (paint.type == VectorPaintType::Solid)
                    return IsBlackish(paint.color) && (paint.color.a < 200 || paint.color == Color4(0, 0, 0, 255));

                return !paint.stops.IsEmpty() && paint.stops.All([](const VectorGradientStop& stop) { return IsBlackish(stop.color); });
            }

            bool IsShadow(const VectorShape& shape)
            {
                bool fill = shape.fill.type == VectorPaintType::None || IsShadow(shape.fill);
                bool stroke = shape.stroke.type == VectorPaintType::None || IsShadow(shape.stroke);
                return fill && stroke && (shape.fill.type != VectorPaintType::None || shape.stroke.type != VectorPaintType::None);
            }

            // The colour a paint shows: its own, or the middle stop of a gradient
            Color4 ColorOf(const VectorPaint& paint)
            {
                if (paint.type == VectorPaintType::Solid || paint.stops.IsEmpty())
                    return paint.color;

                return paint.stops[paint.stops.Count()/2].color;
            }

            Color4 WithAlphaOf(const Color4& color, const Color4& alphaSource)
            {
                return Color4(color.r, color.g, color.b, alphaSource.a);
            }

            // Makes a paint solid in a colour, keeping the opacity it had
            void Paint(VectorPaint& paint, const Color4& color)
            {
                if (paint.type == VectorPaintType::None)
                    return;

                Color4 alphaSource = ColorOf(paint);
                paint = VectorPaint::Solid(WithAlphaOf(color, alphaSource));
            }

            void PaintSolid(VectorShape& shape, const Color4& color, float opacity)
            {
                shape.fill = VectorPaint::Solid(color);
                shape.fillOpacity = opacity;
            }

            Vec2F TopBottomOf(const VectorShape& shape)
            {
                float top = FLT_MAX, bottom = -FLT_MAX;
                auto add = [&](const Vec2F& p) { top = Math::Min(top, p.y); bottom = Math::Max(bottom, p.y); };
                for (auto& subPath : shape.subPaths)
                {
                    add(subPath.start);
                    for (auto& segment : subPath.segments)
                        add(segment.end);
                }
                return top <= bottom ? Vec2F(top, bottom) : Vec2F(0.0f, 1.0f);
            }

            // A gradient from a lighter top: the AssetsLine fill of raised things and popups
            VectorPaint Fall(const VectorShape& shape, const Color4& top, const Color4& bottom, float alpha)
            {
                Vec2F span = TopBottomOf(shape);
                auto withAlpha = [&](const Color4& c) { return Color4(c.r, c.g, c.b, (int)(alpha*255.0f)); };
                auto paint = VectorPaint::Linear(Vec2F(0.0f, span.x), Vec2F(0.0f, span.y),
                                                 { VectorGradientStop(0.0f, withAlpha(top)), VectorGradientStop(1.0f, withAlpha(bottom)) });
                paint.transform = Basis::Identity();
                return paint;
            }

            float AlphaOf(const VectorPaint& paint) { return ColorOf(paint).a/255.0f; }

            bool GetBounds(const VectorShape& shape, Vec2F& min, Vec2F& max)
            {
                return shape.GetBounds(min, max);
            }

            float AreaOf(const VectorShape& shape)
            {
                Vec2F min, max;
                return GetBounds(shape, min, max) ? (max.x - min.x)*(max.y - min.y) : 0.0f;
            }

            // Index of the largest shape that is not a shadow: the edge of a box, under its fill
            int OuterOf(const VectorImage& image)
            {
                int res = -1;
                float area = -1.0f;
                for (int i = 0; i < image.shapes.Count(); i++)
                {
                    if (IsShadow(image.shapes[i]))
                        continue;

                    float shapeArea = AreaOf(image.shapes[i]);
                    if (shapeArea > area)
                    {
                        area = shapeArea;
                        res = i;
                    }
                }
                return res;
            }

            // Adds a line along the top of a shape: the accent over a window header and the shown tab
            void AddTopLine(VectorImage& image, const VectorShape& under, const Color4& color, float height)
            {
                Vec2F min, max;
                if (!GetBounds(under, min, max))
                    return;

                VectorShape line;
                line.AddRect(min, Vec2F(max.x - min.x, height));
                line.fill = VectorPaint::Solid(color);
                line.stroke = VectorPaint();
                image.shapes.Add(line);
            }

            // ---------------------------------------------------------------------------------------------------------
            // Surfaces: the things other things stand on, by family. Hover lightens and strengthens the edge, a press
            // sinks, focus is the accent edge - the light theme's meaning in the dark colours
            // ---------------------------------------------------------------------------------------------------------

            void RestyleRaised(VectorImage& image, ArtState state)
            {
                bool hover = state == ArtState::Hover;
                int outer = OuterOf(image);
                bool single = image.shapes.Count([](const VectorShape& x) { return !IsShadow(x); }) == 1;
                for (int i = 0; i < image.shapes.Count(); i++)
                {
                    auto& shape = image.shapes[i];
                    if (IsShadow(shape))
                        continue;

                    float alpha = AlphaOf(shape.fill);
                    bool edge = i == outer && !single;
                    if (state == ArtState::Focus || IsTeal(ColorOf(shape.fill)))
                        Paint(shape.fill, accent);
                    else if (state == ArtState::Pressed)
                        Paint(shape.fill, edge ? border : pressed);
                    else if (!edge)
                        shape.fill = Fall(shape, hover ? raisedHoverTop : raisedTop, hover ? raisedHoverBottom : raisedBottom, alpha);
                    else
                        shape.fill = Fall(shape, hover ? edgeHoverTop : edgeTop, hover ? edgeHoverBottom : edgeBottom, alpha);
                }
            }

            // Fields, boxes, round fields, square buttons: a flat inset with a flat edge
            void RestyleField(VectorImage& image, const String& file, ArtState state)
            {
                bool flatButton = StartsWith(file, { "UI4_square_button", "UI4_round_field_gray", "UI4_small_panel" });
                int outer = OuterOf(image);
                bool single = image.shapes.Count([](const VectorShape& x) { return !IsShadow(x); }) == 1;
                for (int i = 0; i < image.shapes.Count(); i++)
                {
                    auto& shape = image.shapes[i];
                    if (IsShadow(shape))
                        continue;

                    bool edge = i == outer && !single;
                    auto restyle = [&](VectorPaint& paint, bool isStroke)
                    {
                        if (paint.type == VectorPaintType::None)
                            return;

                        Color4 c = ColorOf(paint);
                        UInt32 rgb = ((UInt32)c.r << 16) | ((UInt32)c.g << 8) | (UInt32)c.b;
                        if (file.StartsWith("UI4_green_edit_box") && IsTeal(c))
                            Paint(paint, green);
                        else if (rgb == 0xF85D48 || rgb == 0xF85E48)
                            Paint(paint, red);
                        else if (rgb == 0x486FF8)
                            Paint(paint, blue);
                        else if (IsTeal(c) || state == ArtState::Focus && Lum(c) < 0.8f)
                            Paint(paint, accent);
                        else if (!edge && !isStroke)
                        {
                            if (flatButton)
                                Paint(paint, state == ArtState::Hover ? raisedHoverBottom : state == ArtState::Pressed ? pressed : raisedBottom);
                            else
                                Paint(paint, state == ArtState::Hover ? fieldHover : state == ArtState::Pressed ? fieldPressed : field);
                        }
                        else
                            Paint(paint, state == ArtState::Hover ? borderStrong : border);
                    };

                    restyle(shape.fill, false);
                    restyle(shape.stroke, true);
                }
            }

            // Toolbar buttons of the window panels: flat until hovered, sunk when pressed
            void RestylePanelButton(VectorImage& image, ArtState state)
            {
                for (auto& shape : image.shapes)
                {
                    if (IsShadow(shape))
                        continue;

                    Color4 c = ColorOf(shape.fill);
                    if (state == ArtState::Focus || IsTeal(c))
                        Paint(shape.fill, accent);
                    else if (state == ArtState::Pressed)
                        Paint(shape.fill, Lum(c) > 0.86f ? pressed : line);
                    else if (Lum(c) > 0.97f)
                        shape.fill = Fall(shape, raisedHoverTop, raisedHoverBottom, AlphaOf(shape.fill));
                    else if (Lum(c) > 0.88f)
                        Paint(shape.fill, panel);
                    else
                        Paint(shape.fill, state == ArtState::Hover ? border : line);
                }
            }

            // Discs and pills that stand on the panel: knobs that light up when hovered and turn violet when pressed
            void RestyleKnob(VectorImage& image, ArtState state)
            {
                Color4 color = state == ArtState::Hover ? knobHover : state == ArtState::Pressed ? accentDeep : knob;
                for (auto& shape : image.shapes)
                {
                    if (IsShadow(shape))
                        continue;

                    if (shape.fill.type != VectorPaintType::None)
                        Paint(shape.fill, color);
                    if (shape.stroke.type != VectorPaintType::None)
                        Paint(shape.stroke, color);
                }
            }

            void RestyleScrollHandle(VectorImage& image, ArtState state)
            {
                Color4 color = state == ArtState::Hover ? handleHover : state == ArtState::Pressed ? accent : handle;
                for (auto& shape : image.shapes)
                {
                    if (!IsShadow(shape))
                        Paint(shape.fill, color);
                }
            }

            void RestyleAll(VectorImage& image, const Color4& color)
            {
                for (auto& shape : image.shapes)
                {
                    if (IsShadow(shape))
                        continue;

                    Paint(shape.fill, color);
                    Paint(shape.stroke, color);
                }
            }

            // Washes and frames of selections: the deep violet the selected things stand on
            void RestyleWash(VectorImage& image, const Color4& color, float opacity)
            {
                for (auto& shape : image.shapes)
                {
                    if (IsShadow(shape))
                        continue;

                    if (shape.fill.type != VectorPaintType::None)
                        PaintSolid(shape, color, opacity);
                    if (shape.stroke.type != VectorPaintType::None)
                        Paint(shape.stroke, accent);
                }
            }

            void RestyleMenuBar(VectorImage& image, const String& file, ArtState state)
            {
                for (auto& shape : image.shapes)
                {
                    if (file == "UI4_Up_Menu_select")
                        PaintSolid(shape, Color4::White(), 0.07f);
                    else if (file == "UI4_Up_Menu_pressed")
                        PaintSolid(shape, accentDeep, 0.9f);
                    else if (IsShadow(shape))
                        continue;
                    else
                    {
                        if (shape.fill.type != VectorPaintType::None)
                            shape.fill = Fall(shape, chromeTop, chromeBottom, 1.0f);
                        if (shape.stroke.type != VectorPaintType::None)
                            Paint(shape.stroke, chromeTop);
                    }
                }
            }

            // Docked and floating window frames and the tabs: the shown tab is the body's colour under an accent line,
            // where the light theme has its teal header
            void RestyleWindowFrame(VectorImage& image, const String& file)
            {
                bool docked = file.Contains("docked");
                bool strip = file.StartsWith("UI4_window_frame_docked_tab_back");
                bool inactive = file.StartsWith("UI4_window_frame_docked_tab_inactive");
                bool shownTab = file.StartsWith("UI4_window_frame_docked_tab_active") || file == "UI4_window_frame_docked" ||
                    file == "UI4_window_frame_regular";

                int header = -1;
                for (int i = 0; i < image.shapes.Count(); i++)
                {
                    auto& shape = image.shapes[i];
                    if (IsShadow(shape))
                        continue;

                    Color4 c = ColorOf(shape.fill.type != VectorPaintType::None ? shape.fill : shape.stroke);
                    Color4 surface = strip ? tabStrip : inactive ? tabInactive : docked ? panel : popupTop;
                    if (IsTeal(c))
                    {
                        if (shape.fill.type != VectorPaintType::None)
                        {
                            Paint(shape.fill, surface);
                            header = i;
                        }
                        if (shape.stroke.type != VectorPaintType::None)
                            Paint(shape.stroke, surface);
                    }
                    else if (Lum(c) > 0.9f)
                        Paint(shape.fill, docked ? panel : popupBottom);
                    else
                    {
                        Paint(shape.fill, line);
                        Paint(shape.stroke, line);
                    }
                }

                if (shownTab && header >= 0)
                    AddTopLine(image, image.shapes[header], accent, 2.0f);
            }

            // ---------------------------------------------------------------------------------------------------------
            // Round buttons: a disc with a mark. The light theme has a teal or white disc; the dark one a disc lighter
            // than what is around it, with a bright mark, a lighter disc when hovered and a violet one when pressed
            // ---------------------------------------------------------------------------------------------------------
            void RestyleRoundButton(VectorImage& image, ArtState state)
            {
                float extent = 0.0f;
                for (auto& shape : image.shapes)
                {
                    Vec2F min, max;
                    if (!IsShadow(shape) && GetBounds(shape, min, max))
                        extent = Math::Max(extent, Math::Max(max.x - min.x, max.y - min.y));
                }

                for (auto& shape : image.shapes)
                {
                    Vec2F min, max;
                    if (IsShadow(shape) || !GetBounds(shape, min, max))
                        continue;

                    Vec2F size = max - min;
                    bool disc = Math::Max(size.x, size.y) >= extent*0.75f && size.x > size.y*0.8f && size.y > size.x*0.8f;
                    Color4 c = ColorOf(shape.fill.type != VectorPaintType::None ? shape.fill : shape.stroke);

                    Color4 color;
                    if (disc && IsTeal(c))
                        color = state == ArtState::Hover ? accentDeepHover : state == ArtState::Pressed ? accentDeepPressed : accentDeep;
                    else if (disc && IsSemantic(c))
                        color = state == ArtState::Hover ? Color4(Math::Min(c.r + 30, 255), Math::Min(c.g + 30, 255), Math::Min(c.b + 30, 255)) : c;
                    else if (disc)
                        color = state == ArtState::Hover ? discHover : state == ArtState::Pressed ? accentDeep : EditorThemeDark::disc;
                    else
                        color = state == ArtState::Hover || state == ArtState::Pressed ? glyphHover : textStrong;

                    Paint(shape.fill, color);
                    Paint(shape.stroke, color);
                }
            }

            // ---------------------------------------------------------------------------------------------------------
            // Icons and marks: one bright glyph colour that lights up when hovered and turns to the accent when
            // pressed; the colours that mean something stay. A light cut-out over a darker glyph takes the panel colour
            // ---------------------------------------------------------------------------------------------------------
            void RestyleGlyph(VectorImage& image, ArtState state, bool tool)
            {
                // A two-tone glyph: the tone that covers more is the glyph, the other one is cut out of it
                float lightArea = 0.0f, darkArea = 0.0f;
                for (auto& shape : image.shapes)
                {
                    if (IsShadow(shape))
                        continue;

                    for (auto paint : { &shape.fill, &shape.stroke })
                    {
                        if (paint->type == VectorPaintType::None || IsSemantic(ColorOf(*paint)))
                            continue;

                        (Lum(ColorOf(*paint)) > 0.85f ? lightArea : darkArea) += AreaOf(shape);
                    }
                }
                bool twoTone = lightArea > 0.0f && darkArea > 0.0f;
                bool lightIsGlyph = lightArea >= darkArea;

                // The tool icons are white as in the light theme: the tools panel dims the ones not chosen
                Color4 color = state == ArtState::Hover ? glyphHover : state == ArtState::Pressed || state == ArtState::Focus ? accent :
                    state == ArtState::Dim ? glyphDim : tool ? textStrong : glyph;

                for (auto& shape : image.shapes)
                {
                    if (IsShadow(shape))
                        continue;

                    for (auto paint : { &shape.fill, &shape.stroke })
                    {
                        if (paint->type == VectorPaintType::None)
                            continue;

                        Color4 c = ColorOf(*paint);
                        if (IsSemantic(c))
                        {
                            if (Lum(c) < 0.3f)
                                Paint(*paint, Color4(Math::Min(c.r*2, 255), Math::Min(c.g*2, 255), Math::Min(c.b*2, 255)));
                            continue;
                        }

                        bool cutOut = twoTone && (Lum(c) > 0.85f) != lightIsGlyph;
                        Paint(*paint, cutOut ? panel : color);
                    }
                }
            }

            // Selected points of the curve and the spline editors: the light theme's green turns accent, paler where it was
            // paler, each stop keeping its opacity
            void RestyleSelectedHandle(VectorImage& image)
            {
                auto recolor = [](const Color4& c) {
                    if (!IsSemantic(c))
                        return c;

                    float pale = Math::Min(c.r, Math::Min(c.g, c.b))/255.0f;
                    return WithAlphaOf(Math::Lerp(accent, Color4::White(), pale*0.8f), c);
                };

                for (auto& shape : image.shapes)
                {
                    for (auto* paint : { &shape.fill, &shape.stroke })
                    {
                        paint->color = recolor(paint->color);
                        for (auto& stop : paint->stops)
                            stop.color = recolor(stop.color);
                    }
                }
            }

            // ---------------------------------------------------------------------------------------------------------
            // Kinds of the art by file
            // ---------------------------------------------------------------------------------------------------------

            bool IsRoundButton(const String& file)
            {
                return StartsWith(file, { "UI4_Close_button", "UI4_Options_button", "UI4_add_btn", "UI4_accept_prefab",
                                          "UI4_break_prefab", "UI4_revert_prefab", "UI4_remove_asset_instance",
                                          "UI4_save_asset_instance", "UI4_minus_small", "UI4_plus_small", "UI4_play_green",
                                          "UI4_pause_green", "UI4_play_anim", "UI4_stop_anim", "UI4_gray_options" });
            }

            // Pictures that carry their own background: file icons and alike read on any panel as they are
            bool IsPicture(const String& file)
            {
                return (file.StartsWith("UI4_big_") && !file.StartsWith("UI4_big_lock")) ||
                    StartsWith(file, { "UI4_actor_icon", "UI4_add_asset_icon", "UI4_anim_file_icon", "UI4_graph_icon",
                                       "UI4_video_icon", "UI4_image_asset_back", "UI4_Record_icon",
                                       "UI4_log_errors_icon", "UI4_log_warnings_icon", "UI4_log_info_icon",
                                       "UI4_dopesheet_back",
                                       "UI2_move_tool_center", "UI2_right_", "UI2_up_", "UI2_scale_both", "CurveHandle",
                                       "CurveSupportHandle", "hor_slice_line", "ver_slice_line", "circle_hole_handle",
                                       "hor_hole_handle", "ver_hole_handle", "UI4_time_line" }) &&
                    !StartsWith(file, { "UI4_time_line_left", "UI4_time_line_right" });
            }
        }

        void RestyleImage(VectorImage& image, const String& path)
        {
            // The Pipeline window colours its own art by its palette, in both themes
            if (path.Contains("/ui/pipeline/"))
                return;

            String file = FileOf(path);
            ArtState state = StateOf(file);

            if (file == "UI4_Background" || file == "UI_Background")
                RestyleAll(image, backdrop);
            else if (file.StartsWith("UI4_button"))
                RestyleRaised(image, state);
            else if (StartsWith(file, { "UI4_Editbox", "UI4_Box", "UI4_Check_bk", "UI4_Radio_bk", "UI4_round_field",
                                        "UI4_square_field", "UI4_square_button", "UI4_small_panel", "UI4_blue_edit_box",
                                        "UI4_green_edit_box", "UI4_red_edit_box", "UI4_enable_toggle_big" }))
                RestyleField(image, file, state);
            else if (file.StartsWith("UI4_panel_button"))
                RestylePanelButton(image, state);
            else if (StartsWith(file, { "UI4_play_panel_bk", "UI4_panel_subpanel" }))
            {
                // Pills on the chrome: the play and the tools buttons, the round drop downs. A ring around a sunk fill,
                // as the light ones; a hovered one lightens, a pressed one is violet
                Color4 fill = state == ArtState::Hover ? knob : state == ArtState::Pressed ? accentDeep : pressed;
                int outer = OuterOf(image);
                for (int i = 0; i < image.shapes.Count(); i++)
                {
                    if (!IsShadow(image.shapes[i]))
                        Paint(image.shapes[i].fill, i == outer ? borderStrong : fill);
                }
            }
            else if (file.StartsWith("UI4_menu_toggled"))
                RestyleKnob(image, state);
            else if (file.StartsWith("UI4_Context_menu_select"))
                RestyleWash(image, accentDeep, 0.8f);
            else if (file.StartsWith("UI4_Context_menu_white") || file.StartsWith("UI_ListBox_selection_hover"))
            {
                for (auto& shape : image.shapes)
                    PaintSolid(shape, Color4::White(), 0.06f);
            }
            else if (file.StartsWith("UI4_Context_menu"))
            {
                for (auto& shape : image.shapes)
                {
                    if (IsShadow(shape))
                        continue;

                    float alpha = AlphaOf(shape.fill);
                    shape.fill = Fall(shape, popupTop, popupBottom, alpha);
                    shape.stroke = Fall(shape, edgeTop, edgeBottom, alpha);
                    shape.strokeWidth = 1.0f;
                    shape.strokeOpacity = 1.0f;
                }
            }
            else if (file.StartsWith("UI4_asset_icon_selection"))
                RestyleWash(image, accentDeep, 0.75f);
            else if (StartsWith(file, { "UI4_selection_frame", "UI4_Window_place", "UI_Window_place" }))
                RestyleWash(image, accent, 0.18f);
            else if (file.StartsWith("UI4_keys_select border"))
                RestyleAll(image, accent);
            else if (file.StartsWith("UI4_keys_select"))
                RestyleWash(image, accentDeep, state == ArtState::Hover ? 0.65f : state == ArtState::Pressed ? 0.85f : 0.5f);
            else if (file.StartsWith("UI4_animation_bar"))
                RestyleAll(image, accentDeep);
            else if (StartsWith(file, { "UI4_Up_Menu", "UI4_ToolsPanel_bk" }))
                RestyleMenuBar(image, file, state);
            else if (file.StartsWith("UI4_window_frame"))
                RestyleWindowFrame(image, file);
            else if (StartsWith(file, { "UI4_Hor_scrollbar_bk", "UI4_Ver_scrollbar_bk" }))
                RestyleAll(image, track);
            else if (StartsWith(file, { "UI4_Hor_scrollhandle", "UI4_Ver_scrollhandle" }))
                RestyleScrollHandle(image, state);
            else if (StartsWith(file, { "UI4_Separator", "UI4_Ver_separator", "UI4_sub_track" }))
                RestyleAll(image, border);
            else if (StartsWith(file, { "UI4_shadow_separator", "UI4_common_anim_track" }))
                RestyleAll(image, line);
            else if (file.StartsWith("UI4_component_head"))
            {
                for (auto& shape : image.shapes)
                {
                    if (!IsShadow(shape))
                        shape.fill = Fall(shape, raisedTop, raisedBottom, AlphaOf(shape.fill));
                }
            }
            else if (file.StartsWith("UI4_animation_state"))
            {
                if (file.EndsWith("_default"))
                    RestyleAll(image, blue);
                else if (file.EndsWith("_frame"))
                    RestyleAll(image, accent);
                else if (!file.EndsWith("_shadow"))
                    RestyleAll(image, state == ArtState::Hover ? raisedHoverTop : state == ArtState::Pressed ? accentDeep : raisedTop);
            }
            else if (StartsWith(file, { "UI4_Ckeck", "UI4_Radio", "UI4_Check_unknown" }))
                RestyleAll(image, accent);
            else if (IsRoundButton(file))
                RestyleRoundButton(image, state);
            else if (StartsWith(file, { "CurveHandleSelected", "CurveSupportHandleSelected" }))
                RestyleSelectedHandle(image);
            else if (IsPicture(file))
                return;
            else
                RestyleGlyph(image, state, file.EndsWith("_tool") || file.EndsWith("Tool"));
        }
    }
}
