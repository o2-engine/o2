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

    UiFrame PreviewFrame(double docW, double docH, const std::optional<double>& docDpi, const UiScreen& screen, const String& mode)
    {
        double scale = mode == "dpi" ? screen.dpi/DocDpi(docDpi) : std::sqrt((screen.w/docW)*(screen.h/docH));
        return UiFrame{ std::max(64.0, JsRound(screen.w/scale)), std::max(64.0, JsRound(screen.h/scale)), scale };
    }

    double GraphicZoom(const std::optional<double>& docDpi, const std::optional<double>& graphicDpi, const UiFrame& frame)
    {
        return KitScale(docDpi, graphicDpi)*frame.scale;
    }
}
