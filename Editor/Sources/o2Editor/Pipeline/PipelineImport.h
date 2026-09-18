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

            Map<String, Map<String, PipelineValue>> portResults; // Results of a per-port node by node id and output port id
            Map<String, String>                     uploads;     // Source files the bundle carries, by upload id
        };

        // Parses a pipeline JSON or a JSON bundle with results as exported by AssetsLine
        Bundle Parse(const String& json);

        // Parses a ZIP bundle: pipeline.json, an optional manifest.json, the results/<nodeId>.<ext> files and,
        // from a v3 manifest, the parts of per-port nodes and the source uploads
        Bundle ParseZip(const String& bytes);

        // Parses a file by its content: a ZIP bundle or a JSON text
        Bundle ParseBytes(const String& bytes);

        // Stores the bundle results into the pipeline cache as previews, content and freshness markers, and writes
        // the source uploads it carries into the uploads folder; returns the number of stored results
        int StoreResults(const String& pipelineId, const Bundle& bundle);

        // Imports the file as a new pipeline asset inside the folder, stores its results and rebuilds the assets;
        // returns the asset path, empty with error set when the import failed
        String ImportFile(const String& filePath, const String& folder, String& error);

        // Returns a file name usable for an asset: letters, digits, spaces, dashes and underscores of the name
        String SafeAssetName(const String& name);
    }
}
