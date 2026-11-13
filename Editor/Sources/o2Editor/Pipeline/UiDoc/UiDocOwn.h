#pragma once

#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocOps.h"
#include "o2Editor/Pipeline/UiDoc/UiKitModel.h"

// ------------------------------------------------------------------------------------------------
// What an AI UI layout node makes for itself, besides the layout (port of AssetsLine shared/uiDoc/own.ts
// and the layout part of shared/nodePorts.ts): graphics the image model drew or cut from the reference
// because no kit had them, and text styles read off the reference (or designed). They live in the step
// that made them (step.assets, step.styles), so undo takes them back. The node's kit is every connected UI
// kit plus these (MergeKits); each own graphic is also an image output of the node, id own-<name>
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // -----------------------------------------------------
    // A graphic the node generated: a prepared PNG upload
    // -----------------------------------------------------
    struct UiOwnAsset
    {
        String                name;
        String                type = "other";
        String                assetType = "simple";
        std::optional<String> variant;
        std::optional<String> description;
        std::optional<String> purpose;     // Where to use it (as a kit element's purpose)
        String                uploadId;
        double                width = 1, height = 1;
        std::optional<UiNine> nineSlice;
        std::optional<double> dpi;
        std::optional<String> from;        // drawn (by the image model) | cut (out of the reference)
    };

    // A stored own graphic -> cleaned; none when it is not one (no upload, no name, a path in the upload id)
    std::optional<UiOwnAsset> CleanOwnAsset(const Json& raw);

    // The graphic as the JSON a step stores
    Json OwnAssetToJson(const UiOwnAsset& asset);

    // ----------------------------------------------
    // The node's own graphics and styles after its steps
    // ----------------------------------------------
    struct UiOwnKit
    {
        Vector<UiOwnAsset> assets;
        Vector<TextStyle>  styles;
    };

    // The node's own graphics and styles after the steps: a later entry replaces an earlier one of the same name; a step
    // that brought the layout to other units (a screen op with scale) rescales what was made before it
    UiOwnKit OwnKitOf(const Vector<UiStep>& steps);

    // The node's own graphics after the steps (OwnKitOf without the styles)
    Vector<UiOwnAsset> OwnAssetsOf(const Vector<UiStep>& steps);

    // Every upload a step list keeps alive
    Vector<String> OwnUploadIds(const Vector<UiStep>& steps);

    // An own graphic as a kit element; png = its picture (base64) when the caller has it
    UiKitElement OwnElement(const UiOwnAsset& asset, const String& png = "");

    // Several kits as one, in order: a name met again gets a numbered suffix (btn_2); the device is the first kit's that
    // names one. None for nothing at all
    std::optional<UiKit> MergeKits(const Vector<const UiKit*>& kits);

    // A name not taken in the kit (graphics) nor among alsoTaken - for a new own asset
    String FreeKitName(const UiKit* kit, const String& want, const Vector<String>& alsoTaken = {});

    // Every upload a node's config keeps alive: a source's file, a kit element's prepared picture, own graphics in steps and
    // in uiRedo (shared/uploads.ts configUploadIds)
    Vector<String> ConfigUploadIds(const Json& config);

    // --- the layout node's outputs ------------------------------------------------------------------

    // ------------------------------------------------
    // One of the layout node's own graphics as an output
    // ------------------------------------------------
    struct LayoutOwnPort
    {
        String     name;
        UiOwnAsset asset;
    };

    // True for the layout node's schema outputs (image, layout)
    bool IsLayoutFixedOutput(const String& name);

    // The node's own graphics as outputs, named after the graphic (a schema output's name gets a number: image_2)
    Vector<LayoutOwnPort> LayoutOwnPorts(const Json& config);

    // The id a graphic's output is given: own-<name lower-cased, anything but a-z 0-9 _ - as _>
    String LayoutOwnPortId(const String& name);

    // ------------------------------------------
    // One of the layout node's graphic outputs
    // ------------------------------------------
    struct LayoutOutputPort
    {
        String id;
        String name;
    };

    // The node's graphic outputs after its steps (the schema outputs image / layout come first and are the node's own):
    // a graphic's output the node already has by name (had: name -> id of its non-schema outputs) keeps its port id
    Vector<LayoutOutputPort> LayoutOwnOutputs(const Json& config, const Vector<Pair<String, String>>& had);

    // The graphic behind an output name; none for the schema outputs and a graphic that is gone
    std::optional<LayoutOwnPort> LayoutOwnPortByName(const Json& config, const String& name);

    // What one of a layout node's own graphics is, as a signature: the upload and the density it is tagged with
    String OwnGraphicSig(const LayoutOwnPort& own);
}
