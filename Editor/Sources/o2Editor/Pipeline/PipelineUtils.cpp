#include "o2Editor/stdafx.h"
#include "PipelineUtils.h"

#include "o2/EngineSettings.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Types/UID.h"

#include <cstring>

namespace Editor::PipelineUtils
{
    static const char* base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    String Base64Encode(const String& bytes)
    {
        const unsigned char* data = (const unsigned char*)bytes.Data();
        int len = bytes.Length();
        String out;
        out.Reserve((len + 2) / 3 * 4 + 1);

        int i = 0;
        while (i + 2 < len)
        {
            unsigned v = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
            out += base64Chars[(v >> 18) & 63];
            out += base64Chars[(v >> 12) & 63];
            out += base64Chars[(v >> 6) & 63];
            out += base64Chars[v & 63];
            i += 3;
        }

        if (i < len)
        {
            unsigned v = data[i] << 16;
            if (i + 1 < len) v |= data[i + 1] << 8;
            out += base64Chars[(v >> 18) & 63];
            out += base64Chars[(v >> 12) & 63];
            out += (i + 1 < len) ? base64Chars[(v >> 6) & 63] : '=';
            out += '=';
        }

        return out;
    }

    String Base64Decode(const String& text)
    {
        static int table[256];
        static bool tableReady = false;
        if (!tableReady)
        {
            for (int i = 0; i < 256; i++) table[i] = -1;
            for (int i = 0; i < 64; i++) table[(unsigned char)base64Chars[i]] = i;
            table[(unsigned char)'-'] = 62;
            table[(unsigned char)'_'] = 63;
            tableReady = true;
        }

        String out;
        out.Reserve(text.Length() * 3 / 4 + 1);
        unsigned acc = 0;
        int bits = 0;
        for (int i = 0; i < text.Length(); i++)
        {
            unsigned char c = (unsigned char)text[i];
            if (c == '=') break;
            int v = table[c];
            if (v < 0) continue;
            acc = (acc << 6) | v;
            bits += 6;
            if (bits >= 8)
            {
                bits -= 8;
                out += (char)((acc >> bits) & 0xff);
            }
        }

        return out;
    }

    UInt64 Fnv1a64(const String& data)
    {
        UInt64 h = 14695981039346656037ull;
        for (int i = 0; i < data.Length(); i++)
        {
            h ^= (unsigned char)data[i];
            h *= 1099511628211ull;
        }
        return h;
    }

    String Fnv1a64Hex(const String& data)
    {
        UInt64 h = Fnv1a64(data);
        // Second independent hash widens the key so two configs never share a cache entry by accident
        UInt64 h2 = Fnv1a64(data + "|salt");
        char buf[40];
        snprintf(buf, sizeof(buf), "%016llx%016llx", (unsigned long long)h, (unsigned long long)h2);
        return buf;
    }

    namespace
    {
        inline UInt32 Rotr(UInt32 x, int n) { return (x >> n) | (x << (32 - n)); }

        const UInt32 sha256K[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };
    }

    String Sha256(const String& input)
    {
        UInt32 h[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };

        Vector<unsigned char> msg;
        msg.Reserve(input.Length() + 72);
        for (int i = 0; i < input.Length(); i++)
            msg.Add((unsigned char)input[i]);

        UInt64 bitLen = (UInt64)input.Length() * 8;
        msg.Add(0x80);
        while (msg.Count() % 64 != 56)
            msg.Add(0);
        for (int i = 7; i >= 0; i--)
            msg.Add((unsigned char)((bitLen >> (i * 8)) & 0xff));

        for (int chunk = 0; chunk < msg.Count(); chunk += 64)
        {
            UInt32 w[64];
            for (int i = 0; i < 16; i++)
            {
                w[i] = ((UInt32)msg[chunk + i * 4] << 24) | ((UInt32)msg[chunk + i * 4 + 1] << 16) |
                       ((UInt32)msg[chunk + i * 4 + 2] << 8) | (UInt32)msg[chunk + i * 4 + 3];
            }
            for (int i = 16; i < 64; i++)
            {
                UInt32 s0 = Rotr(w[i - 15], 7) ^ Rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
                UInt32 s1 = Rotr(w[i - 2], 17) ^ Rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }

            UInt32 a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
            for (int i = 0; i < 64; i++)
            {
                UInt32 S1 = Rotr(e, 6) ^ Rotr(e, 11) ^ Rotr(e, 25);
                UInt32 ch = (e & f) ^ (~e & g);
                UInt32 t1 = hh + S1 + ch + sha256K[i] + w[i];
                UInt32 S0 = Rotr(a, 2) ^ Rotr(a, 13) ^ Rotr(a, 22);
                UInt32 maj = (a & b) ^ (a & c) ^ (b & c);
                UInt32 t2 = S0 + maj;
                hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
            }

            h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        }

        String out;
        for (int i = 0; i < 8; i++)
        {
            out += (char)((h[i] >> 24) & 0xff);
            out += (char)((h[i] >> 16) & 0xff);
            out += (char)((h[i] >> 8) & 0xff);
            out += (char)(h[i] & 0xff);
        }
        return out;
    }

    String HmacSha256(const String& key, const String& data)
    {
        String k = key.Length() > 64 ? Sha256(key) : key;
        while (k.Length() < 64)
            k += (char)0;

        String ipad, opad;
        for (int i = 0; i < 64; i++)
        {
            ipad += (char)(((unsigned char)k[i]) ^ 0x36);
            opad += (char)(((unsigned char)k[i]) ^ 0x5c);
        }

        return Sha256(opad + Sha256(ipad + data));
    }

    static void EscapeJsonString(const char* str, int length, String& out)
    {
        out += '"';
        for (int i = 0; i < length; i++)
        {
            char c = str[i];
            switch (c)
            {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if ((unsigned char)c < 0x20)
                    {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                        out += buf;
                    }
                    else
                        out += c;
            }
        }
        out += '"';
    }

    static void WriteCanonical(const DataValue& value, String& out, const Vector<String>* exclude)
    {
        if (value.IsObject())
        {
            Vector<Pair<String, const DataValue*>> members;
            for (auto it = value.BeginMember(); it != value.EndMember(); ++it)
            {
                String name = it->name.GetString();
                if (exclude && exclude->Contains(name))
                    continue;

                members.Add({ name, &it->value });
            }
            members.Sort([](const Pair<String, const DataValue*>& a, const Pair<String, const DataValue*>& b) { return std::strcmp(a.first.Data(), b.first.Data()) < 0; });

            out += '{';
            bool first = true;
            for (auto& m : members)
            {
                if (!first) out += ',';
                first = false;
                EscapeJsonString(m.first.Data(), m.first.Length(), out);
                out += ':';
                WriteCanonical(*m.second, out, nullptr);
            }
            out += '}';
        }
        else if (value.IsArray())
        {
            out += '[';
            bool first = true;
            for (auto& item : value)
            {
                if (!first) out += ',';
                first = false;
                WriteCanonical(item, out, nullptr);
            }
            out += ']';
        }
        else if (value.IsString())
            EscapeJsonString(value.GetString(), value.GetStringLength(), out);
        else if (value.IsBoolean())
            out += (bool)value ? "true" : "false";
        else if (value.IsNumber())
        {
            double d = (double)value;
            char buf[64];
            if (d == Math::Floor(d) && Math::Abs(d) < 1e15)
                snprintf(buf, sizeof(buf), "%lld", (long long)d);
            else
                snprintf(buf, sizeof(buf), "%.6g", d);
            out += buf;
        }
        else
            out += "null";
    }

    String CanonicalJson(const DataValue& value, const Vector<String>& excludeTopLevelKeys /*= {}*/)
    {
        String out;
        WriteCanonical(value, out, &excludeTopLevelKeys);
        return out;
    }

    String ValueToString(const DataValue& value, const String& def /*= ""*/)
    {
        if (value.IsString())
            return value.GetString();

        if (value.IsBoolean())
            return (bool)value ? "true" : "false";

        if (value.IsNumber())
        {
            double d = (double)value;
            if (d == Math::Floor(d) && Math::Abs(d) < 1e15)
                return (String)(int)d;

            return (String)(float)d;
        }

        return def;
    }

    float ValueToNumber(const DataValue& value, float def /*= 0.0f*/)
    {
        if (value.IsNumber())
            return (float)(double)value;

        if (value.IsBoolean())
            return (bool)value ? 1.0f : 0.0f;

        if (value.IsString())
        {
            String s = String(value.GetString()).Trimed();
            if (s.IsEmpty())
                return def;

            char* end = nullptr;
            double d = strtod(s.Data(), &end);
            if (end == s.Data())
                return def;

            return (float)d;
        }

        return def;
    }

    String ReadFileBytes(const String& path)
    {
        InFile file(path);
        if (!file.IsOpened())
            return "";

        UInt size = file.GetDataSize();
        std::string buffer(size, '\0');
        if (size > 0)
            file.ReadData(&buffer[0], size);

        return String(buffer);
    }

    bool WriteFileBytes(const String& path, const String& data)
    {
        String folder = o2FileSystem.ExtractPathStr(path);
        if (!folder.IsEmpty() && !o2FileSystem.IsFolderExist(folder))
            o2FileSystem.FolderCreate(folder, true);

        OutFile file(path);
        if (!file.IsOpened())
            return false;

        file.WriteData(data.Data(), (UInt)data.Length());
        return true;
    }

    String FileSignature(const String& path)
    {
        if (!o2FileSystem.IsFileExist(path))
            return "missing";

        auto info = o2FileSystem.GetFileInfo(path);
        return (String)(int)info.size + ":" + (String)(int)info.editDate.mYear + "-" + (String)(int)info.editDate.mMonth + "-" +
            (String)(int)info.editDate.mDay + "T" + (String)(int)info.editDate.mHour + ":" + (String)(int)info.editDate.mMinute + ":" +
            (String)(int)info.editDate.mSecond;
    }

    String DataUrlToBytes(const String& dataUrl)
    {
        String s = dataUrl.Trimed();
        if (!s.StartsWith("data:"))
            return "";

        int comma = s.Find(",");
        if (comma < 0)
            return "";

        String header = s.SubStr(0, comma);
        if (!header.Contains("base64"))
            return "";

        return Base64Decode(s.SubStr(comma + 1));
    }

    String BytesToDataUrl(const String& bytes, const String& mime)
    {
        return "data:" + mime + ";base64," + Base64Encode(bytes);
    }

    String MimeForExtension(const String& extIn)
    {
        String ext = extIn.ToLowerCase();
        if (ext.StartsWith("."))
            ext = ext.SubStr(1);

        if (ext == "png") return "image/png";
        if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
        if (ext == "webp") return "image/webp";
        if (ext == "gif") return "image/gif";
        if (ext == "bmp") return "image/bmp";
        if (ext == "mp4") return "video/mp4";
        if (ext == "webm") return "video/webm";
        if (ext == "mov") return "video/quicktime";
        if (ext == "mp3") return "audio/mpeg";
        if (ext == "wav") return "audio/wav";
        if (ext == "ogg") return "audio/ogg";
        if (ext == "m4a") return "audio/mp4";
        if (ext == "flac") return "audio/flac";
        if (ext == "txt") return "text/plain";
        if (ext == "json") return "application/json";
        return "application/octet-stream";
    }

    String PrettyModelName(const String& idIn)
    {
        // The same names AssetsLine shows, so a pipeline reads the same in both editors
        static const Map<String, String> known = {
            { "gemini-3.1-flash-image", "Gemini 3.1 Flash Image \xC2\xB7 Nano Banana 2" },
            { "gemini-3-pro-image", "Gemini 3 Pro Image \xC2\xB7 Nano Banana Pro" },
            { "gemini-3-pro-image-preview", "Gemini 3 Pro Image \xC2\xB7 preview" },
            { "gemini-2.5-flash-image", "Gemini 2.5 Flash Image \xC2\xB7 Nano Banana" },
            { "gemini-2.5-flash-image-preview", "Gemini 2.5 Flash Image \xC2\xB7 preview" },
            { "imagen-4.0-generate-001", "Imagen 4.0" },
            { "imagen-3.0-generate-001", "Imagen 3.0" },
            { "gemini-pro-latest", "Gemini Pro \xC2\xB7 latest" },
            { "gemini-flash-latest", "Gemini Flash \xC2\xB7 latest" },
            { "veo-3.1-generate-preview", "Google Veo 3.1" },
            { "veo-3.1-fast-generate-preview", "Google Veo 3.1 Fast" },
            { "veo-3.0-generate-001", "Google Veo 3" },
            { "veo-3.0-fast-generate-001", "Google Veo 3 Fast" },
            { "veo-2.0-generate-001", "Google Veo 2" },
            { "kling-v2-5-turbo", "Kling 2.5 Turbo" },
            { "kling-v2-1-master", "Kling 2.1 Master" },
            { "kling-v2-1", "Kling 2.1" },
            { "kling-v2-master", "Kling 2.0 Master" },
            { "kling-v1-6", "Kling 1.6 \xC2\xB7 multi-ref" },
            { "lyria-3-clip-preview", "Lyria 3 \xC2\xB7 30 s clip" },
            { "lyria-3-pro-preview", "Lyria 3 Pro \xC2\xB7 full track" },
            { "gemini-2.5-flash-preview-tts", "Gemini 2.5 Flash TTS" },
            { "gemini-2.5-pro-preview-tts", "Gemini 2.5 Pro TTS" },
            { "gemini-3.1-flash-tts-preview", "Gemini 3.1 Flash TTS" },
            { "eleven_text_to_sound_v2", "ElevenLabs SFX v2" },
            { "eleven_multilingual_v2", "ElevenLabs Multilingual v2" },
            { "eleven_flash_v2_5", "ElevenLabs Flash v2.5" },
            { "eleven_turbo_v2_5", "ElevenLabs Turbo v2.5" },
            { "eleven_v3", "ElevenLabs v3" }
        };

        String id = idIn.StartsWith("models/") ? idIn.SubStr(7) : idIn;
        if (id.IsEmpty())
            return id;

        String name;
        if (known.TryGetValue(id, name))
            return name;

        // Unknown ids: words capitalised, versions as they are, qualifiers after a dot
        String result;
        String word;
        auto flush = [&]()
        {
            if (word.IsEmpty())
                return;

            String part = word;
            if (part == "latest" || part == "preview" || part == "exp")
                part = String("\xC2\xB7 ") + part;
            else if (part == "lite")
                part = "Lite";
            else if (!(part[0] >= '0' && part[0] <= '9'))
            {
                String first;
                first += (char)toupper((unsigned char)part[0]);
                part = first + part.SubStr(1);
            }

            if (!result.IsEmpty())
                result += " ";
            result += part;
            word = "";
        };

        for (int i = 0; i < id.Length(); i++)
        {
            char c = id[i];
            if (c == '-' || c == '_')
                flush();
            else
                word += c;
        }
        flush();
        return result;
    }

    String ExtensionForMime(const String& mimeIn)
    {
        String mime = mimeIn.ToLowerCase();
        int semicolon = mime.Find(";");
        if (semicolon >= 0)
            mime = mime.SubStr(0, semicolon);

        if (mime == "image/png") return "png";
        if (mime == "image/jpeg" || mime == "image/jpg") return "jpg";
        if (mime == "image/webp") return "webp";
        if (mime == "image/gif") return "gif";
        if (mime == "image/bmp") return "bmp";
        if (mime == "video/mp4") return "mp4";
        if (mime == "video/webm") return "webm";
        if (mime == "video/quicktime") return "mov";
        if (mime == "audio/mpeg" || mime == "audio/mp3") return "mp3";
        if (mime == "audio/wav" || mime == "audio/wave" || mime == "audio/x-wav") return "wav";
        if (mime == "audio/ogg" || mime == "audio/opus") return "ogg";
        if (mime == "audio/mp4" || mime == "audio/m4a" || mime == "audio/x-m4a") return "m4a";
        if (mime == "audio/flac" || mime == "audio/x-flac") return "flac";
        if (mime == "text/plain") return "txt";
        return "bin";
    }

    static String workPathOverride;

    String GetWorkPath()
    {
        if (!workPathOverride.IsEmpty())
            return workPathOverride;

        return String(GetProjectRootPath()) + "Work/Pipelines/";
    }

    void SetWorkPathOverride(const String& path)
    {
        workPathOverride = path;
        if (!workPathOverride.IsEmpty() && !workPathOverride.EndsWith("/"))
            workPathOverride += "/";
    }

    String GetUploadsPath()
    {
        return GetWorkPath() + "uploads/";
    }

    String GetUploadPath(const String& uploadId)
    {
        if (uploadId.IsEmpty())
            return "";

        return GetUploadsPath() + uploadId;
    }

    String StoreUpload(const String& sourceFile)
    {
        if (!o2FileSystem.IsFileExist(sourceFile))
            return "";

        String ext = o2FileSystem.GetFileExtension(sourceFile).ToLowerCase();
        UID uid;
        uid.Randomize();
        String id = (String)uid;
        if (!ext.IsEmpty())
            id += "." + ext;

        o2FileSystem.FolderCreate(GetUploadsPath(), true);
        if (!o2FileSystem.FileCopy(sourceFile, GetUploadsPath() + id))
            return "";

        return id;
    }

    String ClampPromptChars(const String& textIn, int maxChars)
    {
        String t = textIn.Trimed(" \n\r\t");
        if (t.Length() <= maxChars)
            return t;

        String head = t.SubStr(0, maxChars);

        auto lastOf = [&](const char* symbols) -> int
        {
            int best = -1;
            for (int i = 0; i < head.Length(); i++)
            {
                if (std::strchr(symbols, head[i]))
                    best = i;
            }
            return best;
        };

        int sentence = lastOf(".!?");
        if (sentence >= maxChars / 2)
            return head.SubStr(0, sentence + 1).Trimed(" \n\r\t");

        int clause = lastOf(",;:");
        if (clause >= maxChars / 2)
            return head.SubStr(0, clause).Trimed(" \n\r\t");

        int space = lastOf(" \n\t");
        if (space >= maxChars / 2)
            return head.SubStr(0, space).Trimed(" \n\r\t");

        return head.Trimed(" \n\r\t");
    }

    bool ParseHexColor(const String& textIn, Color4& color)
    {
        String s = textIn.Trimed();
        if (s.StartsWith("#"))
            s = s.SubStr(1);

        if (s.Length() != 3 && s.Length() != 6)
            return false;

        for (int i = 0; i < s.Length(); i++)
        {
            if (!isxdigit((unsigned char)s[i]))
                return false;
        }

        if (s.Length() == 3)
        {
            String expanded;
            for (int i = 0; i < 3; i++) { expanded += s[i]; expanded += s[i]; }
            s = expanded;
        }

        int r = (int)strtol(s.SubStr(0, 2).Data(), nullptr, 16);
        int g = (int)strtol(s.SubStr(2, 4).Data(), nullptr, 16);
        int b = (int)strtol(s.SubStr(4, 6).Data(), nullptr, 16);
        color = Color4(r, g, b, 255);
        return true;
    }

    String ColorToHex(const Color4& color)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "#%02x%02x%02x", Math::Clamp(color.r, 0, 255), Math::Clamp(color.g, 0, 255), Math::Clamp(color.b, 0, 255));
        return buf;
    }

    String ColorName(const Color4& c)
    {
        int r = c.r, g = c.g, b = c.b;
        int max = Math::Max(r, Math::Max(g, b));
        int min = Math::Min(r, Math::Min(g, b));
        if (max - min < 30) return max > 200 ? "white" : max < 60 ? "black" : "grey";
        if (g == max && g - Math::Max(r, b) > 30) return "green";
        if (b == max && b - Math::Max(r, g) > 30) return r > 120 ? "purple" : "blue";
        if (r == max && r - Math::Max(g, b) > 30) return g > 120 ? "orange" : b > 120 ? "magenta" : "red";
        if (r == max && g == max) return "yellow";
        if (g == max && b == max) return "cyan";
        if (r == max && b == max) return "magenta";
        return "the key colour";
    }

    String Trim(const String& text)
    {
        return text.Trimed(" \n\r\t");
    }

    String UrlEncode(const String& text)
    {
        String out;
        for (int i = 0; i < text.Length(); i++)
        {
            unsigned char c = (unsigned char)text[i];
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
                out += (char)c;
            else
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "%%%02X", c);
                out += buf;
            }
        }
        return out;
    }
}
