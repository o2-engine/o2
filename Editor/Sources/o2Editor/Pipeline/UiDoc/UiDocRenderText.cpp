#include "o2Editor/stdafx.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocFonts.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRenderer.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

// Labels of the layout renderer (render.ts's textSvg): lines fitted by LayoutText, placed by text-anchor and the cap
// height, drawn with FreeType in the style's face: shadows and glows, the depth, the outlines from the outermost in, the
// fill (flat or a gradient per line) and the inner shadows and bevel over it
namespace Editor::UiDoc::Render
{
    namespace
    {
        // A text drawn into a block of its own (its box on the canvas plus a margin), composited when done
        struct TextBlock
        {
            int      x0 = 0, y0 = 0, w = 0, h = 0;
            UiMask   fill, stroke;
            UiCanvas canvas;

            void Reset(int bx0, int by0, int bw, int bh)
            {
                x0 = bx0; y0 = by0; w = bw; h = bh;
                fill.Reset(w, h);
                stroke.Reset(w, h);
                canvas.Reset(w, h);
            }
        };
    }

    void Renderer::PlainText(const String& text, const String& fontFile, double size, double x, double baseline, const UiRgba& color,
                             float opacity /*= 1*/)
    {
        if (text.IsEmpty())
            return;
        double sizePx = size*s;
        UiShapedLine line = ShapeUiText(fontFile, text, sizePx, 0);
        double px = X(x), py = Y(baseline);
        int bx0 = (int)std::floor(px - sizePx), by0 = (int)std::floor(py - line.ascent - sizePx*0.5);
        int bx1 = (int)std::ceil(px + line.width + sizePx), by1 = (int)std::ceil(py + line.descent + sizePx*0.5);
        bx0 = std::max(0, bx0); by0 = std::max(0, by0); bx1 = std::min(cw, bx1); by1 = std::min(ch, by1);
        if (bx1 <= bx0 || by1 <= by0)
            return;

        TextBlock b;
        b.Reset(bx0, by0, bx1 - bx0, by1 - by0);
        DrawUiText(line, px - bx0, py - by0, sizePx, 0, b.fill);
        b.canvas.Paint(b.fill, color);
        Top().CompositeBlock(b.canvas.px, b.w, b.h, b.x0, b.y0, opacity);
    }

    namespace
    {
        // A gradient's colour at t (0..1 along it): stops in order, padded at the ends
        UiRgba GradientAt(const TextGradient& g, double t)
        {
            const auto& stops = g.stops;
            if (t <= stops[0].at)
                return UiColorOf(stops[0].color);
            for (int i = 1; i < stops.Count(); i++)
            {
                if (t <= stops[i].at)
                {
                    UiRgba a = UiColorOf(stops[i - 1].color), b = UiColorOf(stops[i].color);
                    double span = stops[i].at - stops[i - 1].at;
                    double k = span > 0 ? (t - stops[i - 1].at)/span : 1;
                    return UiRgba{ (float)(a.r + (b.r - a.r)*k), (float)(a.g + (b.g - a.g)*k), (float)(a.b + (b.b - a.b)*k),
                                   (float)(a.a + (b.a - a.a)*k) };
                }
            }
            return UiColorOf(stops.Last().color);
        }

        // Paints the mask's coverage with a linear gradient from (ax, ay) to (bx, by), block pixels
        void PaintLinear(UiCanvas& canvas, const UiMask& mask, const TextGradient& g, double ax, double ay, double bx, double by)
        {
            double dx = bx - ax, dy = by - ay, len2 = dx*dx + dy*dy;
            for (int y = mask.y0; y < mask.y1; y++)
            {
                const float* m = mask.a.data() + (size_t)y*mask.w;
                float* d = canvas.px.data() + (size_t)y*canvas.w*4;
                for (int x = mask.lo[y]; x < mask.hi[y]; x++)
                {
                    if (m[x] <= 0)
                        continue;
                    double t = len2 > 0 ? ((x + 0.5 - ax)*dx + (y + 0.5 - ay)*dy)/len2 : 0;
                    UiRgba c = GradientAt(g, std::clamp(t, 0.0, 1.0));
                    float ca = m[x]*c.a;
                    float* p = d + x*4;
                    p[0] = c.r*ca + p[0]*(1 - ca);
                    p[1] = c.g*ca + p[1]*(1 - ca);
                    p[2] = c.b*ca + p[2]*(1 - ca);
                    p[3] = ca + p[3]*(1 - ca);
                }
            }
            canvas.Touch(mask.x0, mask.y0, mask.x1, mask.y1);
        }

        // Bilinear read of a plane, outside value out
        float Sample(const std::vector<float>& plane, int w, int h, double x, double y, float out)
        {
            int ix = (int)std::floor(x), iy = (int)std::floor(y);
            double fx = x - ix, fy = y - iy;
            auto at = [&](int px, int py) { return px < 0 || py < 0 || px >= w || py >= h ? out : plane[(size_t)py*w + px]; };
            double top = at(ix, iy)*(1 - fx) + at(ix + 1, iy)*fx;
            double bottom = at(ix, iy + 1)*(1 - fx) + at(ix + 1, iy + 1)*fx;
            return (float)(top*(1 - fy) + bottom*fy);
        }
    }

    void Renderer::Text(const TextBox& t)
    {
        if (t.text.IsEmpty())
            return;

        const TextStyle& st = t.style;
        UiTextFit fit = LayoutText(t.text, t.rect.w, t.rect.h, st, t.size, t.padX);
        String file = FaceOf(st.font, st.weight).file;
        double size = fit.size, lineH = fit.lineH;
        String anchor = t.align == "left" ? "start" : t.align == "right" ? "end" : "middle";
        double pad = t.padX;
        double x = t.align == "left" ? t.rect.x + pad : t.align == "right" ? t.rect.x + t.rect.w - pad : t.rect.x + t.rect.w/2;
        double total = fit.lines.Count()*lineH;
        double top = t.rect.y + (t.rect.h - total)/2;
        double k = size/st.size;
        double ls = st.letterSpacing ? F(st.letterSpacing*k) : 0;

        // The effects drawn: none for a plain label
        Vector<TextEffect> fx = t.plain ? Vector<TextEffect>() : LiveEffects(st);
        Vector<TextEffect> strokes, extrudes, outer, inner;
        double outline = 0;
        for (auto& e : fx)
        {
            if (e.type == "stroke")
            {
                strokes.Add(e);
                outline += e.width;
            }
            else if (e.type == "extrude")
                extrudes.Add(e);
            else if (e.type == "shadow" || e.type == "glow")
                outer.Add(e);
            else
                inner.Add(e);
        }

        // The lines in pixels: shaped, anchored and on their baselines
        double sizePx = F(size)*s, lsPx = ls*s;
        std::vector<UiShapedLine> lines;
        std::vector<String> shown;
        std::vector<double> starts, bases, centers;
        double bx0 = 1e300, bx1 = -1e300, by0 = 1e300, by1 = -1e300;
        for (int i = 0; i < fit.lines.Count(); i++)
        {
            String text = st.uppercase ? JsToUpper(fit.lines[i]) : fit.lines[i];
            UiShapedLine line = ShapeUiText(file, text, sizePx, lsPx);
            double px = X(F(x));
            double start = anchor == "middle" ? px - line.width/2 : anchor == "end" ? px - line.width : px;
            double base = Y(F(top + i*lineH + lineH/2 + (size*TEXT_METRICS.cap)/2));
            bx0 = std::min(bx0, start);
            bx1 = std::max(bx1, start + line.width);
            by0 = std::min(by0, base - line.ascent);
            by1 = std::max(by1, base + line.descent);
            lines.push_back(line);
            shown.push_back(text);
            starts.push_back(start);
            bases.push_back(base);
        }
        if (lines.empty() || bx1 < bx0)
            return;

        // A block around the glyph cells: overhanging ink and everything the effects reach out to
        TextReach reach = EffectsReach(st);
        double kp = k*s;
        double margin = sizePx*0.6 + 2 + std::max(std::max(reach.left, reach.right), std::max(reach.top, reach.bottom))*kp;
        int x0 = std::max(0, (int)std::floor(bx0 - margin)), y0 = std::max(0, (int)std::floor(by0 - margin));
        int x1 = std::min(cw, (int)std::ceil(bx1 + margin)), y1 = std::min(ch, (int)std::ceil(by1 + margin));
        if (x1 <= x0 || y1 <= y0)
            return;
        int bw = x1 - x0, bh = y1 - y0;

        UiCanvas canvas;
        canvas.Reset(bw, bh);
        UiMask mask, stroke;
        mask.Reset(bw, bh);
        stroke.Reset(bw, bh);

        // The outlined letters grown by grow px (the style's scale), at an offset in canvas pixels, as one coverage
        auto shape = [&](double grow, double dx, double dy) {
            mask.Clear();
            double width = (outline + grow)*2*kp;
            for (size_t i = 0; i < lines.size(); i++)
            {
                DrawUiText(lines[i], starts[i] - x0 + dx, bases[i] - y0 + dy, sizePx, 0, mask);
                if (width > 0)
                    DrawUiText(lines[i], starts[i] - x0 + dx, bases[i] - y0 + dy, sizePx, width, mask);
            }
        };

        // Shadows and glows of the whole outlined shape, at the letters and at the far end of each depth
        if (!outer.IsEmpty())
        {
            UiCanvas layer;
            for (int n = outer.Count() - 1; n >= 0; n--)
            {
                const TextEffect& e = outer[n];
                bool glow = e.type == "glow";
                double px = glow ? 0 : e.x, py = glow ? 0 : e.y, blur = glow ? e.size : e.blur, spread = glow ? e.size*0.25 : e.spread;
                layer.Reset(bw, bh);
                std::vector<std::pair<double, double>> stands = { { 0, 0 } };
                for (auto& x2 : extrudes)
                    stands.push_back({ x2.x*kp, x2.y*kp });
                for (auto& stand : stands)
                {
                    shape(spread, stand.first + px*kp, stand.second + py*kp);
                    layer.Paint(mask, UiColorOf(e.color));
                }
                double sigma = blur*k > 0.05 ? std::max(0.01, blur*k/2)*s : 0;
                if (sigma > 0)
                {
                    layer.Touch(0, 0, bw, bh);
                    GaussianBlur(layer.px, bw, bh, sigma);
                }
                canvas.Composite(layer, (float)e.opacity);
            }
        }

        // The depth: copies half a pixel apart read as one solid side
        for (int n = extrudes.Count() - 1; n >= 0; n--)
        {
            const TextEffect& e = extrudes[n];
            int copies = std::max(1, std::min(32, (int)std::ceil(std::max(std::fabs(e.x), std::fabs(e.y))*k*2)));
            for (int i = copies; i >= 1; i--)
            {
                shape(0, e.x*kp*i/copies, e.y*kp*i/copies);
                canvas.Paint(mask, UiColorOf(e.color));
            }
        }

        // The outlines from the outermost in
        double reachSum = outline;
        for (int n = strokes.Count() - 1; n >= 0; n--)
        {
            const TextEffect& e = strokes[n];
            mask.Clear();
            for (size_t i = 0; i < lines.size(); i++)
            {
                DrawUiText(lines[i], starts[i] - x0, bases[i] - y0, sizePx, 0, mask);
                DrawUiText(lines[i], starts[i] - x0, bases[i] - y0, sizePx, reachSum*2*kp, mask);
            }
            canvas.Paint(mask, UiColorOf(e.color), (float)e.opacity);
            reachSum -= e.width;
        }

        // The fill: flat, or a gradient running over each line's capitals on its own
        UiCanvas fill;
        fill.Reset(bw, bh);
        const TextGradient* grad = !t.color && !t.plain && st.fill ? &*st.fill : nullptr;
        UiRgba flat = UiColorOf(t.color ? *t.color : st.color);
        if (!grad)
        {
            mask.Clear();
            for (size_t i = 0; i < lines.size(); i++)
                DrawUiText(lines[i], starts[i] - x0, bases[i] - y0, sizePx, 0, mask);
            fill.Paint(mask, flat);
        }
        else
        {
            double rad = grad->angle*3.14159265358979323846/180;
            double gx = std::cos(rad), gy = std::sin(rad);
            TextStyle at = st;
            at.size = size;
            for (size_t i = 0; i < lines.size(); i++)
            {
                mask.Clear();
                DrawUiText(lines[i], starts[i] - x0, bases[i] - y0, sizePx, 0, mask);
                double w = std::max(1.0, EstimateTextWidth(shown[i], at))*s;
                double ax = X(F(x)) - x0;
                double cx = t.align == "left" ? ax + w/2 : t.align == "right" ? ax - w/2 : ax;
                double h = sizePx*TEXT_METRICS.cap*1.06;
                double cy = bases[i] - y0 - sizePx*TEXT_METRICS.cap/2;
                double half = std::fabs(gx)*w/2 + std::fabs(gy)*h/2;
                PaintLinear(fill, mask, *grad, cx - gx*half, cy - gy*half, cx + gx*half, cy + gy*half);
            }
        }

        // Inner shadows and the bevel: shades of the letters' own edge, laid over the fill inside it
        if (!inner.IsEmpty())
        {
            struct Shade
            {
                UiRgba color;
                double x, y, blur, opacity;
            };
            std::vector<Shade> shades;
            for (int n = inner.Count() - 1; n >= 0; n--)
            {
                const TextEffect& e = inner[n];
                if (e.type == "innerShadow")
                    shades.push_back({ UiColorOf(e.color), e.x*kp, e.y*kp, e.blur*k, e.opacity });
                else
                {
                    shades.push_back({ UiColorOf(e.shade), 0, -e.depth*kp, e.soft*k, e.opacity });
                    shades.push_back({ UiColorOf(e.highlight), 0, e.depth*kp, e.soft*k, e.opacity });
                }
            }

            std::vector<float> alpha((size_t)bw*bh);
            for (size_t i = 0; i < alpha.size(); i++)
                alpha[i] = fill.px[i*4 + 3];
            std::vector<float> inv(alpha.size()), shifted(alpha.size());
            for (auto& sh : shades)
            {
                // The inverse of the letters' alpha, blurred, offset, flooded and kept inside the letters
                std::vector<float> block((size_t)bw*bh*4, 0.0f);
                for (size_t i = 0; i < alpha.size(); i++)
                    block[i*4 + 3] = 1 - alpha[i];
                if (sh.blur > 0.05)
                    GaussianBlur(block, bw, bh, sh.blur/2*s);
                for (size_t i = 0; i < alpha.size(); i++)
                    inv[i] = block[i*4 + 3];
                for (int y = 0; y < bh; y++)
                {
                    for (int xx = 0; xx < bw; xx++)
                    {
                        size_t i = (size_t)y*bw + xx;
                        if (alpha[i] <= 0)
                            continue;
                        float a = Sample(inv, bw, bh, xx - sh.x, y - sh.y, 1.0f)*(float)sh.opacity*sh.color.a*alpha[i];
                        if (a <= 0)
                            continue;
                        float* p = fill.px.data() + i*4;
                        p[0] = sh.color.r*a + p[0]*(1 - a);
                        p[1] = sh.color.g*a + p[1]*(1 - a);
                        p[2] = sh.color.b*a + p[2]*(1 - a);
                        p[3] = a + p[3]*(1 - a);
                    }
                }
            }
        }

        canvas.Composite(fill);
        canvas.Touch(0, 0, bw, bh);
        Top().CompositeBlock(canvas.px, bw, bh, x0, y0);
    }
}
