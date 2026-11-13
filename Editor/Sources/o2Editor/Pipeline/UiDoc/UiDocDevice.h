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

    // Real devices, portrait (desktops landscape): the kit's target and the preview picker's list
    const Vector<UiDevice>& UiDevices();

    // The device a kit is made for when it names none
    extern const char* const UI_DEFAULT_DEVICE;

    // ------------------------------------------------------------------
    // A kit's target device: a known one (id) or a screen typed in
    // ------------------------------------------------------------------
    struct UiTarget
    {
        String id;   // "" for a typed screen
        String name;
        double w = 0, h = 0, dpi = UI_BASE_DPI;

        // The target as a screen
        UiScreen Screen() const { return UiScreen{ w, h, dpi }; }

        bool operator==(const UiTarget& other) const
        {
            return id == other.id && name == other.name && w == other.w && h == other.h && dpi == other.dpi;
        }
    };

    // A known device by id, null when there is none
    const UiDevice* DeviceById(const String& id);

    // The default device as a target (iPhone 16)
    UiTarget DefaultTarget();

    // A stored or typed screen ({id} of a known device, or {w, h, dpi, name}) -> a target; none when it is not one
    std::optional<UiTarget> CleanTarget(const Json* raw);

    // The target as JSON: {id, name, w, h, dpi} (a typed screen without id)
    Json TargetToJson(const UiTarget& target);

    // The target device a UI kit node's config names (kitDevice: a device id, or "custom" with kitDeviceCustom
    // {w, h, dpi}); the default device otherwise
    UiTarget TargetOfConfig(const Json& config);

    // Device pixels per pixel of a picture taken to be the device's screen: the picture is shown whole on it
    double SourceFit(const UiScreen& target, double srcW, double srcH);

    // The area a picture takes on the device's screen, device pixels, and the fit
    struct UiOnTarget
    {
        double w = 1, h = 1, k = 1;
    };
    UiOnTarget SourceOnTarget(const UiScreen& target, double srcW, double srcH);

    // Whether a layout at docDpi is in its device's pixels (its density is the device's)
    bool InTargetUnits(const std::optional<double>& docDpi, const UiScreen& target);

    // ------------------------------------------------------------------------------------------
    // What brings a layout to its device's pixels: the values of a `screen` op with `scale`
    // ------------------------------------------------------------------------------------------
    struct UiTargetUnits
    {
        double width = 0, height = 0, dpi = UI_BASE_DPI, scale = 1, fromDpi = UI_BASE_DPI;
    };
    UiTargetUnits ToTargetUnits(double docW, double docH, const std::optional<double>& docDpi, const UiScreen& target);

    // "9:16", "3:4", "19.5:9" - a screen's aspect ratio, the short side first for portrait
    String AspectName(double w, double h);


    // The canvas a layout is solved on for a screen
    struct UiFrame
    {
        double w = 0, h = 0; // Design pixels
        double scale = 1.0;  // Screen pixels per design pixel

        bool operator==(const UiFrame& other) const { return w == other.w && h == other.h && scale == other.scale; }
    };

    // The canvas a layout (design size docW x docH) is solved on for a screen: the design fitted into the screen (scaled
    // until it shows whole), the room left over on one side added to the canvas - Unity's Canvas Scaler in Expand mode with
    // the design size as its reference resolution. The mode of older view states is ignored: every screen is met by fit
    UiFrame PreviewFrame(double docW, double docH, const std::optional<double>& docDpi, const UiScreen& screen, const String& mode);

    // How many screen pixels a pixel of a kit graphic covers on that screen: above 1 the graphic is enlarged (soft)
    double GraphicZoom(const std::optional<double>& docDpi, const std::optional<double>& graphicDpi, const UiFrame& frame);
}
