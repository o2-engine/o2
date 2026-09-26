#pragma once

#include "o2Editor/Pipeline/Sync/AssetsLineSync.h"

namespace Editor
{
    // A pipeline asset of the synced folder as read in this pass
    struct AssetsLineSync::LocalPipeline
    {
        String        file;     // Relative to the assets folder
        String        stamp;    // File stamp when it was read
        PipelineGraph graph;    // The document
        String        content;  // ContentText of the document
    };

    // Everything one pass works with
    struct AssetsLineSync::Pass
    {
        AssetsLineClient client;     // Requests of this pass
        AssetsLineConfig config;     // Settings at the start of the pass
        String           assetsPath; // Assets folder, with a trailing slash
        String           folder;     // Synced folder relative to the assets folder, with a trailing slash

        Map<String, int>    remoteRev;  // Stored pipelines: id -> revision
        Map<String, String> remoteName; // Stored pipelines: id -> name
        double              resultsSeq = 0; // Server's result counter

        Map<String, LocalPipeline> local;      // Local pipelines by id
        Vector<String>             changedIds; // Documents pushed or pulled in this pass
        Vector<String>             busyIds;    // Documents skipped because the editor holds unsaved edits
        bool                       assetsChanged = false; // Files under the assets folder were written
        String                     error;      // First failure
        bool                       offline = false; // The server could not be reached
    };

    namespace AssetsLineSyncUtils
    {
        // Reads a pipeline document from JSON text in either format; the store's bookkeeping is dropped
        bool ParseDocument(const String& text, PipelineGraph& graph);

        // Reads a pipeline document the server sent; the store's bookkeeping is dropped
        bool ParseDocument(const DataValue& json, PipelineGraph& graph);

        // Gives every node, port and edge a new id (a copied document must not alias the original's results)
        void RemapAllIds(PipelineGraph& graph);

        // Returns a name usable as a file name
        String SafeFileName(const String& name);

        // Returns the stamp of a file: size, edit time and, for a small file, a hash of its content; empty when it does not exist
        String Stamp(const String& path);

        // True when a stamp recorded earlier describes the file as it is now; a stamp recorded without a content hash
        // compares without it
        bool SameStamp(const String& known, const String& now);

        // Moves a file with its .meta
        bool MoveWithMeta(const String& from, const String& to);

        // Deletes a file with its .meta
        void DeleteWithMeta(const String& path);

        // Returns the asset UID stored in the .meta of a file, empty when there is none
        String MetaUid(const String& path);
    }
}
