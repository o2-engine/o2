#include "o2Editor/stdafx.h"
#include "PipelinePartFit.h"

#include <cmath>
#include <map>
#include <mutex>

#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor
{
    namespace PipelinePartFit
    {
        using UiLayout::UiImage;
        using UiDoc::JsRound;

        // The crop's longer side while matching: the sweep grows with the square of it
        static const double SIDE = 168;
        // Scales tried, as a ratio to the nominal one
        static const double RATIO_MIN = 0.45, RATIO_MAX = 1.6;
        // A measured scale within this of the nominal one is the nominal one
        static const double SAME = 0.12;
        // How much better than the nominal placement a new one must score to be believed (a smaller one does more)
        static const double GAIN = 6, GAIN_SMALLER = 8, SMALLER_COLOUR = 20, SMALLER_OUTLINE = 40;
        // The boundary's value when too little of the outline is on the crop to tell
        static const double UNSEEN = 30;

        // A size of the sprite at its best place on the small crop
        struct Placed
        {
            double score = 0, colour = 0, outline = 0;
            int    x = 0, y = 0, w = 0, h = 0;
        };

        // The sprite at a size: its solid interior as sampled points (x, y), its outline as sampled points with the place a
        // few pixels inside it and a few outside (x, y, inX, inY, outX, outY)
        struct Laid
        {
            int              w = 0, h = 0;
            std::vector<int> solid, contour;
            UiImage          img;
        };

        static std::vector<int> Thin(const std::vector<int>& pts, int size, int max)
        {
            int n = (int)pts.size()/size;
            int step = std::max(1, (int)std::ceil((double)n/max));
            std::vector<int> out;
            for (int i = 0; i < n; i += step)
            {
                for (int k = 0; k < size; k++)
                    out.push_back(pts[i*size + k]);
            }
            return out;
        }

        static bool Points(const UiImage& img, Laid& out)
        {
            int w = img.w, h = img.h;
            auto op = [&](int x, int y, int a) { return x >= 0 && y >= 0 && x < w && y < h && img.data[(y*w + x)*4 + 3] >= a; };
            std::vector<int> solid, contour;
            int reach = std::max(2, (int)JsRound(std::min(w, h)*0.06));
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    if (!op(x, y, 128))
                        continue;
                    bool inside = op(x - 1, y, 128) && op(x + 1, y, 128) && op(x, y - 1, 128) && op(x, y + 1, 128);
                    if (inside)
                    {
                        if (op(x, y, 220))
                        {
                            solid.push_back(x);
                            solid.push_back(y);
                        }
                        continue;
                    }

                    int nx = 0, ny = 0;
                    for (int v = -3; v <= 3; v++)
                    {
                        for (int u = -3; u <= 3; u++)
                        {
                            if (op(x + u, y + v, 128))
                            {
                                nx += u;
                                ny += v;
                            }
                        }
                    }
                    double len = std::hypot((double)nx, (double)ny);
                    if (len < 1)
                        continue;
                    int ix = x + (int)JsRound(nx/len*reach), iy = y + (int)JsRound(ny/len*reach);
                    int ox = x - (int)JsRound(nx/len*reach), oy = y - (int)JsRound(ny/len*reach);
                    if (!op(ix, iy, 128) || op(ox, oy, 128))
                        continue;
                    for (int v : { x, y, ix, iy, ox, oy })
                        contour.push_back(v);
                }
            }
            if (solid.size() < 2*24 || contour.size() < 6*12)
                return false;

            out.w = w;
            out.h = h;
            out.solid = Thin(solid, 2, 420);
            out.contour = Thin(contour, 6, 260);
            out.img = img;
            return true;
        }

        static UiImage Resized(const UiImage& img, int w, int h)
        {
            return w == img.w && h == img.h ? img : img.Resize(std::max(1, w), std::max(1, h));
        }

        static UInt64 HashBytes(UInt64 h, const UInt8* data, size_t n)
        {
            for (size_t i = 0; i < n; i++)
            {
                h ^= data[i];
                h *= 1099511628211ull;
            }
            return h;
        }

        std::optional<Fit> FitPartOnCrop(const UiImage& crop, const UiImage& sprite, double nominal)
        {
            if (!(nominal > 0) || crop.w < 12 || crop.h < 12 || sprite.w < 8 || sprite.h < 8)
                return std::nullopt;

            // A plate fills its frame: there is no outline to lay anywhere
            int clear = 0;
            for (size_t i = 3; i < sprite.data.size(); i += 4*7)
            {
                if (sprite.data[i] < 128)
                    clear++;
            }
            if (clear < 8)
                return std::nullopt;

            static std::mutex lock;
            static std::map<UInt64, std::optional<Fit>> memo;
            UInt64 key = HashBytes(1469598103934665603ull, crop.data.data(), crop.data.size());
            key = HashBytes(key, sprite.data.data(), sprite.data.size());
            int dims[5] = { crop.w, crop.h, sprite.w, sprite.h, (int)JsRound(nominal*1e6) };
            key = HashBytes(key, (const UInt8*)dims, sizeof(dims));
            {
                std::lock_guard<std::mutex> guard(lock);
                auto it = memo.find(key);
                if (it != memo.end())
                    return it->second;
            }

            double K = std::min(1.0, SIDE/std::max(crop.w, crop.h));
            UiImage small = K < 1 ? Resized(crop, std::max(8, (int)JsRound(crop.w*K)), std::max(8, (int)JsRound(crop.h*K))) : crop;
            int W = small.w, H = small.h;
            // A working copy of the sprite no larger than it will ever be laid
            double top = std::min(1.0, std::max(W, H)*1.25/std::max(sprite.w, sprite.h));
            UiImage base = top < 1 ? Resized(sprite, std::max(4, (int)JsRound(sprite.w*top)), std::max(4, (int)JsRound(sprite.h*top))) : sprite;

            struct Around
            {
                int x = 0, y = 0, r = 0;
            };
            auto at = [&](double ratio, const Around* around) -> std::optional<Placed> {
                int tw = (int)JsRound(sprite.w/(nominal*ratio)*K), th = (int)JsRound(sprite.h/(nominal*ratio)*K);
                if (tw < 8 || th < 6 || tw > W*1.12 || th > H*1.12)
                    return std::nullopt;
                Laid laid;
                if (!Points(Resized(base, tw, th), laid))
                    return std::nullopt;

                const std::vector<int>& solid = laid.solid;
                const std::vector<int>& contour = laid.contour;
                const UInt8* data = laid.img.data.data();
                const UInt8* sd = small.data.data();
                int mx = (int)JsRound(tw*0.08), my = (int)JsRound(th*0.08);
                int step = around ? 1 : std::max(1, (int)JsRound(std::min(tw, th)/44.0));
                int x0 = around ? around->x - around->r : -mx, x1 = around ? around->x + around->r : W - tw + mx;
                int y0 = around ? around->y - around->r : -my, y1 = around ? around->y + around->r : H - th + my;
                std::optional<Placed> best;
                for (int dy = y0; dy <= y1; dy += step)
                {
                    for (int dx = x0; dx <= x1; dx += step)
                    {
                        double cd = 0;
                        for (size_t i = 0; i < solid.size(); i += 2)
                        {
                            int cx = dx + solid[i], cy = dy + solid[i + 1];
                            if (cx < 0 || cy < 0 || cx >= W || cy >= H)
                            {
                                cd += 160;
                                continue;
                            }
                            size_t a = ((size_t)solid[i + 1]*tw + solid[i])*4, b = ((size_t)cy*W + cx)*4;
                            cd += (std::abs(data[a] - sd[b]) + std::abs(data[a + 1] - sd[b + 1]) + std::abs(data[a + 2] - sd[b + 2]))/3.0;
                        }

                        double edge = 0;
                        int seen = 0;
                        for (size_t i = 0; i < contour.size(); i += 6)
                        {
                            int ax = dx + contour[i + 2], ay = dy + contour[i + 3], bx = dx + contour[i + 4], by = dy + contour[i + 5];
                            if (ax < 0 || ay < 0 || ax >= W || ay >= H || bx < 0 || by < 0 || bx >= W || by >= H)
                                continue;
                            size_t a = ((size_t)ay*W + ax)*4, b = ((size_t)by*W + bx)*4;
                            edge += std::min(60.0, (std::abs(sd[a] - sd[b]) + std::abs(sd[a + 1] - sd[b + 1]) + std::abs(sd[a + 2] - sd[b + 2]))/3.0);
                            seen++;
                        }

                        double colour = cd/(solid.size()/2.0);
                        double outline = seen >= contour.size()/6.0/4.0 ? edge/seen : UNSEEN;
                        double score = -colour + 0.8*outline;
                        if (!best || score > best->score)
                            best = Placed{ score, colour, outline, dx, dy, tw, th };
                    }
                }
                return best;
            };

            std::optional<Placed> stay;
            std::optional<Placed> best;
            double bestRatio = 1;
            for (double ratio = RATIO_MIN; ratio <= RATIO_MAX*1.001; ratio *= 1.045)
            {
                auto r = at(ratio, nullptr);
                if (!r)
                    continue;
                if (!best || r->score > best->score)
                {
                    best = r;
                    bestRatio = ratio;
                }
                if (std::fabs(ratio - 1) <= 0.06 && (!stay || r->score > stay->score))
                    stay = r;
            }
            // The nominal size itself, whatever the sweep's steps were
            auto exact = at(1, nullptr);
            if (exact && (!stay || exact->score > stay->score))
                stay = exact;

            std::optional<Fit> out;
            bool believed = best && std::fabs(bestRatio - 1) > SAME &&
                (!stay || (bestRatio < 1 ? best->score > stay->score + GAIN
                                         : best->score > stay->score + GAIN_SMALLER && best->colour <= SMALLER_COLOUR &&
                                               best->outline >= SMALLER_OUTLINE));
            if (best && believed)
            {
                // A finer pass around the coarse answer
                Placed coarse = *best;
                double coarseRatio = bestRatio;
                for (double k : { 0.97, 0.985, 1.015, 1.03 })
                {
                    Around around{ (int)JsRound(coarse.x + (coarse.w - coarse.w/k)/2), (int)JsRound(coarse.y + (coarse.h - coarse.h/k)/2), 2 };
                    auto r = at(coarseRatio*k, &around);
                    if (r && r->score > best->score)
                    {
                        best = r;
                        bestRatio = coarseRatio*k;
                    }
                }
                Fit fit;
                fit.scale = JsRound(nominal*bestRatio*10000)/10000;
                fit.ratio = JsRound(bestRatio*1000)/1000;
                fit.x = best->x/K;
                fit.y = best->y/K;
                fit.w = sprite.w/fit.scale;
                fit.h = sprite.h/fit.scale;
                out = fit;
            }

            std::lock_guard<std::mutex> guard(lock);
            if (memo.size() > 600)
                memo.erase(memo.begin());
            memo[key] = out;
            return out;
        }
    }
}
