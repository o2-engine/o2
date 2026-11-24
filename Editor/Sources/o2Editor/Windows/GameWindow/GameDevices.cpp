#include "o2Editor/stdafx.h"
#include "GameDevices.h"

namespace Editor
{
    namespace
    {
        GameDevice Make(const char* id, const char* name, const char* group, const char* kind, const char* look,
                        float w, float h, int pw, int ph, float inch, float padX, float padY, float radius,
                        float safeTop = 0, float safeBottom = 0)
        {
            GameDevice d;
            d.id = id; d.name = name; d.group = group; d.kind = kind; d.look = look;
            d.size = Vec2F(w, h); d.pixels = Vec2I(pw, ph); d.inch = inch; d.pad = Vec2F(padX, padY);
            d.radius = radius; d.safeTop = safeTop; d.safeBottom = safeBottom;
            return d;
        }

        // The body around the screen, in points: its padding, the frame's border, the outer corners, how far the side
        // buttons stick out (o2portal ui/devices.ts bodyOf)
        struct Body { float pt, pr, pb, pl, border, outer, reach; };

        Body BodyOf(const GameDevice& d, bool landscape)
        {
            if (d.look == "window")
                return { 30, 0, 0, 0, 1, 10, 0 };
            if (d.look == "console")
                return { 40, 190, 40, 190, 0, 80, 0 };

            bool flip = landscape && !d.IsFixedLandscape();
            float x = flip ? d.pad.y : d.pad.x, y = flip ? d.pad.x : d.pad.y;
            float border = d.look == "tablet" ? 3.0f : 4.0f;
            float outer = d.look == "home" ? 60.0f : d.radius + Math::Min(x, y) + border;
            return { y, x, y, x, border, outer, 4 };
        }

        struct Btn { char side; float at, len; };

        Vector<Btn> ButtonsOf(const GameDevice& d, bool landscape)
        {
            Vector<Btn> list;
            if (d.look == "island")
                list = { { 'l', 0.18f, 0.045f }, { 'l', 0.27f, 0.08f }, { 'l', 0.37f, 0.08f }, { 'r', 0.29f, 0.12f }, { 'r', 0.6f, 0.07f } };
            else if (d.look == "punch")
                list = { { 'r', 0.26f, 0.08f }, { 'r', 0.38f, 0.15f } };
            else if (d.look == "home")
                list = { { 'l', 0.15f, 0.04f }, { 'l', 0.23f, 0.07f }, { 'l', 0.32f, 0.07f }, { 'r', 0.23f, 0.09f } };
            else if (d.look == "tablet")
                list = { { 't', 0.82f, 0.06f }, { 'r', 0.08f, 0.08f }, { 'r', 0.18f, 0.08f } };

            if (!landscape || d.IsFixedLandscape())
                return list;

            // on its side the body has turned counterclockwise: the left edge is at the bottom, the top at the left
            for (auto& b : list)
            {
                if (b.side == 'l') b.side = 'b';
                else if (b.side == 'r') b.side = 't';
                else { b.side = b.side == 't' ? 'l' : 'r'; b.at = 1.0f - b.at - b.len; }
            }
            return list;
        }

        String Num(float v)
        {
            return String::Format("%.2f", v);
        }

        // A rounded rectangle as a path, corners as cubic curves; radius per corner: top left, top right, bottom
        // right, bottom left
        String RoundRect(float x, float y, float w, float h, float tl, float tr, float br, float bl)
        {
            const float k = 0.5523f;
            auto clampR = [&](float r) { return Math::Max(0.0f, Math::Min(r, Math::Min(w, h)*0.5f)); };
            tl = clampR(tl); tr = clampR(tr); br = clampR(br); bl = clampR(bl);
            String p;
            p += "M" + Num(x + tl) + " " + Num(y);
            p += " L" + Num(x + w - tr) + " " + Num(y);
            p += " C" + Num(x + w - tr + tr*k) + " " + Num(y) + " " + Num(x + w) + " " + Num(y + tr - tr*k) + " " + Num(x + w) + " " + Num(y + tr);
            p += " L" + Num(x + w) + " " + Num(y + h - br);
            p += " C" + Num(x + w) + " " + Num(y + h - br + br*k) + " " + Num(x + w - br + br*k) + " " + Num(y + h) + " " + Num(x + w - br) + " " + Num(y + h);
            p += " L" + Num(x + bl) + " " + Num(y + h);
            p += " C" + Num(x + bl - bl*k) + " " + Num(y + h) + " " + Num(x) + " " + Num(y + h - bl + bl*k) + " " + Num(x) + " " + Num(y + h - bl);
            p += " L" + Num(x) + " " + Num(y + tl);
            p += " C" + Num(x) + " " + Num(y + tl - tl*k) + " " + Num(x + tl - tl*k) + " " + Num(y) + " " + Num(x + tl) + " " + Num(y);
            p += " Z";
            return p;
        }

        String RoundRect(float x, float y, float w, float h, float r)
        {
            return RoundRect(x, y, w, h, r, r, r, r);
        }

        String Circle(float cx, float cy, float r, const String& fill)
        {
            return "<circle cx=\"" + Num(cx) + "\" cy=\"" + Num(cy) + "\" r=\"" + Num(r) + "\" fill=\"" + fill + "\"/>\n";
        }

        String Path(const String& d, const String& fill, float opacity = 1.0f, bool evenOdd = false)
        {
            String s = "<path d=\"" + d + "\" fill=\"" + fill + "\"";
            if (opacity < 1.0f)
                s += " fill-opacity=\"" + Num(opacity) + "\"";
            if (evenOdd)
                s += " fill-rule=\"evenodd\"";
            return s + "/>\n";
        }

        String Linear(const String& id, float x1, float y1, float x2, float y2, const Vector<Pair<float, String>>& stops)
        {
            String s = "<linearGradient id=\"" + id + "\" gradientUnits=\"userSpaceOnUse\" x1=\"" + Num(x1) + "\" y1=\"" + Num(y1) +
                "\" x2=\"" + Num(x2) + "\" y2=\"" + Num(y2) + "\">";
            for (auto& stop : stops)
                s += "<stop offset=\"" + Num(stop.first) + "\" stop-color=\"" + stop.second + "\"/>";
            return s + "</linearGradient>\n";
        }

        String Radial(const String& id, float cx, float cy, float r, const Vector<Pair<float, String>>& stops)
        {
            String s = "<radialGradient id=\"" + id + "\" gradientUnits=\"userSpaceOnUse\" cx=\"" + Num(cx) + "\" cy=\"" + Num(cy) +
                "\" r=\"" + Num(r) + "\">";
            for (auto& stop : stops)
                s += "<stop offset=\"" + Num(stop.first) + "\" stop-color=\"" + stop.second + "\"/>";
            return s + "</radialGradient>\n";
        }
    }

    bool GameDevice::IsFixedLandscape() const
    {
        return kind == "desktop";
    }

    Vec2F GameDevice::GetScreenSize(bool landscape) const
    {
        return landscape && !IsFixedLandscape() ? Vec2F(size.y, size.x) : size;
    }

    Vec2I GameDevice::GetScreenPixels(bool landscape) const
    {
        return landscape && !IsFixedLandscape() ? Vec2I(pixels.y, pixels.x) : pixels;
    }

    String GameDevice::GetCaption() const
    {
        String caption = name + "   " + (String)pixels.x + " x " + (String)pixels.y;
        if (inch > 0)
            caption += ", " + String::Format("%g", inch) + "\"";
        return caption;
    }

    BorderF GameDevice::GetSafeArea(bool landscape) const
    {
        if (safeTop <= 0 && safeBottom <= 0)
            return BorderF();
        if (!landscape || IsFixedLandscape() || look == "tablet")
            return BorderF(0, safeBottom, 0, safeTop);
        if (look == "island")
            return BorderF(safeTop, 21, safeTop, 0);
        return BorderF(safeTop, 0, 0, 0);
    }

    String GameDevice::GetBodySvg(bool landscape, Vec2F& bodySize, Vec2F& screenOffset) const
    {
        Body b = BodyOf(*this, landscape);
        bool flip = landscape && !IsFixedLandscape();
        Vec2F screen = GetScreenSize(landscape);

        float padW = screen.x + b.pl + b.pr, padH = screen.y + b.pt + b.pb; // the padding box: bezels and screen
        float ox = b.reach + b.border, oy = b.reach + b.border;             // its top left
        bodySize = Vec2F(padW + 2*(b.border + b.reach), padH + 2*(b.border + b.reach));
        screenOffset = Vec2F(ox + b.pl, oy + b.pt);

        float sx = screenOffset.x, sy = screenOffset.y;
        float bw = padW + 2*b.border, bh = padH + 2*b.border;

        Vector<Pair<float, String>> metal;
        String bezel = "#08090a";
        Vector<Pair<float, String>> buttons = { { 0.0f, "#c9cdd0" }, { 0.45f, "#eceeef" }, { 1.0f, "#8d9297" } };
        if (look == "punch")
        {
            metal = { { 0.0f, "#5b6066" }, { 0.25f, "#2d3035" }, { 0.5f, "#6a6f75" }, { 0.75f, "#2a2d31" }, { 1.0f, "#50555a" } };
            buttons = { { 0.0f, "#2a2d31" }, { 0.45f, "#6a6f75" }, { 1.0f, "#2d3035" } };
        }
        else if (look == "home")
        {
            metal = { { 0.0f, "#3a3d41" }, { 0.3f, "#17191b" }, { 0.55f, "#4a4e52" }, { 0.8f, "#151719" }, { 1.0f, "#34373a" } };
            buttons = { { 0.0f, "#2a2d31" }, { 0.45f, "#6a6f75" }, { 1.0f, "#2d3035" } };
            bezel = "#0d0e10";
        }
        else if (look == "tablet")
            metal = { { 0.0f, "#d6d9dc" }, { 0.25f, "#9a9fa4" }, { 0.5f, "#e3e5e7" }, { 0.75f, "#8e9398" }, { 1.0f, "#cfd2d5" } };
        else
            metal = { { 0.0f, "#e2e5e7" }, { 0.16f, "#8d9297" }, { 0.33f, "#eceeef" }, { 0.5f, "#6b7075" }, { 0.67f, "#c9cdd0" },
                      { 0.84f, "#7a7f84" }, { 1.0f, "#dfe2e4" } };

        String defs, body;
        String screenHole = RoundRect(sx, sy, screen.x, screen.y, radius);

        if (look == "window")
        {
            // a desktop window: the frame line, the title bar with its three lights, the screen under it
            defs += Linear("bar", 0, 0, 0, b.pt, { { 0.0f, "#e9eeed" }, { 1.0f, "#dfe6e5" } });
            body += Path(RoundRect(b.reach, b.reach, bw, bh, b.outer) + " " + RoundRect(ox, oy, padW, padH, b.outer - b.border),
                         "#b3c0be", 1.0f, true);
            body += Path(RoundRect(ox, oy, padW, padH, b.outer - b.border) + " " +
                         RoundRect(sx, sy, screen.x, screen.y, 0, 0, 9, 9), "url(#bar)", 1.0f, true);
            body += Circle(ox + 18, oy + 15, 6, "#ff5f57");
            body += Circle(ox + 38, oy + 15, 6, "#febc2e");
            body += Circle(ox + 58, oy + 15, 6, "#28c840");
        }
        else if (look == "console")
        {
            // Steam Deck: the grips, the sticks, the pads, the buttons
            defs += Radial("deck", ox + padW*0.5f, oy, padW*0.7f, { { 0.0f, "#3a3e43" }, { 0.6f, "#25282c" }, { 1.0f, "#1d1f22" } });
            body += Path(RoundRect(ox, oy, padW, padH, b.outer) + " " + screenHole, "url(#deck)", 1.0f, true);
            for (int side = 0; side < 2; side++)
            {
                float stickX = side == 0 ? ox + 54 + 42 : ox + padW - 54 - 42;
                body += Circle(stickX, oy + 36 + 42, 42, "#15171a");
                body += Circle(stickX, oy + 36 + 40, 32, "#3c4045");
                float padX = side == 0 ? ox + 44 : ox + padW - 44 - 104;
                body += Path(RoundRect(padX, oy + padH - 40 - 104, 104, 104, 18), "#24272b");
            }
            body += Path(RoundRect(ox + 70 + 17, oy + 178, 18, 52, 2) + " " + RoundRect(ox + 70, oy + 178 + 17, 52, 18, 2), "#17191c");
            float ax = ox + padW - 78 - 8.5f, ay = oy + 182 + 8.5f;
            body += Circle(ax, ay, 8.5f, "#17191c");
            body += Circle(ax - 22, ay + 22, 8.5f, "#17191c");
            body += Circle(ax + 22, ay + 22, 8.5f, "#17191c");
            body += Circle(ax, ay + 44, 8.5f, "#17191c");
        }
        else
        {
            // a phone or a tablet: the metal frame, the black glass around the screen
            defs += Linear("metal", 0, 0, bodySize.x, bodySize.y, metal);
            body += Path(RoundRect(b.reach, b.reach, bw, bh, b.outer) + " " + RoundRect(ox, oy, padW, padH, b.outer - b.border),
                         "url(#metal)", 1.0f, true);
            body += Path(RoundRect(ox, oy, padW, padH, b.outer - b.border) + " " + screenHole, bezel, 1.0f, true);
            // the glass: a line of light along the bezel's edge
            body += Path(RoundRect(ox, oy, padW, padH, b.outer - b.border) + " " +
                         RoundRect(ox + 1, oy + 1, padW - 2, padH - 2, b.outer - b.border - 1), "#ffffff", 0.07f, true);

            // the side buttons, out of the frame: a metal bar lit across its width
            int n = 0;
            for (auto& btn : ButtonsOf(*this, landscape))
            {
                String id = "btn" + (String)(n++);
                float x, y, w, h;
                if (btn.side == 'l' || btn.side == 'r')
                {
                    x = btn.side == 'l' ? 0 : ox + padW + b.border;
                    y = oy + btn.at*padH; w = b.reach; h = btn.len*padH;
                    defs += Linear(id, x, 0, x + w, 0, buttons);
                }
                else
                {
                    y = btn.side == 't' ? 0 : oy + padH + b.border;
                    x = ox + btn.at*padW; w = btn.len*padW; h = b.reach;
                    defs += Linear(id, 0, y, 0, y + h, buttons);
                }
                body += Path(RoundRect(x, y, w, h, 1.5f), "url(#" + id + ")");
            }

            auto lens = [&](float cx, float cy, float r, const String& id) {
                defs += Radial(id, cx - r*0.2f, cy - r*0.24f, r, { { 0.0f, "#33465c" }, { 0.3f, "#0d131a" }, { 1.0f, "#000000" } });
                body += Circle(cx, cy, r, "url(#" + id + ")");
            };

            if (look == "island")
            {
                // the island over the screen, the home bar under it
                if (!flip)
                {
                    float ix = ox + padW*0.5f - 62, iy = oy + b.pt + 11;
                    body += Path(RoundRect(ix, iy, 124, 36, 18), "#000000");
                    lens(ix + 124 - 13 - 5.5f, iy + 12.5f + 5.5f, 5.5f, "lens");
                    body += Path(RoundRect(ox + padW*0.5f - 68, oy + padH - b.pb - 8 - 5, 136, 5, 2.5f), "#ffffff", 0.78f);
                }
                else
                {
                    float ix = ox + b.pl + 11, iy = oy + padH*0.5f - 62;
                    body += Path(RoundRect(ix, iy, 36, 124, 18), "#000000");
                    lens(ix + 12.5f + 5.5f, iy + 13 + 5.5f, 5.5f, "lens");
                    body += Path(RoundRect(ox + padW*0.5f - 105, oy + padH - b.pb - 6 - 5, 210, 5, 2.5f), "#ffffff", 0.78f);
                }
            }
            else if (look == "punch")
            {
                // a hole for the camera
                if (!flip)
                    lens(ox + padW*0.5f, oy + b.pt + 12 + 6, 6, "lens");
                else
                    lens(ox + b.pl + 12 + 6, oy + padH*0.5f, 6, "lens");
            }
            else if (look == "home")
            {
                // the forehead and the chin: the earpiece, the camera, the home button
                if (!flip)
                {
                    body += Path(RoundRect(ox + padW*0.5f - 24, oy + b.pt*0.5f - 3, 48, 6, 3), "#1d2024");
                    lens(ox + padW*0.5f - 46 + 5, oy + b.pt*0.5f, 5, "lens");
                    float hx = ox + padW*0.5f, hy = oy + padH - b.pb*0.5f;
                    body += Circle(hx, hy, 27, "#2a2e33");
                    body += Circle(hx, hy, 25.5f, "#121417");
                }
                else
                {
                    body += Path(RoundRect(ox + b.pl*0.5f - 3, oy + padH*0.5f - 24, 6, 48, 3), "#1d2024");
                    lens(ox + b.pl*0.5f, oy + padH*0.5f + 36 + 5, 5, "lens");
                    float hx = ox + padW - b.pr*0.5f, hy = oy + padH*0.5f;
                    body += Circle(hx, hy, 27, "#2a2e33");
                    body += Circle(hx, hy, 25.5f, "#121417");
                }
            }
            else if (look == "tablet")
            {
                // a camera in the middle of the long or the short bezel
                if (!flip)
                    lens(ox + padW*0.5f, oy + b.pt*0.5f, 3.5f, "lens");
                else
                    lens(ox + b.pl*0.5f, oy + padH*0.5f, 3.5f, "lens");
            }
        }

        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" + Num(bodySize.x) + "\" height=\"" + Num(bodySize.y) +
            "\" viewBox=\"0 0 " + Num(bodySize.x) + " " + Num(bodySize.y) + "\">\n<defs>\n" + defs + "</defs>\n" + body + "</svg>\n";
    }

    const Vector<GameDevice>& GetGameDevices()
    {
        static Vector<GameDevice> devices = {
            Make("iphone-se", "iPhone SE", "Phones", "phone", "home", 375, 667, 750, 1334, 4.7f, 16, 72, 0),
            Make("iphone-16", "iPhone 16", "Phones", "phone", "island", 393, 852, 1179, 2556, 6.1f, 12, 12, 55, 59, 34),
            Make("iphone-16-plus", "iPhone 16 Plus", "Phones", "phone", "island", 430, 932, 1290, 2796, 6.7f, 12, 12, 55, 59, 34),
            Make("iphone-16-pro", "iPhone 16 Pro", "Phones", "phone", "island", 402, 874, 1206, 2622, 6.3f, 10, 10, 62, 62, 34),
            Make("iphone-16-pro-max", "iPhone 16 Pro Max", "Phones", "phone", "island", 440, 956, 1320, 2868, 6.9f, 10, 10, 62, 62, 34),
            Make("pixel-8", "Pixel 8", "Phones", "phone", "punch", 412, 915, 1080, 2400, 6.2f, 10, 11, 42, 32, 0),
            Make("pixel-8-pro", "Pixel 8 Pro", "Phones", "phone", "punch", 448, 998, 1344, 2992, 6.7f, 9, 10, 40, 32, 0),
            Make("galaxy-s24", "Galaxy S24", "Phones", "phone", "punch", 360, 780, 1080, 2340, 6.2f, 8, 9, 36, 28, 0),
            Make("galaxy-s24-ultra", "Galaxy S24 Ultra", "Phones", "phone", "punch", 384, 824, 1440, 3120, 6.8f, 8, 8, 14, 28, 0),
            Make("ipad-mini", "iPad mini", "Tablets", "tablet", "tablet", 744, 1133, 1488, 2266, 8.3f, 22, 22, 22, 24, 20),
            Make("ipad", "iPad", "Tablets", "tablet", "tablet", 820, 1180, 1640, 2360, 10.9f, 24, 24, 22, 24, 20),
            Make("ipad-pro-11", "iPad Pro 11\"", "Tablets", "tablet", "tablet", 834, 1210, 1668, 2420, 11, 20, 20, 22, 24, 20),
            Make("ipad-pro-13", "iPad Pro 13\"", "Tablets", "tablet", "tablet", 1032, 1376, 2064, 2752, 13, 22, 22, 22, 24, 20),
            Make("galaxy-tab-s9", "Galaxy Tab S9", "Tablets", "tablet", "tablet", 800, 1280, 1600, 2560, 11, 18, 18, 14),
            Make("steam-deck", "Steam Deck", "Desktop", "desktop", "console", 1280, 800, 1280, 800, 7, 36, 190, 4),
            Make("720p", "Window 720p", "Desktop", "desktop", "window", 1280, 720, 1280, 720, 0, 0, 0, 0),
            Make("1080p", "Window 1080p", "Desktop", "desktop", "window", 1920, 1080, 1920, 1080, 0, 0, 0, 0),
        };
        return devices;
    }

    const GameDevice* FindGameDevice(const String& id)
    {
        for (auto& device : GetGameDevices())
        {
            if (device.id == id)
                return &device;
        }
        return nullptr;
    }
}
