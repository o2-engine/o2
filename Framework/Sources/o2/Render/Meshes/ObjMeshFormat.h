#pragma once

#include "o2/Utils/Math/Mesh3DPrimitives.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // Wavefront OBJ geometry parsing and writing. Supports v/vt/vn attributes, polygonal faces
    // (triangulated as fans), negative relative indices; materials and groups are ignored
    namespace ObjMeshFormat
    {
        bool Parse(const String& text, Mesh3DData& output, String* errorMessage = nullptr);

        String Write(const Mesh3DData& data);
    }
}
