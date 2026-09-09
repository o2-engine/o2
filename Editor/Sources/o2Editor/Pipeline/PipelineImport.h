#pragma once

#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/PipelineValue.h"

using namespace o2;

namespace Editor
{
    // ------------------------------------------------------------------------------------
    // Import of AssetsLine exports: the pipeline JSON or a ZIP bundle, optionally with results
    // ------------------------------------------------------------------------------------
    namespace PipelineImport
    {
        // Parsed AssetsLine export: the converted graph and the node results the file carried
        struct Bundle
        {
            bool                       ok = false; // True when the graph was parsed
            String                     error;      // Failure reason when ok is false
            String                     name;       // Pipeline name from the file
            PipelineGraph              graph;      // Converted graph with the original node and port ids and a fresh cache id
            Map<String, PipelineValue> results;    // Node results by node id
        };

        // Parses a pipeline JSON or a JSON bundle with results as exported by AssetsLine
        Bundle Parse(const String& json);

        // Parses a ZIP bundle: pipeline.json, an optional manifest.json and the results/<nodeId>.<ext> files
        Bundle ParseZip(const String& bytes);

        // Parses a file by its content: a ZIP bundle or a JSON text
        Bundle ParseBytes(const String& bytes);

        // Stores the bundle results into the pipeline cache as previews, content and freshness markers; returns their count
        int StoreResults(const String& pipelineId, const Bundle& bundle);

        // Imports the file as a new pipeline asset inside the folder, stores its results and rebuilds the assets;
        // returns the asset path, empty with error set when the import failed
        String ImportFile(const String& filePath, const String& folder, String& error);

        // Returns a file name usable for an asset: letters, digits, spaces, dashes and underscores of the name
        String SafeAssetName(const String& name);
    }
}
