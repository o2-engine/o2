#pragma once

#include <optional>

#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------------------------------------------
    // What AssetsLine writes into a PNG besides its pixels (port of shared/pngMeta.ts): the density the picture is drawn
    // for and, for an AI extract part, how much larger the model drew it than the source showed it. Both ride in tEXt
    // chunks ("AssetsLine:dpi", "AssetsLine:scale"), so they travel with the bytes through the cache, the sync and the
    // exports unchanged; a pHYs chunk carries the density as well for image editors. Only our own keys are read back
    // ---------------------------------------------------------------------------------------------------------------
    namespace PipelinePngMeta
    {
        extern const char* dpiKey;   // "AssetsLine:dpi"
        extern const char* scaleKey; // "AssetsLine:scale"
        extern const char* sourceKey; // "AssetsLine:source": the size of the picture an AI extract part was cut from, "WxH"

        // Returns true when the bytes start with the PNG signature and hold more than a header
        bool IsPng(const String& bytes);

        // Returns our text entries of a PNG ("AssetsLine:..." keys before the first IDAT); empty for anything else
        Map<String, String> ReadText(const String& png);

        // Returns the PNG with these text entries set (an empty value removes one) and, with a density, its pHYs chunk;
        // anything that is not a PNG comes back as it is. New chunks go right after IHDR, in the order given
        String WriteText(const String& png, const Vector<Pair<String, String>>& entries, std::optional<double> dpi = std::nullopt);

        // Returns the density the picture is tagged with
        std::optional<double> ReadDpi(const String& png);

        // Returns how much larger than its source an AI extract part was drawn
        std::optional<double> ReadScale(const String& png);

        // Returns the size of the picture an AI extract part was cut from, false when it is not tagged
        bool ReadSource(const String& png, int& width, int& height);

        // Returns the PNG tagged with the density (none removes the tag) and, when given, the extract scale and the size of
        // the source picture; the density is written at 0.1 and the scale at 0.0001, as AssetsLine does
        String WithDpi(const String& png, std::optional<double> dpi, std::optional<double> scale = std::nullopt,
                       int sourceW = 0, int sourceH = 0);

        // Returns the pixel size from the IHDR, false for anything that is not a PNG
        bool Size(const String& png, int& width, int& height);
    }
}
