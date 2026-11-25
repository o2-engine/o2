#include "o2/stdafx.h"
#include "SdfGlyphGenerator.h"

#include FT_OUTLINE_H

#include "msdfgen.h"
#include "core/ShapeDistanceFinder.h"
#include "o2/Utils/Debug/Profiling/NanoProfiler.h"

namespace o2
{
    namespace
    {
        struct OutlineContext
        {
            double            scale = 1.0;
            msdfgen::Point2   position;
            msdfgen::Shape*   shape = nullptr;
            msdfgen::Contour* contour = nullptr;
        };

        msdfgen::Point2 OutlinePoint(const FT_Vector* vector, double scale)
        {
            return msdfgen::Point2(scale*vector->x, scale*vector->y);
        }

        int OutlineMoveTo(const FT_Vector* to, void* user)
        {
            auto context = (OutlineContext*)user;
            if (!(context->contour && context->contour->edges.empty()))
                context->contour = &context->shape->addContour();

            context->position = OutlinePoint(to, context->scale);
            return 0;
        }

        int OutlineLineTo(const FT_Vector* to, void* user)
        {
            auto context = (OutlineContext*)user;
            msdfgen::Point2 end = OutlinePoint(to, context->scale);
            if (end != context->position)
            {
                context->contour->addEdge(msdfgen::EdgeHolder(context->position, end));
                context->position = end;
            }
            return 0;
        }

        int OutlineConicTo(const FT_Vector* control, const FT_Vector* to, void* user)
        {
            auto context = (OutlineContext*)user;
            msdfgen::Point2 end = OutlinePoint(to, context->scale);
            if (end != context->position)
            {
                context->contour->addEdge(msdfgen::EdgeHolder(context->position, OutlinePoint(control, context->scale), end));
                context->position = end;
            }
            return 0;
        }

        int OutlineCubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
        {
            auto context = (OutlineContext*)user;
            msdfgen::Point2 end = OutlinePoint(to, context->scale);
            msdfgen::Point2 c1 = OutlinePoint(control1, context->scale), c2 = OutlinePoint(control2, context->scale);
            if (end != context->position || msdfgen::crossProduct(c1 - end, c2 - end))
            {
                context->contour->addEdge(msdfgen::EdgeHolder(context->position, c1, c2, end));
                context->position = end;
            }
            return 0;
        }

        // Loads the outline in em, not hinted; false for a glyph without one
        bool LoadShape(FT_Face face, UInt32 character, msdfgen::Shape& shape, float& advance)
        {
            FT_UInt index = FT_Get_Char_Index(face, character);
            if (index == 0 && character != 0)
                return false;

            if (FT_Load_Glyph(face, index, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) != 0)
                return false;

            double scale = 1.0/(face->units_per_EM ? face->units_per_EM : 1);
            advance = (float)(face->glyph->advance.x*scale);

            if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
                return true;

            OutlineContext context;
            context.scale = scale;
            context.shape = &shape;

            FT_Outline_Funcs functions = { OutlineMoveTo, OutlineLineTo, OutlineConicTo, OutlineCubicTo, 0, 0 };
            if (FT_Outline_Decompose(&face->glyph->outline, &functions, &context) != 0)
                return false;

            if (!shape.contours.empty() && shape.contours.back().edges.empty())
                shape.contours.pop_back();

            return true;
        }
    }

    int SdfFontSettings::GetPadding() const
    {
        return Math::CeilToInt(distanceRange*0.5f) + 1;
    }

    String SdfFontSettings::ToString() const
    {
        return String::Format("%d/%g/%d/v%d", glyphSize, distanceRange, multiChannel ? 1 : 0, SdfGlyphGenerator::version);
    }

    bool SdfFontSettings::operator==(const SdfFontSettings& other) const
    {
        return glyphSize == other.glyphSize && Math::Equals(distanceRange, other.distanceRange) &&
            multiChannel == other.multiChannel;
    }

    bool SdfFontSettings::operator!=(const SdfFontSettings& other) const
    {
        return !(*this == other);
    }

    bool SdfGlyphGenerator::Generate(FT_Face face, UInt32 character, const SdfFontSettings& settings, SdfGlyphField& field)
    {
        PROFILE_SAMPLE("Font: distance field glyph");

        field = SdfGlyphField();
        field.character = character;

        msdfgen::Shape shape;
        if (!face || !LoadShape(face, character, shape, field.advance))
            return false;

        if (shape.contours.empty() || !shape.validate())
            return true;

        shape.normalize();

        msdfgen::Shape::Bounds bounds = shape.getBounds();
        if (bounds.l >= bounds.r || bounds.b >= bounds.t)
            return true;

        // A point far outside must have a negative distance; fonts with the reverse winding are turned over
        msdfgen::Point2 outer(bounds.l - (bounds.r - bounds.l) - 1, bounds.b - (bounds.t - bounds.b) - 1);
        if (msdfgen::SimpleTrueShapeDistanceFinder::oneShotDistance(shape, outer) > 0)
        {
            for (auto& contour : shape.contours)
                contour.reverse();
        }

        double scale = settings.glyphSize;
        int padding = settings.GetPadding();
        int left = Math::FloorToInt((float)(bounds.l*scale)) - padding;
        int bottom = Math::FloorToInt((float)(bounds.b*scale)) - padding;
        int right = Math::CeilToInt((float)(bounds.r*scale)) + padding;
        int top = Math::CeilToInt((float)(bounds.t*scale)) + padding;

        field.size = Vec2I(right - left, top - bottom);
        field.bounds = RectF((float)left/(float)scale, (float)top/(float)scale, (float)right/(float)scale,
                             (float)bottom/(float)scale);

        msdfgen::Projection projection(msdfgen::Vector2(scale), msdfgen::Vector2(-left/scale, -bottom/scale));
        msdfgen::SDFTransformation transformation(projection, msdfgen::Range(settings.distanceRange/scale));

        int count = field.size.x*field.size.y;
        field.pixels.resize(count*4);
        auto toByte = [](float value) { return (UInt8)Math::Clamp(Math::RoundToInt(value*255.0f), 0, 255); };

        if (settings.multiChannel)
        {
            msdfgen::edgeColoringInkTrap(shape, 3.0);

            msdfgen::MSDFGeneratorConfig config(true, msdfgen::ErrorCorrectionConfig(
                msdfgen::ErrorCorrectionConfig::EDGE_PRIORITY, msdfgen::ErrorCorrectionConfig::CHECK_DISTANCE_AT_EDGE));

            msdfgen::Bitmap<float, 4> mtsdf(field.size.x, field.size.y);
            msdfgen::generateMTSDF(mtsdf, shape, transformation, config);

            for (int y = 0; y < field.size.y; y++)
            {
                for (int x = 0; x < field.size.x; x++)
                {
                    const float* source = mtsdf(x, y);
                    UInt8* pixel = &field.pixels[(y*field.size.x + x)*4];
                    for (int c = 0; c < 4; c++)
                        pixel[c] = toByte(source[c]);
                }
            }
        }
        else
        {
            msdfgen::Bitmap<float, 1> sdf(field.size.x, field.size.y);
            msdfgen::generateSDF(sdf, shape, transformation, msdfgen::GeneratorConfig(true));

            for (int y = 0; y < field.size.y; y++)
            {
                for (int x = 0; x < field.size.x; x++)
                {
                    UInt8 value = toByte(*sdf(x, y));
                    UInt8* pixel = &field.pixels[(y*field.size.x + x)*4];
                    pixel[0] = pixel[1] = pixel[2] = pixel[3] = value;
                }
            }
        }

        return true;
    }

    float SdfGlyphGenerator::GetCapHeight(FT_Face face)
    {
        if (!face)
            return 0.7f;

        FT_UInt index = FT_Get_Char_Index(face, 'A');
        if (index != 0 && FT_Load_Glyph(face, index, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) == 0)
            return (float)face->glyph->metrics.horiBearingY/(float)(face->units_per_EM ? face->units_per_EM : 1);

        return (float)face->ascender/(float)(face->units_per_EM ? face->units_per_EM : 1);
    }
}
