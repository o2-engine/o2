#pragma once

#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"

// ------------------------------------------------------------------------------------------------
// Pixel density for the UI kit and the AI UI layout (port of AssetsLine shared/uiDoc/device.ts).
// A kit graphic is drawn for some density (its DPI), and so is a layout (the DPI of its design
// pixels). Drawn into the layout, one pixel of the graphic takes layoutDpi / graphicDpi design pixels.
// What says nothing about its density is at the base density, 480 dpi (Android's xxhdpi, @3x).
// The UiDoc / UiKitElement overloads live in UiDocModel.h and UiDocSkin.h
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // The density of everything untagged
    constexpr double UI_BASE_DPI = 480.0;

    // The lowest density a picture or a layout may have
    constexpr double UI_DPI_MIN = 36.0;

    // The highest: a 20 px icon an image model redrew at 1024 px is ~50x the source
    constexpr double UI_DPI_MAX = 96000.0;

    // A density as given (any number, text or nothing) -> clamped to [36, 96000] at 0.1, nothing when it is not positive
    std::optional<double> CleanDpi(const Json* value);
    std::optional<double> CleanDpi(double value);

    // A layout's density: its own or the base
    double DocDpi(const std::optional<double>& docDpi);

    // A graphic's density: its own or the base
    double GraphicDpi(const std::optional<double>& graphicDpi);

    // Design pixels one pixel of a kit graphic takes in a layout
    double KitScale(const std::optional<double>& docDpi, const std::optional<double>& graphicDpi);

    // A graphic's size in the layout's design pixels
    struct UiSize
    {
        double w = 0, h = 0;

        bool operator==(const UiSize& other) const { return w == other.w && h == other.h; }
    };

    // A graphic's width x height in design pixels of a layout at docDpi
    UiSize DesignSize(const std::optional<double>& docDpi, double width, double height, const std::optional<double>& graphicDpi);

    // "@3x" for 480 dpi: the Android / iOS scale name of a density (160 dpi = @1x)
    String DensityName(double dpi);

    // --- screens ---------------------------------------------------------------------------------

    // A screen: pixels and pixel density
    struct UiScreen
    {
        double w = 0, h = 0, dpi = UI_BASE_DPI;
    };

    // A real device, portrait (desktops landscape)
    struct UiDevice
    {
        String id;
        String name;
        String kind; // phone | tablet | desktop
        double w = 0, h = 0, dpi = UI_BASE_DPI;

        // The device as a screen
        UiScreen Screen() const { return UiScreen{ w, h, dpi }; }
    };

    // The 14 devices of the preview picker
    const Vector<UiDevice>& UiDevices();


    // The canvas a layout is solved on for a screen
    struct UiFrame
    {
        double w = 0, h = 0; // Design pixels
        double scale = 1.0;  // Screen pixels per design pixel

        bool operator==(const UiFrame& other) const { return w == other.w && h == other.h && scale == other.scale; }
    };

    // The canvas of a layout (design size docW x docH at docDpi) shown on a screen. mode "dpi": by physical size (a denser
    // or bigger screen shows more room around same-sized blocks); "fit": by the screen's size like Unity's Canvas Scaler
    // (scale with screen size, match 0.5), how the Unity export behaves
    UiFrame PreviewFrame(double docW, double docH, const std::optional<double>& docDpi, const UiScreen& screen, const String& mode);

    // How many screen pixels a pixel of a kit graphic covers on that screen: above 1 the graphic is enlarged (soft)
    double GraphicZoom(const std::optional<double>& docDpi, const std::optional<double>& graphicDpi, const UiFrame& frame);
}
