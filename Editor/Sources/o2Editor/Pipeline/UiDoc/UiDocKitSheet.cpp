#include "o2Editor/stdafx.h"

#include <cmath>

#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRenderer.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

// The UI kit as one picture for the layout agent (port of AssetsLine backend/src/uiKit/sheet.ts): every graphic on a
// checkerboard cell with its name, type, asset type and size under it - the agent sees what each name looks like instead
// of guessing from the name
namespace Editor::UiDoc
{
    UiImage RenderKitSheet(const UiKit& kit, int max /*= 40*/, std::optional<double> layoutDpi /*= std::nullopt*/)
    {
        const int COLS = 5, CW = 204, CH = 196, PIC_W = 184, PIC_H = 136, PAD = 10;
        int n = std::min(max, kit.elements.Count());
        if (n <= 0)
            return UiImage();

        int cols = std::min(COLS, n);
        int rows = (n + cols - 1)/cols;
        int W = cols*CW + PAD*2, H = rows*CH + PAD*2;

        UiDoc doc = EmptyDoc("Sheet", W, H);
        UiRenderOptions o;
        Render::Renderer r(doc, o);
        r.Begin();
        r.FillShape({ RectPath(0, 0, W, H) }, UiColorOf("#1B1D26"));

        // The checkerboard pattern from the sheet's origin: 16 px tiles, 8 px squares
        UiRgba dark = UiColorOf("#2B2F3A"), light = UiColorOf("#353A47");
        auto checker = [&](int x, int y) { return ((x % 16) < 8) == ((y % 16) < 8) ? light : dark; };
        UiRgba nameColor = UiColorOf("#F1F2F6"), metaColor = UiColorOf("#9AA0AE");
        for (int i = 0; i < n; i++)
        {
            const UiKitElement& e = kit.elements[i];
            int x = PAD + (i % cols)*CW, y = PAD + (i/cols)*CH;
            r.FillPattern({ RoundRectPath(x + 4, y + 4, CW - 8, PIC_H + 12, 8) }, checker);

            String kind = e.assetType == "sliced" ? String("9-slice") : e.assetType;
            UiSize size = layoutDpi ? DesignSize(*layoutDpi, e.width, e.height, e.dpi) : UiSize{ e.width, e.height };
            String meta = e.type + (e.variant && !e.variant->IsEmpty() ? "/" + *e.variant : String()) + " \xC2\xB7 " + kind + " \xC2\xB7 " +
                JsNum(size.w) + "\xC3\x97" + JsNum(size.h);
            r.PlainText(e.name, "Roboto-Bold.ttf", 15, x + 8, y + PIC_H + 36, nameColor);
            r.PlainText(meta, "Roboto-Regular.ttf", 12, x + 8, y + PIC_H + 55, metaColor);

            // The picture fitted inside its cell (enlarged too), lanczos as sharp resizes
            UiImage img;
            if (!UiImage::Decode(PipelineUtils::Base64Decode(e.png), img))
                continue;
            double scale = std::min((PIC_W - 8)/(double)img.w, PIC_H/(double)img.h);
            int pw = std::max(1, (int)JsRound(img.w*scale)), ph = std::max(1, (int)JsRound(img.h*scale));
            UiImage fitted = img.Resize(pw, ph);
            UiPicture p = UiPicture::Of(fitted);
            int left = (int)JsRound(x + (CW - pw)/2.0), topY = (int)JsRound(y + 10 + (PIC_H - ph)/2.0);
            UiPictureDraw d;
            d.tx = left;
            d.ty = topY;
            d.cx0 = left; d.cy0 = topY; d.cx1 = left + pw; d.cy1 = topY + ph;
            r.Top().Draw(p, d);
        }
        return r.Finish();
    }
}
