#include "o2Editor/stdafx.h"
#include "UiKitPrep.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    // --- hash ---------------------------------------------------------------------------------------------

    String Sha1Hex(const String& bytes)
    {
        uint32_t h[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
        std::vector<uint8_t> msg(bytes.Data(), bytes.Data() + bytes.Length());
        uint64_t bitLen = (uint64_t)msg.size()*8;
        msg.push_back(0x80);
        while (msg.size() % 64 != 56)
            msg.push_back(0);
        for (int i = 7; i >= 0; i--)
            msg.push_back((uint8_t)(bitLen >> (i*8)));

        auto rol = [](uint32_t v, int n) { return (v << n) | (v >> (32 - n)); };
        for (size_t chunk = 0; chunk < msg.size(); chunk += 64)
        {
            uint32_t w[80];
            for (int i = 0; i < 16; i++)
            {
                w[i] = ((uint32_t)msg[chunk + i*4] << 24) | ((uint32_t)msg[chunk + i*4 + 1] << 16) |
                    ((uint32_t)msg[chunk + i*4 + 2] << 8) | (uint32_t)msg[chunk + i*4 + 3];
            }
            for (int i = 16; i < 80; i++)
                w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

            uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
            for (int i = 0; i < 80; i++)
            {
                uint32_t f, k;
                if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
                else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
                else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
                else { f = b ^ c ^ d; k = 0xCA62C1D6; }
                uint32_t t = rol(a, 5) + f + e + k + w[i];
                e = d;
                d = c;
                c = rol(b, 30);
                b = a;
                a = t;
            }
            h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
        }

        char out[41];
        for (int i = 0; i < 5; i++)
            snprintf(out + i*8, 9, "%08x", h[i]);
        return String(out);
    }

    String InputHash(const String& bytes)
    {
        return Sha1Hex(bytes).SubStr(0, 16);
    }

    // --- backdrop -------------------------------------------------------------------------------------------

    static UInt8 Alpha(const UiImage& img, int x, int y) { return img.At(x, y)[3]; }

    // Median colour of the opaque border pixels; false when the border is see-through already
    static bool BorderColor(const UiImage& img, double key[3])
    {
        std::vector<int> rs, gs, bs;
        int clear = 0, total = 0;
        auto take = [&](int x, int y) {
            const UInt8* p = img.At(x, y);
            total++;
            if (p[3] < 128)
            {
                clear++;
                return;
            }
            rs.push_back(p[0]);
            gs.push_back(p[1]);
            bs.push_back(p[2]);
        };
        for (int x = 0; x < img.w; x++)
        {
            take(x, 0);
            take(x, img.h - 1);
        }
        for (int y = 1; y < img.h - 1; y++)
        {
            take(0, y);
            take(img.w - 1, y);
        }
        if (clear > total*0.5 || rs.empty())
            return false;

        auto med = [](std::vector<int>& a) {
            std::sort(a.begin(), a.end());
            return (double)a[a.size() >> 1];
        };
        key[0] = med(rs);
        key[1] = med(gs);
        key[2] = med(bs);
        return true;
    }

    int KeyBackground(UiImage& img, double tol /*= 30*/)
    {
        double key[3];
        if (img.IsEmpty() || !BorderColor(img, key))
            return 0;

        int w = img.w, h = img.h;
        std::vector<UInt8> seen((size_t)w*h, 0);
        std::vector<int> stack;
        auto dist = [&](int p) {
            const UInt8* d = img.data.data() + (size_t)p*4;
            double dr = d[0] - key[0], dg = d[1] - key[1], db = d[2] - key[2];
            return std::sqrt(dr*dr + dg*dg + db*db);
        };
        auto push = [&](int x, int y) {
            int p = y*w + x;
            if (!seen[p] && img.data[(size_t)p*4 + 3] > 0 && dist(p) < tol*2)
            {
                seen[p] = 1;
                stack.push_back(p);
            }
        };
        for (int x = 0; x < w; x++)
        {
            push(x, 0);
            push(x, h - 1);
        }
        for (int y = 0; y < h; y++)
        {
            push(0, y);
            push(w - 1, y);
        }

        int cleared = 0;
        while (!stack.empty())
        {
            int p = stack.back();
            stack.pop_back();
            double d = dist(p);
            UInt8& a = img.data[(size_t)p*4 + 3];
            if (d < tol)
            {
                a = 0;
                cleared++;
            }
            else
            {
                // A soft edge; it does not spread past it
                a = (UInt8)std::min((double)a, JsRound(((d - tol)/tol)*255));
                continue;
            }
            int x = p % w, y = p/w;
            if (x > 0) push(x - 1, y);
            if (x < w - 1) push(x + 1, y);
            if (y > 0) push(x, y - 1);
            if (y < h - 1) push(x, y + 1);
        }
        return cleared;
    }

    UiPixelBox AlphaBox(const UiImage& img, int min /*= 6*/)
    {
        int x0 = img.w, y0 = img.h, x1 = -1, y1 = -1;
        for (int y = 0; y < img.h; y++)
        {
            for (int x = 0; x < img.w; x++)
            {
                if (Alpha(img, x, y) > min)
                {
                    x0 = std::min(x0, x);
                    x1 = std::max(x1, x);
                    y0 = std::min(y0, y);
                    y1 = std::max(y1, y);
                }
            }
        }
        if (x1 < 0)
            return UiPixelBox{ 0, 0, img.w, img.h };
        return UiPixelBox{ x0, y0, x1 - x0 + 1, y1 - y0 + 1 };
    }

    // --- nine-slice -----------------------------------------------------------------------------------------

    // How many pixels of two columns (vertical) or rows differ visibly (colours premultiplied by alpha)
    static int LineDiff(const UiImage& img, int a, int b, bool vertical, double step = 20)
    {
        int n = vertical ? img.h : img.w;
        int count = 0;
        for (int k = 0; k < n; k++)
        {
            const UInt8* i = vertical ? img.At(a, k) : img.At(k, a);
            const UInt8* j = vertical ? img.At(b, k) : img.At(k, b);
            double ai = i[3]/255.0, aj = j[3]/255.0;
            double d = std::abs((int)i[3] - (int)j[3]);
            for (int c = 0; c < 3; c++)
                d = std::max(d, std::fabs(i[c]*ai - j[c]*aj));
            if (d > step)
                count++;
        }
        return count;
    }

    // Two lines count as equal when at most this many of their pixels differ
    static int SameLimit(int len)
    {
        return std::max(2, (int)std::floor(len*0.012));
    }

    // The stretchable run of equal neighbouring columns (vertical) or rows: the run through the middle when it is long
    // enough, else the longest
    static std::optional<std::pair<int, int>> UniformRun(const UiImage& img, bool vertical)
    {
        int len = vertical ? img.w : img.h;
        int limit = SameLimit(vertical ? img.h : img.w);
        double minRun = std::max(3.0, JsRound(len*0.12));
        std::vector<std::pair<int, int>> runs;
        int start = 0;
        for (int i = 1; i <= len; i++)
        {
            bool same = i < len && LineDiff(img, i - 1, i, vertical) <= limit;
            if (!same)
            {
                runs.push_back({ start, i - 1 });
                start = i;
            }
        }

        std::vector<std::pair<int, int>> longRuns;
        for (auto& r : runs)
        {
            if (r.second - r.first + 1 >= minRun)
                longRuns.push_back(r);
        }
        if (longRuns.empty())
            return std::nullopt;

        double mid = (len - 1)/2.0;
        for (auto& r : longRuns)
        {
            if (r.first <= mid + 1 && r.second >= mid - 1)
                return r;
        }

        std::pair<int, int> best = longRuns[0];
        for (size_t i = 1; i < longRuns.size(); i++)
        {
            if (longRuns[i].second - longRuns[i].first > best.second - best.first)
                best = longRuns[i];
        }
        return best;
    }

    // Corner radius from the alpha along the diagonal -> a border a bit wider than it
    static double CornerBorder(const UiImage& img)
    {
        int r = 0;
        const int corners[4][4] = { { 0, 0, 1, 1 }, { img.w - 1, 0, -1, 1 }, { 0, img.h - 1, 1, -1 }, { img.w - 1, img.h - 1, -1, -1 } };
        for (auto& c : corners)
        {
            for (int d = 0; d < std::min(img.w, img.h)/2.0; d++)
            {
                if (Alpha(img, c[0] + c[2]*d, c[1] + c[3]*d) > 128)
                {
                    r = std::max(r, d);
                    break;
                }
            }
        }
        return JsRound(r*3.4 + 4);
    }

    UiNineFound DetectNine(const UiImage& img)
    {
        UiNineFound out;
        out.uniformX = UniformRun(img, true);
        out.uniformY = UniformRun(img, false);
        double fb = CornerBorder(img);
        auto clampX = [&](double v) { return std::max(0.0, std::min(v, std::floor((img.w - 1)/2.0))); };
        auto clampY = [&](double v) { return std::max(0.0, std::min(v, std::floor((img.h - 1)/2.0))); };
        auto& ux = out.uniformX;
        auto& uy = out.uniformY;
        out.nine.left = ux ? ux->first : clampX(std::min(fb, std::floor(img.w*0.4)));
        out.nine.right = ux ? img.w - 1 - ux->second : clampX(std::min(fb, std::floor(img.w*0.4)));
        out.nine.top = uy ? uy->first : clampY(std::min(fb, std::floor(img.h*0.4)));
        out.nine.bottom = uy ? img.h - 1 - uy->second : clampY(std::min(fb, std::floor(img.h*0.4)));
        return out;
    }

    // Is every line in [lo, hi] the same as line ref (a little slack for noise)?
    static bool AllSame(const UiImage& img, int lo, int hi, int ref, bool vertical)
    {
        int limit = std::max(3, (int)std::floor((vertical ? img.h : img.w)*0.05));
        for (int i = lo; i <= hi; i++)
        {
            if (LineDiff(img, ref, i, vertical) > limit)
                return false;
        }
        return true;
    }

    // Keeps `keep` columns (rows) of a uniform middle: the sprite shrinks, its stretch does not change. The middle's
    // first and last lines join the borders (they may carry a corner's antialiasing)
    static UiImage Collapse(const UiImage& img, UiNine& n, int keep = 4)
    {
        UiImage out = img;
        {
            int lo = (int)n.left + 1, hi = out.w - 2 - (int)n.right, mid = (lo + hi) >> 1;
            if (hi - lo + 1 > keep + 2 && AllSame(out, lo, hi, mid, true))
            {
                n.left = lo;
                n.right = out.w - 1 - hi;
                int w = (int)n.left + keep + (int)n.right;
                UiImage next = UiImage::Blank(w, out.h);
                for (int y = 0; y < out.h; y++)
                {
                    for (int x = 0; x < w; x++)
                    {
                        int sx = x < n.left ? x : x < n.left + keep ? mid : out.w - (w - x);
                        std::memcpy(next.At(x, y), out.At(sx, y), 4);
                    }
                }
                out = next;
            }
        }
        {
            int lo = (int)n.top + 1, hi = out.h - 2 - (int)n.bottom, mid = (lo + hi) >> 1;
            if (hi - lo + 1 > keep + 2 && AllSame(out, lo, hi, mid, false))
            {
                n.top = lo;
                n.bottom = out.h - 1 - hi;
                int h = (int)n.top + keep + (int)n.bottom;
                UiImage next = UiImage::Blank(out.w, h);
                for (int y = 0; y < h; y++)
                {
                    int sy = y < n.top ? y : y < n.top + keep ? mid : out.h - (h - y);
                    std::memcpy(next.At(0, y), out.At(0, sy), (size_t)out.w*4);
                }
                out = next;
            }
        }
        return out;
    }

    // --- the element -------------------------------------------------------------------------------------------

    static Json BoxJson(const UiPixelBox& b)
    {
        Json out = Json::Object();
        out["x"] = b.x;
        out["y"] = b.y;
        out["w"] = b.w;
        out["h"] = b.h;
        return out;
    }

    bool PrepareElement(const UiPrepInput& input, UiPrepOutput& out)
    {
        UiImage src = input.image;
        if (src.IsEmpty() && !UiImage::Decode(input.data, src))
            return false;

        UiImage img = src;
        if (input.key)
            KeyBackground(img);
        UiPixelBox trim = input.trim ? AlphaBox(img) : UiPixelBox{ 0, 0, img.w, img.h };
        if (trim.w != img.w || trim.h != img.h)
            img = img.Crop(trim.x, trim.y, trim.w, trim.h);

        out = UiPrepOutput();
        out.source = Json::Object();
        out.source["w"] = src.w;
        out.source["h"] = src.h;
        out.source["trim"] = BoxJson(trim);

        if (input.assetType != "sliced")
        {
            out.image = img;
            out.png = img.Png();
            out.width = img.w;
            out.height = img.h;
            return true;
        }

        UiNineFound found = DetectNine(img);
        UiNine nine;
        if (input.nineSlice)
        {
            // Input pixels -> the trimmed picture
            const UiNine& given = *input.nineSlice;
            nine.left = std::max(0.0, given.left - trim.x);
            nine.top = std::max(0.0, given.top - trim.y);
            nine.right = std::max(0.0, given.right - (src.w - trim.x - trim.w));
            nine.bottom = std::max(0.0, given.bottom - (src.h - trim.y - trim.h));
        }
        else
            nine = found.nine;

        if (nine.left + nine.right >= img.w)
        {
            double k = (img.w - 1)/std::max(1.0, nine.left + nine.right);
            nine.left = std::floor(nine.left*k);
            nine.right = std::floor(nine.right*k);
        }
        if (nine.top + nine.bottom >= img.h)
        {
            double k = (img.h - 1)/std::max(1.0, nine.top + nine.bottom);
            nine.top = std::floor(nine.top*k);
            nine.bottom = std::floor(nine.bottom*k);
        }

        out.detected = UiNine{ found.nine.left + trim.x, found.nine.top + trim.y, found.nine.right + (src.w - trim.x - trim.w),
                               found.nine.bottom + (src.h - trim.y - trim.h) };

        if (input.optimize)
        {
            UiImage collapsed = Collapse(img, nine);
            if (collapsed.w != img.w || collapsed.h != img.h)
            {
                Json size = Json::Object();
                size["w"] = collapsed.w;
                size["h"] = collapsed.h;
                out.source["collapsed"] = size;
            }
            img = collapsed;
        }

        out.image = img;
        out.png = img.Png();
        out.width = img.w;
        out.height = img.h;
        out.nineSlice = nine;
        return true;
    }

    bool DetectInputNine(const String& data, bool key, UiInputNine& out)
    {
        UiImage src;
        if (!UiImage::Decode(data, src))
            return false;

        UiImage img = src;
        if (key)
            KeyBackground(img);
        UiPixelBox trim = AlphaBox(img);
        img = img.Crop(trim.x, trim.y, trim.w, trim.h);
        UiNineFound f = DetectNine(img);
        out.nine = UiNine{ f.nine.left + trim.x, f.nine.top + trim.y, f.nine.right + (src.w - trim.x - trim.w),
                           f.nine.bottom + (src.h - trim.y - trim.h) };
        out.uniformX = f.uniformX.has_value();
        out.uniformY = f.uniformY.has_value();
        out.w = src.w;
        out.h = src.h;
        return true;
    }
}
