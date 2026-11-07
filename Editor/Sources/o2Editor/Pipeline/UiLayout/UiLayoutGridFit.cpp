#include "o2Editor/stdafx.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutGridFit.h"

#include <cmath>
#include <limits>
#include <map>

namespace Editor::UiLayout
{
    namespace
    {
        const double phaseStep = 0.25;

        double Luma(const UiImage& img, int x, int y)
        {
            const UInt8* p = img.At(x, y);
            return 0.299*p[0] + 0.587*p[1] + 0.114*p[2];
        }

        // Mean |gradient| along the axis at every position of [a0, a1), over the span [b0, b1) across it
        std::vector<double> EdgeProfile(const UiImage& img, bool alongX, int a0, int a1, int b0, int b1)
        {
            std::vector<double> out;
            for (int a = a0; a < a1 - 1; a++)
            {
                double sum = 0;
                for (int b = b0; b < b1; b++)
                    sum += alongX ? std::abs(Luma(img, a + 1, b) - Luma(img, a, b)) : std::abs(Luma(img, b, a + 1) - Luma(img, b, a));
                out.push_back(sum/std::max(1, b1 - b0));
            }
            return out;
        }

        // The colour spread (sum of the channels' deviations) of every line across the axis: low in a gutter
        std::vector<double> SpreadProfile(const UiImage& img, bool alongX, int a0, int a1, int b0, int b1)
        {
            std::vector<double> out;
            int n = std::max(1, b1 - b0);
            for (int a = a0; a < a1; a++)
            {
                double spread = 0;
                for (int ch = 0; ch < 3; ch++)
                {
                    double sum = 0, sq = 0;
                    for (int b = b0; b < b1; b++)
                    {
                        double v = (alongX ? img.At(a, b) : img.At(b, a))[ch];
                        sum += v;
                        sq += v*v;
                    }
                    double mean = sum/n;
                    spread += std::sqrt(std::max(0.0, sq/n - mean*mean));
                }
                out.push_back(spread);
            }
            return out;
        }

        // The period of the profile near cell (0.6..1.6 of it), with the strength of its autocorrelation
        bool Period(const std::vector<double>& profile, double cell, double& pitch, double& strength)
        {
            int n = (int)profile.size();
            double mean = 0;
            for (double v : profile)
                mean += v;
            mean /= std::max(1, n);

            std::vector<double> p(profile.size());
            for (int i = 0; i < n; i++)
                p[i] = profile[i] - mean;

            auto ac = [&](int lag) { double s = 0; for (int i = 0; i + lag < n; i++) s += p[i]*p[i + lag]; return s; };
            double zero = ac(0);
            if (zero <= 0)
                return false;

            int lo = std::max(2, (int)std::floor(0.6*cell)), hi = std::min(n - 2, (int)std::floor(1.6*cell) + 1);
            if (hi <= lo)
                return false;

            int lag = lo;
            double best = -std::numeric_limits<double>::infinity();
            for (int l = lo; l <= hi; l++)
            {
                double v = ac(l)/zero;
                if (v > best)
                {
                    best = v;
                    lag = l;
                }
            }

            double a = ac(lag - 1)/zero, b = best, c = ac(lag + 1)/zero, den = a - 2*b + c;
            pitch = lag + (den != 0 ? (a - c)/(2*den) : 0.0);
            strength = best;
            return true;
        }

        // Mean spread at lines start + k*pitch (k = 0..count), a 3 px window each
        double LinesSpread(const std::vector<double>& spread, int a0, double start, double pitch, int count)
        {
            double sum = 0;
            int used = 0;
            for (int k = 0; k <= count; k++)
            {
                int i = (int)JsRound(start + k*pitch - a0);
                if (i < 0 || i >= (int)spread.size())
                    continue;

                double s = 0;
                int m = 0;
                for (int j = std::max(0, i - 1); j <= std::min((int)spread.size() - 1, i + 1); j++)
                {
                    s += spread[j];
                    m++;
                }
                sum += s/m;
                used++;
            }
            return used ? sum/used : std::numeric_limits<double>::infinity();
        }

        // The offset of the quietest lines: the middle of the run of offsets as quiet as the quietest (a flat gutter is as
        // quiet all across, its middle is the border)
        double QuietestOffset(const std::vector<double>& scores, double pitch)
        {
            int n = (int)scores.size();
            int m = 0;
            double mean = scores[0];
            for (int i = 1; i < n; i++)
            {
                mean += scores[i];
                if (scores[i] < scores[m])
                    m = i;
            }
            mean /= n;
            double limit = scores[m] + 0.05*(mean - scores[m]) + 1e-9;
            int l = m, r = m;
            while (r - l < n - 1 && scores[((l - 1)%n + n)%n] <= limit)
                l--;
            while (r - l < n - 1 && scores[(r + 1)%n] <= limit)
                r++;
            return std::fmod(std::fmod((l + r)/2.0*phaseStep, pitch) + pitch, pitch);
        }

        String Fixed1(double v)
        {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.1f", v);
            return buffer;
        }
    }

    std::optional<GridAxisFit> FitGridAxis(const UiImage& img, bool alongX, double s0, double s1, double b0, double b1, int count,
                                           const std::set<int>& occupied)
    {
        int size = alongX ? img.w : img.h;
        int cross = alongX ? img.h : img.w;
        int c0 = std::max(0, (int)std::floor(b0)), c1 = std::min(cross, (int)std::ceil(b1));
        int i0 = std::max(0, (int)std::floor(s0)), i1 = std::min(size, (int)std::ceil(s1));
        double cell = (s1 - s0)/count;
        if (count < 3 || occupied.empty() || c1 - c0 < 4 || i1 - i0 < 8)
            return std::nullopt;

        double pitch = 0, strength = 0;
        if (!Period(EdgeProfile(img, alongX, i0, i1, c0, c1), cell, pitch, strength) || strength < 0.25)
            return std::nullopt;

        // Cell borders: the offset whose lines run through the steadiest colour, over the span widened by a pitch
        int a0 = std::max(0, (int)std::floor(s0 - pitch)), a1 = std::min(size, (int)std::ceil(s1 + pitch));
        auto spread = SpreadProfile(img, alongX, a0, a1, c0, c1);
        std::vector<double> scores;
        int lines = (int)std::ceil((a1 - a0)/pitch);
        for (double off = 0; off < pitch; off += phaseStep)
            scores.push_back(LinesSpread(spread, a0, a0 + off, pitch, lines));
        double phase = a0 + QuietestOffset(scores, pitch);

        // The first occupied cell, placed by the model's own cell size, fixes which border the grid starts at
        int first = *occupied.begin(), last = *occupied.rbegin();
        double k = std::floor((s0 + (first + 0.5)*cell - phase)/pitch);
        double origin = phase + (k - first)*pitch;

        // Empty cells at the ends whose centre falls outside the model's box are not there
        int lo = 0, hi = count - 1;
        while (hi > last && origin + (hi + 0.5)*pitch > s1)
            hi--;
        while (lo < first && origin + (lo + 0.5)*pitch < s0)
            lo++;

        GridAxisFit fit;
        fit.pitch = pitch;
        fit.start = origin + lo*pitch;
        fit.count = hi - lo + 1;
        fit.shift = lo;

        bool changed = fit.count != count || std::abs(pitch - cell) > 0.02*cell || std::abs(fit.start - s0) > 1.5;
        if (!changed)
            return std::nullopt;

        double ours = LinesSpread(spread, a0, fit.start, fit.pitch, fit.count);
        double theirs = LinesSpread(spread, a0, s0, cell, count);
        if (!(ours < theirs))
            return std::nullopt;

        return fit;
    }

    Vector<String> FitGrids(UiDoc& doc, const UiImage& design)
    {
        Vector<String> notes;
        for (auto& grid : doc.els)
        {
            if (grid->kind != "grid" || !grid->rows || !grid->cols)
                continue;

            Vector<Ref<UiEl>> cells;
            std::set<int> rowsUsed, colsUsed;
            for (auto& e : doc.els)
            {
                if (e->cell && e->cell->of == grid->id)
                {
                    cells.Add(e);
                    rowsUsed.insert(e->cell->row);
                    colsUsed.insert(e->cell->col);
                }
            }
            if (cells.IsEmpty())
                continue;

            Box b = grid->box;
            int cols = *grid->cols, rows = *grid->rows;
            auto fx = FitGridAxis(design, true, b.x, b.x + b.w, b.y, b.y + b.h, cols, colsUsed);
            auto fy = FitGridAxis(design, false, b.y, b.y + b.h, b.x, b.x + b.w, rows, rowsUsed);
            if (!fx && !fy)
                continue;

            GridAxisFit x = fx ? *fx : GridAxisFit{ b.w/cols, b.x, cols, 0 };
            GridAxisFit y = fy ? *fy : GridAxisFit{ b.h/rows, b.y, rows, 0 };
            Vector<String> parts;
            if (fx)
                parts.Add(fx->count != cols ? (String)cols + "\xE2\x86\x92" + (String)fx->count + " columns"
                                            : "columns " + Fixed1(b.w/cols) + "\xE2\x86\x92" + Fixed1(fx->pitch) + " px");
            if (fy)
                parts.Add(fy->count != rows ? (String)rows + "\xE2\x86\x92" + (String)fy->count + " rows"
                                            : "rows " + Fixed1(b.h/rows) + "\xE2\x86\x92" + Fixed1(fy->pitch) + " px");
            String line = grid->id + ": ";
            for (int i = 0; i < parts.Count(); i++)
                line += (i ? ", " : "") + parts[i];
            notes.Add(line);

            double gx0 = std::max(0.0, x.start), gy0 = std::max(0.0, y.start);
            grid->box = Box(gx0, gy0, std::min((double)design.w, x.start + x.count*x.pitch) - gx0,
                            std::min((double)design.h, y.start + y.count*y.pitch) - gy0);
            grid->cols = x.count;
            grid->rows = y.count;
            grid->measured = "grid";

            std::map<std::string, Vector<Ref<UiEl>>> texts;
            for (auto& e : doc.els)
            {
                if (e->kind == "text" && e->measured == "analysis")
                    texts[e->parent.Data()].Add(e);
            }

            for (auto& c : cells)
            {
                c->cell->row -= y.shift;
                c->cell->col -= x.shift;
                c->box = Box(x.start + c->cell->col*x.pitch, y.start + c->cell->row*y.pitch, x.pitch, y.pitch);
                c->measured = "grid";
                auto found = texts.find(c->id.Data());
                if (found != texts.end())
                {
                    for (auto& t : found->second)
                        t->box = c->box;
                }
            }
        }
        return notes;
    }
}
