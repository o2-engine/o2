#include "o2Editor/stdafx.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocFonts.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRenderer.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

// Labels of the layout renderer (render.ts's textSvg): lines fitted by LayoutText, placed by text-anchor and the cap
// height, drawn with FreeType in the style's face - fill or vertical gradient, an outline under the fill, a drop shadow
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

        UiRgba fill = UiColorOf(t.color ? *t.color : st.color);
        bool gradient = !t.plain && st.gradient && !t.color;
        UiRgba bottom = gradient ? UiColorOf(*st.gradient) : fill;
        double strokeW = !t.plain && st.stroke && st.stroke->width > 0 ? F((st.stroke->width*2*size)/st.size) : 0;
        UiRgba strokeColor = st.stroke ? UiColorOf(st.stroke->color) : UiRgba();
        bool shadow = !t.plain && st.shadow;
        double k = size/st.size;
        double sdx = 0, sdy = 0, sstd = 0, sop = 0;
        UiRgba scolor;
        if (shadow)
        {
            sdx = F(st.shadow->x*k);
            sdy = F(st.shadow->y*k);
            sstd = F(std::max(0.01, (st.shadow->blur*k)/2));
            sop = F(st.shadow->opacity);
            scolor = UiColorOf(st.shadow->color);
        }
        double ls = st.letterSpacing ? F((st.letterSpacing*size)/st.size) : 0;

        // The lines in pixels: shaped, anchored and on their baselines
        double sizePx = F(size)*s, lsPx = ls*s, strokePx = strokeW*s;
        std::vector<UiShapedLine> lines;
        std::vector<double> starts, bases;
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
            starts.push_back(start);
            bases.push_back(base);
        }
        if (lines.empty() || bx1 < bx0)
            return;

        // A block around the glyph cells: overhanging ink, the outline
        double margin = sizePx*0.6 + strokePx;
        int x0 = std::max(0, (int)std::floor(bx0 - margin)), y0 = std::max(0, (int)std::floor(by0 - margin));
        int x1 = std::min(cw, (int)std::ceil(bx1 + margin)), y1 = std::min(ch, (int)std::ceil(by1 + margin));
        double sigmaPx = sstd*s;
        int blurPad = shadow ? (int)std::ceil(sigmaPx*3 + std::max(std::fabs(sdx), std::fabs(sdy))*s) + 1 : 0;
        if (x1 <= x0 || y1 <= y0)
            return;

        TextBlock b;
        b.Reset(x0, y0, x1 - x0, y1 - y0);
        for (size_t i = 0; i < lines.size(); i++)
        {
            DrawUiText(lines[i], starts[i] - x0, bases[i] - y0, sizePx, 0, b.fill);
            if (strokePx > 0)
                DrawUiText(lines[i], starts[i] - x0, bases[i] - y0, sizePx, strokePx, b.stroke);
        }

        // paint-order="stroke": the outline under the fill
        if (strokePx > 0)
            b.canvas.Paint(b.stroke, strokeColor);
        if (gradient)
            b.canvas.PaintGradient(b.fill, fill, bottom, by0 - y0, by1 - y0);
        else
            b.canvas.Paint(b.fill, fill);

        if (shadow && sop > 0)
        {
            // feDropShadow: the text's alpha blurred, offset and flooded with the colour, under the text; inside the
            // filter region (the text's box grown by 30 % across and 50 % up and down)
            int sw2 = b.w + blurPad*2, sh2 = b.h + blurPad*2;
            std::vector<float> sh((size_t)sw2*sh2*4, 0.0f);
            for (int y = 0; y < b.h; y++)
            {
                for (int xx = 0; xx < b.w; xx++)
                {
                    float a = b.canvas.px[((size_t)y*b.w + xx)*4 + 3]*(float)sop;
                    float* d = sh.data() + ((size_t)(y + blurPad)*sw2 + xx + blurPad)*4;
                    d[0] = scolor.r*a;
                    d[1] = scolor.g*a;
                    d[2] = scolor.b*a;
                    d[3] = a;
                }
            }
            GaussianBlur(sh, sw2, sh2, sigmaPx);

            int offX = (int)std::lround(sdx*s), offY = (int)std::lround(sdy*s);
            int at0 = x0 - blurPad + offX, at1 = y0 - blurPad + offY;
            double bw = bx1 - bx0, bh = by1 - by0;
            double fx0 = bx0 - bw*0.3, fx1 = bx0 + bw*1.3, fy0 = by0 - bh*0.5, fy1 = by0 + bh*1.5;
            for (int y = 0; y < sh2; y++)
            {
                for (int xx = 0; xx < sw2; xx++)
                {
                    double cx = at0 + xx + 0.5, cy = at1 + y + 0.5;
                    if (cx < fx0 || cx > fx1 || cy < fy0 || cy > fy1)
                    {
                        float* d = sh.data() + ((size_t)y*sw2 + xx)*4;
                        d[0] = d[1] = d[2] = d[3] = 0;
                    }
                }
            }
            Top().CompositeBlock(sh, sw2, sh2, at0, at1);
        }

        Top().CompositeBlock(b.canvas.px, b.w, b.h, b.x0, b.y0);
    }
}
