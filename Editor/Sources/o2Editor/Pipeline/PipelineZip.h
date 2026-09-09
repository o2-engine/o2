#pragma once

#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // ------------------------------------------------------------------
    // Minimal ZIP archive reader and writer: stored and deflated entries
    // ------------------------------------------------------------------
    namespace PipelineZip
    {
        // One file of the archive
        struct Entry
        {
            String name; // Path inside the archive, folders separated by '/'
            String data; // File bytes
        };

        // Returns true when the bytes start with the local file header signature
        bool IsZip(const String& bytes);

        // Reads the archive files; returns false with error set on a broken or unsupported archive
        bool Read(const String& bytes, Vector<Entry>& entries, String& error);

        // Builds an archive of the entries, deflating those that get smaller when compress is set
        String Write(const Vector<Entry>& entries, bool compress);
    }
}
