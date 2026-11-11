#include "o2Editor/stdafx.h"
#include "UiUnityInternal.h"

#include <cmath>

#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc::Unity
{
    // --- deterministic ids --------------------------------------------------------------------------------

    UInt64 Fnv64(const String& text)
    {
        UInt64 h = 0xcbf29ce484222325ull;
        for (int i = 0; i < text.Length(); i++)
        {
            h ^= (UInt64)(unsigned char)text[i];
            h *= 0x100000001b3ull;
        }
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdull;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53ull;
        h ^= h >> 33;
        return h;
    }

    static String Hex16(UInt64 v)
    {
        char buffer[17];
        snprintf(buffer, sizeof(buffer), "%016llx", (unsigned long long)v);
        return String(buffer);
    }

    String Hash24(const String& key)
    {
        return (Hex16(Fnv64("h-a|" + key)) + Hex16(Fnv64("h-b|" + key))).SubStr(0, 24).ToUpperCase();
    }

    String Ids::Next(const String& key)
    {
        for (int i = 0; ; i++)
        {
            UInt64 v = Fnv64(mSeed + "|" + key + (i ? "#" + String(i) : String())) & 0x7fffffffffffffffull;
            if (v < 1000000)
                v += 1000000;
            std::string s = std::to_string(v);
            if (mUsed.insert(s).second)
                return String(s.c_str());
        }
    }

    // --- assets (UiPrefabAssets) ----------------------------------------------------------------------------

    bool FontMetrics(const String& file, double& lineHeight, double& fontSize)
    {
        if (file != "Roboto-Regular.ttf" && file != "Roboto-Medium.ttf" && file != "Roboto-Bold.ttf")
            return false;
        lineHeight = 18;
        fontSize = 16;
        return true;
    }

    UnityAssets::UnityAssets(const String& spritesFolder, const String& seed, std::set<std::string>& warnings,
                             std::map<std::string, String>& pngs):
        warnings(warnings), pngs(pngs), spritesFolder(spritesFolder), seed(seed)
    {
        defaultFont.fileID = "10102";
        defaultFont.guid = "0000000000000000e000000000000000";
        defaultFont.type = 0;
    }

    const FontInfo& UnityAssets::Font(const String& family) const
    {
        auto it = fonts.find(family.Data());
        return it != fonts.end() ? it->second : defaultFont;
    }

    static String Values4(const V4& b)
    {
        return UnityFloat(b.x) + "," + UnityFloat(b.y) + "," + UnityFloat(b.z) + "," + UnityFloat(b.w);
    }

    SpriteInfo UnityAssets::SpriteFor(const String& key, const Json* slice, double hue)
    {
        auto it = sprites.find(JsToLower(key).Data());
        if (it == sprites.end())
            throw ExportError(("sprite not found: " + key).Data());

        SpriteInfo source = it->second;
        if (!Approximately(std::fmod(hue, 360.0), 0))
            source = BakeHue(source, Repeat(F(hue), 360));
        if (!HasSlice(slice))
            return source;

        V4 border;
        if (slice->IsBool() || slice->IsString())
        {
            border = source.border;
            if (IsZero4(border))
            {
                double b = F(std::max(4.0, std::min(60.0, std::floor(F(std::min(source.w, source.h) / 4)))));
                border = V4{ b, b, b, b };
            }
        }
        else if (slice->IsObject())
            border = V4{ F(NumOf(slice, "left")), F(NumOf(slice, "bottom")), F(NumOf(slice, "right")), F(NumOf(slice, "top")) };
        else
        {
            double b = F(slice->ToJsNumber());
            border = V4{ b, b, b, b };
        }

        border.x = std::min(border.x, F(source.w - 1));
        border.z = std::min(border.z, F(F(source.w - 1) - border.x));
        border.y = std::min(border.y, F(source.h - 1));
        border.w = std::min(border.w, F(F(source.h - 1) - border.y));
        if (Eq4(source.border, border))
            return source;
        return SpriteAsset(source, border);
    }

    SpriteInfo UnityAssets::SpriteAsset(const SpriteInfo& source, const V4& border)
    {
        String signature = "slice|" + source.file + "|" + JsNum(source.w) + "x" + JsNum(source.h) + "|" + Values4(border);
        auto found = variants.find(signature.Data());
        if (found != variants.end())
            return found->second;

        String name = "sprite_" + Hash24(seed + "|" + signature);
        String path = spritesFolder + "/" + name + ".asset";
        String guid = UnityGuid(seed + "|" + path);
        files.Add({ path, SpriteAssetYaml(name, source, border) });
        files.Add({ path + ".meta", NativeMeta(guid, "21300000") });
        generated++;

        SpriteInfo info = source;
        info.fileID = "21300000";
        info.guid = guid;
        info.type = 2;
        info.border = border;
        info.file = name + ".asset";
        variants[signature.Data()] = info;
        return info;
    }

    SpriteInfo UnityAssets::BakeHue(const SpriteInfo& source, double degrees)
    {
        String signature = "hue|" + source.file + "|" + UnityFloat(degrees);
        auto found = variants.find(signature.Data());
        if (found != variants.end())
            return found->second;

        auto png = pngs.find(source.texture.Data());
        UiLayout::UiImage img;
        if (png == pngs.end() || !UiLayout::UiImage::Decode(png->second, img))
        {
            warnings.insert((source.file + ": hueShift not baked (not a PNG); the sprite keeps its own colours.").Data());
            return source;
        }
        for (size_t i = 0; i + 3 < img.data.size(); i += 4)
        {
            int rgb[3];
            HueShiftPixel(img.data[i], img.data[i + 1], img.data[i + 2], degrees, rgb);
            img.data[i] = (UInt8)rgb[0];
            img.data[i + 1] = (UInt8)rgb[1];
            img.data[i + 2] = (UInt8)rgb[2];
        }
        String bakedPng = img.Png();

        String name = "hue_" + Hash24(seed + "|" + signature) + ".png";
        String path = spritesFolder + "/" + name;
        String guid = UnityGuid(seed + "|" + path);
        files.Add({ path, bakedPng });
        files.Add({ path + ".meta", TextureMeta(guid, source.border) });
        pngs[path.Data()] = bakedPng;
        baked++;

        SpriteInfo info = source;
        info.guid = guid;
        info.fileID = "21300000";
        info.type = 3;
        info.file = name;
        info.texture = path;
        variants[signature.Data()] = info;
        return info;
    }

    // Unity's Color.RGBToHSV + HSVToRGB over one pixel (Color32 in, Color32 out), with Mono's arithmetic: an expression
    // in double, rounded to float where the C# stores it
    void HueShiftPixel(int r8, int g8, int b8, double degrees, int out[3])
    {
        double r = F(r8 / 255.0), g = F(g8 / 255.0), b = F(b8 / 255.0);
        double h = 0, s = 0, v = 0;
        auto helper = [&](double offset, double dominant, double one, double two)
        {
            v = dominant;
            if (v != 0)
            {
                double small = one > two ? two : one;
                double diff = F(v - small);
                if (diff != 0)
                {
                    s = F(diff / v);
                    h = F(offset + (one - two) / diff);
                }
                else
                {
                    s = 0;
                    h = F(offset + (one - two));
                }
                h = F(h / 6);
                if (h < 0)
                    h = F(h + 1);
            }
            else
            {
                s = 0;
                h = 0;
            }
        };
        if (b > g && b > r)
            helper(4, b, r, g);
        else if (g > r)
            helper(2, g, b, r);
        else
            helper(0, r, g, b);

        double H = Repeat(F(h + degrees / 360), 1), S = s, V = v;
        double rr = 0, gg = 0, bb = 0;
        if (S == 0)
            rr = gg = bb = V;
        else if (V != 0)
        {
            double hf = H * 6;
            double temp = std::floor(hf);
            double t = F(hf - temp);
            double var1 = F(V * (1 - S)), var2 = F(V * (1 - S * t)), var3 = F(V * (1 - S * (1 - t)));
            switch ((int)temp)
            {
                case 0: rr = V; gg = var3; bb = var1; break;
                case 1: rr = var2; gg = V; bb = var1; break;
                case 2: rr = var1; gg = V; bb = var3; break;
                case 3: rr = var1; gg = var2; bb = V; break;
                case 4: rr = var3; gg = var1; bb = V; break;
                case 5: rr = V; gg = var1; bb = var2; break;
                case 6: rr = V; gg = var3; bb = var1; break;
                case -1: rr = V; gg = var1; bb = var2; break;
            }
        }
        auto to8 = [](double c) { return (int)RoundEven(F(std::min(std::max(c, 0.0), 1.0) * 255)); };
        out[0] = to8(rr);
        out[1] = to8(gg);
        out[2] = to8(bb);
    }
}
