#include "o2Editor/stdafx.h"
#include "UiDocDevice.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    std::optional<double> CleanDpi(double n)
    {
        if (!std::isfinite(n) || n <= 0)
            return std::nullopt;

        return JsRound(std::max(UI_DPI_MIN, std::min(UI_DPI_MAX, n))*10.0)/10.0;
    }

    std::optional<double> CleanDpi(const Json* value)
    {
        return CleanDpi(JsNumber(value));
    }

    double DocDpi(const std::optional<double>& docDpi)
    {
        auto d = docDpi ? CleanDpi(*docDpi) : std::nullopt;
        return d ? *d : UI_BASE_DPI;
    }

    double GraphicDpi(const std::optional<double>& graphicDpi)
    {
        auto d = graphicDpi ? CleanDpi(*graphicDpi) : std::nullopt;
        return d ? *d : UI_BASE_DPI;
    }

    double KitScale(const std::optional<double>& docDpi, const std::optional<double>& graphicDpi)
    {
        return DocDpi(docDpi)/GraphicDpi(graphicDpi);
    }

    UiSize DesignSize(const std::optional<double>& docDpi, double width, double height, const std::optional<double>& graphicDpi)
    {
        double k = KitScale(docDpi, graphicDpi);
        return UiSize{ JsRound(width*k), JsRound(height*k) };
    }

    String DensityName(double dpi)
    {
        double x = dpi/160.0;
        return "@" + JsNum(JsRound(x*100.0)/100.0) + "x";
    }

    const Vector<UiDevice>& UiDevices()
    {
        static const Vector<UiDevice> devices = {
            { "iphone-16", "iPhone 16", "phone", 1179, 2556, 460 },
            { "iphone-16-pro", "iPhone 16 Pro", "phone", 1206, 2622, 460 },
            { "iphone-16-pro-max", "iPhone 16 Pro Max", "phone", 1320, 2868, 460 },
            { "iphone-se", "iPhone SE", "phone", 750, 1334, 326 },
            { "iphone-15", "iPhone 15", "phone", 1179, 2556, 460 },
            { "iphone-15-pro-max", "iPhone 15 Pro Max", "phone", 1290, 2796, 460 },
            { "pixel-8", "Pixel 8", "phone", 1080, 2400, 428 },
            { "galaxy-s24", "Galaxy S24", "phone", 1080, 2340, 416 },
            { "android-1080", "Android 1080\xC3\x97" "1920", "phone", 1080, 1920, 480 },
            { "android-720", "Android 720\xC3\x97" "1280", "phone", 720, 1280, 320 },
            { "ipad", "iPad 10.9\xE2\x80\xB3", "tablet", 1640, 2360, 264 },
            { "ipad-mini", "iPad mini", "tablet", 1488, 2266, 326 },
            { "ipad-pro-13", "iPad Pro 13\xE2\x80\xB3", "tablet", 2064, 2752, 264 },
            { "galaxy-tab-s9", "Galaxy Tab S9", "tablet", 1600, 2560, 274 },
            { "desktop-1080", "Desktop 1920\xC3\x97" "1080", "desktop", 1920, 1080, 96 },
            { "desktop-1440", "Desktop 2560\xC3\x97" "1440", "desktop", 2560, 1440, 109 },
            { "macbook-air", "MacBook Air 13\xE2\x80\xB3", "desktop", 2560, 1664, 224 },
        };
        return devices;
    }

    const char* const UI_DEFAULT_DEVICE = "iphone-16";

    const UiDevice* DeviceById(const String& id)
    {
        return UiDevices().Find([&](const UiDevice& d) { return d.id == id; });
    }

    UiTarget DefaultTarget()
    {
        const UiDevice* d = DeviceById(UI_DEFAULT_DEVICE);
        return UiTarget{ d->id, d->name, d->w, d->h, d->dpi };
    }

    std::optional<UiTarget> CleanTarget(const Json* raw)
    {
        if (!raw || !(raw->IsObject() || raw->IsArray()))
            return std::nullopt;

        const Json* id = JsGet(raw, "id");
        if (id && id->IsString())
        {
            if (const UiDevice* known = DeviceById(id->Str()))
                return UiTarget{ known->id, known->name, known->w, known->h, known->dpi };
        }

        double w = JsRound(JsNumber(JsGet(raw, "w"))), h = JsRound(JsNumber(JsGet(raw, "h")));
        auto dpi = CleanDpi(JsGet(raw, "dpi"));
        if (!(w >= 64 && h >= 64 && w <= 8192 && h <= 8192) || !dpi)
            return std::nullopt;

        String name = JsSlice(JsTrim(JsStringOr(JsGet(raw, "name"))), 0, 40);
        if (name.IsEmpty())
            name = JsNum(w) + "\xC3\x97" + JsNum(h);
        return UiTarget{ "", name, w, h, *dpi };
    }

    Json TargetToJson(const UiTarget& target)
    {
        Json out = Json::Object();
        if (!target.id.IsEmpty())
            out["id"] = target.id;
        out["name"] = target.name;
        out["w"] = target.w;
        out["h"] = target.h;
        out["dpi"] = target.dpi;
        return out;
    }

    UiTarget TargetOfConfig(const Json& config)
    {
        const Json* device = JsGet(config, "kitDevice");
        if (device && device->IsString() && device->Str() == "custom")
        {
            Json custom = Json::Object();
            const Json* given = JsGet(config, "kitDeviceCustom");
            if (given && given->IsObject())
                custom = *given;
            custom["name"] = "Custom";
            auto t = CleanTarget(&custom);
            return t ? *t : DefaultTarget();
        }

        Json byId = Json::Object();
        byId["id"] = device ? *device : Json();
        auto t = CleanTarget(&byId);
        return t ? *t : DefaultTarget();
    }

    double SourceFit(const UiScreen& target, double srcW, double srcH)
    {
        if (!(srcW > 0 && srcH > 0))
            return 1;
        return std::min(target.w/srcW, target.h/srcH);
    }

    UiOnTarget SourceOnTarget(const UiScreen& target, double srcW, double srcH)
    {
        double k = SourceFit(target, srcW, srcH);
        return UiOnTarget{ std::max(1.0, JsRound(srcW*k)), std::max(1.0, JsRound(srcH*k)), k };
    }

    bool InTargetUnits(const std::optional<double>& docDpi, const UiScreen& target)
    {
        return std::fabs(DocDpi(docDpi) - target.dpi) < 0.5;
    }

    UiTargetUnits ToTargetUnits(double docW, double docH, const std::optional<double>& docDpi, const UiScreen& target)
    {
        auto on = SourceOnTarget(target, docW, docH);
        return UiTargetUnits{ on.w, on.h, target.dpi, JsRound(on.k*100000)/100000, DocDpi(docDpi) };
    }

    String AspectName(double w, double h)
    {
        double a = std::max(w, h)/std::min(w, h);
        static const int ratios[][2] = { { 16, 9 }, { 4, 3 }, { 3, 2 }, { 16, 10 }, { 5, 4 }, { 1, 1 }, { 21, 9 }, { 2, 1 } };
        for (auto& r : ratios)
        {
            if (std::fabs(a - (double)r[0]/r[1]) < 0.012)
                return w >= h ? JsNum(r[0]) + ":" + JsNum(r[1]) : JsNum(r[1]) + ":" + JsNum(r[0]);
        }
        double v = JsRound(a*9*2)/2;
        return w >= h ? JsNum(v) + ":9" : "9:" + JsNum(v);
    }

    UiFrame PreviewFrame(double docW, double docH, const std::optional<double>& docDpi, const UiScreen& screen, const String& mode)
    {
        double scale = std::min(screen.w/docW, screen.h/docH);
        return UiFrame{ std::max(docW, JsRound(screen.w/scale)), std::max(docH, JsRound(screen.h/scale)), scale };
    }

    double GraphicZoom(const std::optional<double>& docDpi, const std::optional<double>& graphicDpi, const UiFrame& frame)
    {
        return KitScale(docDpi, graphicDpi)*frame.scale;
    }
}
