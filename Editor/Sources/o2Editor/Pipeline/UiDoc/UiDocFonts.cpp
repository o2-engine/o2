#include "o2Editor/stdafx.h"
#include "UiDocFonts.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H
#include FT_ADVANCES_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    String UiFontsHome()
    {
        if (const char* env = getenv("O2_UI_FONTS_DIR"))
        {
            String dir = env;
            if (o2FileSystem.IsFileExist(dir + "/Roboto-Regular.ttf"))
                return UiLayout::AbsolutePath(dir);
        }

        String file = UiLayout::FindRepoPath("o2/Tools/UiFonts/Roboto-Regular.ttf");
        if (file.IsEmpty())
            file = UiLayout::FindRepoPath("Tools/UiFonts/Roboto-Regular.ttf");
        return file.IsEmpty() ? String() : o2FileSystem.GetParentPath(file);
    }

    namespace
    {
        // ------------------------------------------------------------------------------------------
        // Pair kerning from a font's GPOS table (the 'kern' feature's pair adjustment lookups): what
        // browsers and librsvg apply through HarfBuzz, and FreeType's FT_Get_Kerning does not read
        // ------------------------------------------------------------------------------------------
        class GposKern
        {
        public:
            // Reads the table; false when the font has no pair kerning there
            bool Load(FT_Face face)
            {
                FT_ULong length = 0;
                if (FT_Load_Sfnt_Table(face, TTAG_GPOS, 0, nullptr, &length) != 0 || length < 10)
                    return false;
                mData.resize(length);
                if (FT_Load_Sfnt_Table(face, TTAG_GPOS, 0, mData.data(), &length) != 0)
                    return false;

                size_t featureList = U16(6), lookupList = U16(8); // After the version (4 bytes) and the script list
                std::vector<int> lookups;
                int featureCount = U16(featureList);
                for (int i = 0; i < featureCount; i++)
                {
                    size_t rec = featureList + 2 + i*6;
                    if (Tag(rec) != "kern")
                        continue;
                    size_t feature = featureList + U16(rec + 4);
                    int count = U16(feature + 2);
                    for (int j = 0; j < count; j++)
                    {
                        int index = U16(feature + 4 + j*2);
                        if (std::find(lookups.begin(), lookups.end(), index) == lookups.end())
                            lookups.push_back(index);
                    }
                }
                std::sort(lookups.begin(), lookups.end());

                int lookupCount = U16(lookupList);
                for (int index : lookups)
                {
                    if (index >= lookupCount)
                        continue;
                    size_t lookup = lookupList + U16(lookupList + 2 + index*2);
                    int type = U16(lookup);
                    int subCount = U16(lookup + 4);
                    std::vector<size_t> subs;
                    for (int k = 0; k < subCount; k++)
                    {
                        size_t sub = lookup + U16(lookup + 6 + k*2);
                        int subType = type;
                        if (type == 9)
                        {
                            subType = U16(sub + 2);
                            sub += U32(sub + 4);
                        }
                        if (subType == 2)
                            subs.push_back(sub);
                    }
                    if (!subs.empty())
                        mLookups.push_back(subs);
                }
                return !mLookups.empty();
            }

            // The x advance adjustment between two glyphs, font units
            int Kern(unsigned left, unsigned right)
            {
                UInt64 key = ((UInt64)left << 32) | right;
                auto it = mCache.find(key);
                if (it != mCache.end())
                    return it->second;

                int total = 0;
                for (auto& subs : mLookups)
                {
                    for (size_t sub : subs)
                    {
                        int value;
                        if (PairValue(sub, left, right, value))
                        {
                            total += value;
                            break;
                        }
                    }
                }
                return mCache[key] = total;
            }

        private:
            std::vector<UInt8>                mData;
            std::vector<std::vector<size_t>>  mLookups; // Pair adjustment subtables of each kern lookup
            std::unordered_map<UInt64, int>   mCache;

            int U16(size_t at) const { return at + 2 <= mData.size() ? (mData[at] << 8) | mData[at + 1] : 0; }
            int S16(size_t at) const { return (int16_t)U16(at); }
            size_t U32(size_t at) const { return ((size_t)U16(at) << 16) | (size_t)U16(at + 2); }
            std::string Tag(size_t at) const { return at + 4 <= mData.size() ? std::string((const char*)mData.data() + at, 4) : std::string(); }

            // The index of a glyph in a coverage table, -1 when it is not covered
            int Coverage(size_t table, unsigned glyph) const
            {
                int format = U16(table);
                if (format == 1)
                {
                    int lo = 0, hi = U16(table + 2) - 1;
                    while (lo <= hi)
                    {
                        int mid = (lo + hi)/2;
                        unsigned g = (unsigned)U16(table + 4 + mid*2);
                        if (g == glyph) return mid;
                        if (g < glyph) lo = mid + 1; else hi = mid - 1;
                    }
                    return -1;
                }
                if (format == 2)
                {
                    int count = U16(table + 2);
                    for (int i = 0; i < count; i++)
                    {
                        size_t rec = table + 4 + i*6;
                        unsigned start = (unsigned)U16(rec), end = (unsigned)U16(rec + 2);
                        if (glyph >= start && glyph <= end)
                            return U16(rec + 4) + (int)(glyph - start);
                    }
                }
                return -1;
            }

            // The class of a glyph in a class definition table (0 for none)
            int ClassOf(size_t table, unsigned glyph) const
            {
                int format = U16(table);
                if (format == 1)
                {
                    unsigned start = (unsigned)U16(table + 2);
                    int count = U16(table + 4);
                    return glyph >= start && glyph < start + count ? U16(table + 6 + (glyph - start)*2) : 0;
                }
                if (format == 2)
                {
                    int count = U16(table + 2);
                    for (int i = 0; i < count; i++)
                    {
                        size_t rec = table + 4 + i*6;
                        if (glyph >= (unsigned)U16(rec) && glyph <= (unsigned)U16(rec + 2))
                            return U16(rec + 4);
                    }
                }
                return 0;
            }

            static int ValueSize(int format)
            {
                int n = 0;
                for (int b = 0; b < 8; b++)
                    n += (format >> b) & 1;
                return n*2;
            }

            // XAdvance of a value record (0 when the record has none)
            int XAdvance(size_t record, int format) const
            {
                if (!(format & 0x0004))
                    return 0;
                size_t at = record + ((format & 1) ? 2 : 0) + ((format & 2) ? 2 : 0);
                return S16(at);
            }

            bool PairValue(size_t sub, unsigned left, unsigned right, int& value) const
            {
                int format = U16(sub);
                int cov = Coverage(sub + U16(sub + 2), left);
                if (cov < 0)
                    return false;
                int vf1 = U16(sub + 4), vf2 = U16(sub + 6);
                int s1 = ValueSize(vf1), s2 = ValueSize(vf2);
                if (format == 1)
                {
                    if (cov >= U16(sub + 8))
                        return false;
                    size_t set = sub + U16(sub + 10 + cov*2);
                    int count = U16(set);
                    int recSize = 2 + s1 + s2;
                    int lo = 0, hi = count - 1;
                    while (lo <= hi)
                    {
                        int mid = (lo + hi)/2;
                        size_t rec = set + 2 + (size_t)mid*recSize;
                        unsigned g = (unsigned)U16(rec);
                        if (g == right)
                        {
                            value = XAdvance(rec + 2, vf1);
                            return true;
                        }
                        if (g < right) lo = mid + 1; else hi = mid - 1;
                    }
                    return false;
                }
                if (format == 2)
                {
                    size_t classDef1 = sub + U16(sub + 8), classDef2 = sub + U16(sub + 10);
                    int class1Count = U16(sub + 12), class2Count = U16(sub + 14);
                    int c1 = ClassOf(classDef1, left), c2 = ClassOf(classDef2, right);
                    if (c1 >= class1Count || c2 >= class2Count)
                        return false;
                    size_t rec = sub + 16 + ((size_t)c1*class2Count + c2)*(s1 + s2);
                    value = XAdvance(rec, vf1);
                    return true;
                }
                return false;
            }
        };

        // A loaded font file
        struct Face
        {
            String                                 file;
            std::string                            bytes;   // FreeType reads the face from here
            FT_Face                                face = nullptr;
            double                                 upem = 1000;
            double                                 ascender = 0, descender = 0; // Font units, descender negative
            bool                                   kerning = false; // A legacy kern table (FT_Get_Kerning)
            GposKern                               gpos;
            bool                                   gposKerning = false;
            std::unordered_map<char32_t, unsigned> glyphOf;
            std::unordered_map<unsigned, double>   advanceOf; // Font units
        };

        // A rendered glyph: coverage with its offset from the pen (left) and the baseline (top, up)
        struct GlyphPic
        {
            int                left = 0, top = 0, w = 0, h = 0;
            std::vector<UInt8> cov;
        };

        // FreeType, the faces and the glyph cache behind one lock
        struct Engine
        {
            std::mutex                               lock;
            FT_Library                               lib = nullptr;
            FT_Stroker                               stroker = nullptr;
            bool                                     triedLib = false;
            std::vector<std::unique_ptr<Face>>       faces;
            std::map<std::string, int>               byFile;  // -1 = failed to load
            std::map<std::array<int, 5>, GlyphPic>   glyphs;  // face, size64, glyph, subpixel, stroke64

            ~Engine()
            {
                for (auto& f : faces)
                    FT_Done_Face(f->face);
                if (stroker)
                    FT_Stroker_Done(stroker);
                if (lib)
                    FT_Done_FreeType(lib);
            }

            bool Ready()
            {
                if (!triedLib)
                {
                    triedLib = true;
                    if (FT_Init_FreeType(&lib) != 0)
                        lib = nullptr;
                    else if (FT_Stroker_New(lib, &stroker) != 0)
                        stroker = nullptr;
                }
                return lib != nullptr;
            }

            int FaceIndex(const String& file)
            {
                auto it = byFile.find(file);
                if (it != byFile.end())
                    return it->second;
                if (!Ready())
                    return byFile[file] = -1;

                String home = UiFontsHome();
                auto f = std::make_unique<Face>();
                f->file = file;
                f->bytes = home.IsEmpty() ? std::string() : (std::string)PipelineUtils::ReadFileBytes(home + "/" + file);
                if (f->bytes.empty() ||
                    FT_New_Memory_Face(lib, (const FT_Byte*)f->bytes.data(), (FT_Long)f->bytes.size(), 0, &f->face) != 0)
                    return byFile[file] = -1;

                FT_Select_Charmap(f->face, FT_ENCODING_UNICODE);
                f->upem = f->face->units_per_EM ? f->face->units_per_EM : 1000;
                f->ascender = f->face->ascender;
                f->descender = f->face->descender;
                f->kerning = FT_HAS_KERNING(f->face);
                f->gposKerning = f->gpos.Load(f->face);
                faces.push_back(std::move(f));
                return byFile[file] = (int)faces.size() - 1;
            }

            unsigned GlyphOf(Face& f, char32_t c)
            {
                auto it = f.glyphOf.find(c);
                if (it != f.glyphOf.end())
                    return it->second;
                return f.glyphOf[c] = FT_Get_Char_Index(f.face, c);
            }

            double AdvanceOf(Face& f, unsigned glyph)
            {
                auto it = f.advanceOf.find(glyph);
                if (it != f.advanceOf.end())
                    return it->second;
                FT_Fixed adv = 0;
                FT_Get_Advance(f.face, glyph, FT_LOAD_NO_SCALE, &adv);
                return f.advanceOf[glyph] = (double)adv;
            }

            const GlyphPic& Render(int face, int size64, unsigned glyph, int sub, int stroke64)
            {
                std::array<int, 5> key = { face, size64, (int)glyph, sub, stroke64 };
                auto it = glyphs.find(key);
                if (it != glyphs.end())
                    return it->second;
                if (glyphs.size() > 20000)
                    glyphs.clear();

                GlyphPic& pic = glyphs[key];
                FT_Face f = faces[face]->face;
                if (FT_Set_Char_Size(f, 0, size64, 72, 72) != 0 || FT_Load_Glyph(f, glyph, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) != 0)
                    return pic;

                FT_Glyph g = nullptr;
                if (FT_Get_Glyph(f->glyph, &g) != 0)
                    return pic;

                // Quarter-pixel placement: x right, y down (FreeType's y goes up)
                FT_Vector delta = { (FT_Pos)((sub >> 2)*16), (FT_Pos)(-(sub & 3)*16) };
                FT_Glyph_Transform(g, nullptr, &delta);
                if (stroke64 > 0 && stroker)
                {
                    FT_Stroker_Set(stroker, stroke64/2, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
                    FT_Glyph_Stroke(&g, stroker, 1);
                }
                if (FT_Glyph_To_Bitmap(&g, FT_RENDER_MODE_NORMAL, nullptr, 1) == 0)
                {
                    FT_BitmapGlyph bg = (FT_BitmapGlyph)g;
                    pic.left = bg->left;
                    pic.top = bg->top;
                    pic.w = (int)bg->bitmap.width;
                    pic.h = (int)bg->bitmap.rows;
                    pic.cov.resize((size_t)pic.w*pic.h);
                    for (int y = 0; y < pic.h; y++)
                    {
                        const unsigned char* src = bg->bitmap.buffer + (ptrdiff_t)y*bg->bitmap.pitch;
                        std::copy(src, src + pic.w, pic.cov.begin() + (size_t)y*pic.w);
                    }
                }
                FT_Done_Glyph(g);
                return pic;
            }
        };

        Engine& TheEngine()
        {
            static Engine engine;
            return engine;
        }
    }

    UiShapedLine ShapeUiText(const String& fontFile, const String& text, double sizePx, double letterSpacingPx)
    {
        UiShapedLine line;
        Engine& e = TheEngine();
        std::lock_guard<std::mutex> guard(e.lock);
        int main = e.FaceIndex(fontFile);
        if (main < 0)
            return line;
        int fallback = fontFile == "Roboto-Regular.ttf" ? -1 : e.FaceIndex("Roboto-Regular.ttf");

        Face& f = *e.faces[main];
        line.ascent = f.ascender*sizePx/f.upem;
        line.descent = -f.descender*sizePx/f.upem;

        double pen = 0;
        int prevFace = -1;
        unsigned prev = 0;
        for (char32_t c : Utf8ToCodePoints(text))
        {
            int face = main;
            unsigned glyph = e.GlyphOf(f, c);
            if (glyph == 0 && fallback >= 0 && e.GlyphOf(*e.faces[fallback], c) != 0)
            {
                face = fallback;
                glyph = e.GlyphOf(*e.faces[fallback], c);
            }
            Face& gf = *e.faces[face];
            double k = sizePx/gf.upem;
            if (prevFace == face && prev)
            {
                if (gf.gposKerning)
                    pen += gf.gpos.Kern(prev, glyph)*k;
                else if (gf.kerning)
                {
                    FT_Vector kern = { 0, 0 };
                    if (FT_Get_Kerning(gf.face, prev, glyph, FT_KERNING_UNSCALED, &kern) == 0)
                        pen += kern.x*k;
                }
            }
            line.glyphs.push_back(UiPlacedGlyph{ face, glyph, pen });
            pen += e.AdvanceOf(gf, glyph)*k + letterSpacingPx;
            prevFace = face;
            prev = glyph;
        }
        line.width = pen;
        return line;
    }

    void DrawUiText(const UiShapedLine& line, double x, double baseline, double sizePx, double strokeWidthPx, UiMask& mask)
    {
        Engine& e = TheEngine();
        std::lock_guard<std::mutex> guard(e.lock);
        int size64 = std::max(1, (int)std::lround(sizePx*64));
        int stroke64 = strokeWidthPx > 0 ? std::max(1, (int)std::lround(strokeWidthPx*64)) : 0;

        double fy = std::floor(baseline);
        int oy = (int)fy;
        int qy = (int)std::lround((baseline - fy)*4);
        if (qy == 4)
        {
            oy++;
            qy = 0;
        }

        for (auto& g : line.glyphs)
        {
            if (g.face < 0 || g.face >= (int)e.faces.size())
                continue;
            double pen = x + g.x;
            double fx = std::floor(pen);
            int ox = (int)fx;
            int qx = (int)std::lround((pen - fx)*4);
            if (qx == 4)
            {
                ox++;
                qx = 0;
            }
            const GlyphPic& pic = e.Render(g.face, size64, g.glyph, qx*4 + qy, stroke64);
            if (pic.w > 0 && pic.h > 0)
                mask.Blit(pic.cov.data(), pic.w, pic.w, pic.h, ox + pic.left, oy - pic.top);
        }
    }

    bool HasUiFont(const String& fontFile)
    {
        Engine& e = TheEngine();
        std::lock_guard<std::mutex> guard(e.lock);
        return e.FaceIndex(fontFile) >= 0;
    }

    void ClearUiGlyphCache()
    {
        Engine& e = TheEngine();
        std::lock_guard<std::mutex> guard(e.lock);
        e.glyphs.clear();
    }
}
