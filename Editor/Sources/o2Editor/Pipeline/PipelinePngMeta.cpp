#include "o2Editor/stdafx.h"
#include "PipelinePngMeta.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"

namespace Editor
{
    namespace PipelinePngMeta
    {
        const char* dpiKey = "AssetsLine:dpi";
        const char* scaleKey = "AssetsLine:scale";
        const char* sourceKey = "AssetsLine:source";

        static const unsigned char signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };

        struct Chunk
        {
            String type;
            String data;
        };

        static UInt32 Crc32(const String& bytes)
        {
            static UInt32 table[256];
            static bool ready = false;
            if (!ready)
            {
                for (UInt32 n = 0; n < 256; n++)
                {
                    UInt32 c = n;
                    for (int k = 0; k < 8; k++)
                        c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
                    table[n] = c;
                }
                ready = true;
            }

            UInt32 c = 0xffffffffu;
            for (int i = 0; i < bytes.Length(); i++)
                c = table[(c ^ (unsigned char)bytes[i]) & 0xff] ^ (c >> 8);

            return c ^ 0xffffffffu;
        }

        static UInt32 ReadU32(const String& b, int p)
        {
            return ((UInt32)(unsigned char)b[p] << 24) | ((UInt32)(unsigned char)b[p + 1] << 16) |
                ((UInt32)(unsigned char)b[p + 2] << 8) | (UInt32)(unsigned char)b[p + 3];
        }

        static void WriteU32(String& out, UInt32 v)
        {
            out += (char)((v >> 24) & 0xff);
            out += (char)((v >> 16) & 0xff);
            out += (char)((v >> 8) & 0xff);
            out += (char)(v & 0xff);
        }

        bool IsPng(const String& bytes)
        {
            if (bytes.Length() <= 33)
                return false;

            for (int i = 0; i < 8; i++)
            {
                if ((unsigned char)bytes[i] != signature[i])
                    return false;
            }
            return true;
        }

        static bool ChunksOf(const String& b, Vector<Chunk>& out)
        {
            if (!IsPng(b))
                return false;

            int p = 8;
            while (p + 12 <= b.Length())
            {
                UInt32 len = ReadU32(b, p);
                String type = b.SubStr(p + 4, p + 8);
                if ((UInt64)p + 12 + len > (UInt64)b.Length())
                    return false;

                out.Add({ type, b.SubStr(p + 8, p + 8 + (int)len) });
                p += 12 + (int)len;
                if (type == "IEND")
                    break;
            }
            return true;
        }

        static String Encode(const Vector<Chunk>& chunks)
        {
            String out;
            for (int i = 0; i < 8; i++)
                out += (char)signature[i];

            for (auto& c : chunks)
            {
                WriteU32(out, (UInt32)c.data.Length());
                String head = c.type + c.data;
                out += head;
                WriteU32(out, Crc32(head));
            }
            return out;
        }

        static bool TextOf(const Chunk& c, String& key, String& value)
        {
            if (c.type != "tEXt")
                return false;

            int z = c.data.Find('\0');
            if (z < 0)
                return false;

            key = c.data.SubStr(0, z);
            value = c.data.SubStr(z + 1);
            return true;
        }

        Map<String, String> ReadText(const String& png)
        {
            Map<String, String> out;
            Vector<Chunk> chunks;
            if (!ChunksOf(png, chunks))
                return out;

            for (auto& c : chunks)
            {
                if (c.type == "IDAT")
                    break;

                String key, value;
                if (TextOf(c, key, value) && key.StartsWith("AssetsLine:"))
                    out[key] = value;
            }
            return out;
        }

        String WriteText(const String& png, const Vector<Pair<String, String>>& entries, std::optional<double> dpi)
        {
            Vector<Chunk> chunks;
            if (!ChunksOf(png, chunks) || chunks.IsEmpty() || chunks[0].type != "IHDR")
                return png;

            bool withDpi = dpi && *dpi != 0.0;
            Vector<Chunk> kept;
            for (auto& c : chunks)
            {
                if (withDpi && c.type == "pHYs")
                    continue;

                String key, value;
                if (TextOf(c, key, value) && entries.Any([&](const Pair<String, String>& e) { return e.first == key; }))
                    continue;

                kept.Add(c);
            }

            Vector<Chunk> added;
            if (withDpi)
            {
                UInt32 ppm = (UInt32)UiDoc::JsRound(*dpi / 0.0254);
                String d;
                WriteU32(d, ppm);
                WriteU32(d, ppm);
                d += (char)1;
                added.Add({ "pHYs", d });
            }

            for (auto& e : entries)
            {
                if (e.second.IsEmpty())
                    continue;

                added.Add({ "tEXt", e.first + String('\0') + e.second });
            }

            Vector<Chunk> result = { kept[0] };
            result.Add(added);
            for (int i = 1; i < kept.Count(); i++)
                result.Add(kept[i]);

            return Encode(result);
        }

        static std::optional<double> PositiveNumber(const Map<String, String>& text, const char* key)
        {
            String value;
            if (!text.TryGetValue(key, value))
                return std::nullopt;

            UiLayout::Json json(value);
            double v = UiDoc::JsNumber(&json);
            if (!std::isfinite(v) || v <= 0.0)
                return std::nullopt;

            return v;
        }

        std::optional<double> ReadDpi(const String& png)
        {
            return PositiveNumber(ReadText(png), dpiKey);
        }

        std::optional<double> ReadScale(const String& png)
        {
            return PositiveNumber(ReadText(png), scaleKey);
        }

        bool ReadSource(const String& png, int& width, int& height)
        {
            auto text = ReadText(png);
            auto it = text.find(sourceKey);
            if (it == text.end())
                return false;

            const std::string& s = it->second;
            size_t x = s.find('x');
            if (x == std::string::npos || x == 0 || x + 1 >= s.size())
                return false;
            for (size_t i = 0; i < s.size(); i++)
            {
                if (i != x && !(s[i] >= '0' && s[i] <= '9'))
                    return false;
            }
            double w = std::stod(s.substr(0, x)), h = std::stod(s.substr(x + 1));
            if (!(w > 0 && h > 0) || w > 1e9 || h > 1e9)
                return false;
            width = (int)w;
            height = (int)h;
            return true;
        }

        String WithDpi(const String& png, std::optional<double> dpi, std::optional<double> scale, int sourceW, int sourceH)
        {
            Vector<Pair<String, String>> entries;
            bool withDpi = dpi && *dpi != 0.0;
            entries.Add(Pair<String, String>(dpiKey, withDpi ? UiDoc::JsNum(UiDoc::JsRound(*dpi * 10.0) / 10.0) : String()));
            if (scale)
                entries.Add(Pair<String, String>(scaleKey, UiDoc::JsNum(UiDoc::JsRound(*scale * 10000.0) / 10000.0)));
            if (sourceW > 0 && sourceH > 0)
                entries.Add(Pair<String, String>(sourceKey, UiDoc::JsNum(sourceW) + "x" + UiDoc::JsNum(sourceH)));

            return WriteText(png, entries, withDpi ? dpi : std::nullopt);
        }

        bool Size(const String& png, int& width, int& height)
        {
            if (!IsPng(png))
                return false;

            width = (int)ReadU32(png, 16);
            height = (int)ReadU32(png, 20);
            return true;
        }
    }
}
