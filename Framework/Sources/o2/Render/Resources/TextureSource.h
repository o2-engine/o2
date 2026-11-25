#pragma once

#ifndef TEXTURE_SOURCE_H
#define TEXTURE_SOURCE_H

#include "o2/Render/Resources/TextureRef.h"
#include "o2/Utils/Math/Rect.h"

namespace o2
{
    struct TextureSource 
    {
        TextureRef texture;
        RectF      sourceRect;
    };
}

#endif
