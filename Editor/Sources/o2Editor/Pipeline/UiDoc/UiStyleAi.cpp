#include "o2Editor/stdafx.h"
#include "UiStyleAi.h"

#include <cmath>

#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAiShared.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    const char* const STYLE_MATTE = "#808080";

    bool StylePictureForModel(const String& png, UiStylePicture& out, int max /*= 1024*/)
    {
        UiLayout::UiImage img;
        if (!UiLayout::UiImage::Decode(png, img) || img.IsEmpty())
            return false;

        bool transparent = false;
        for (size_t i = 3; i < img.data.size() && !transparent; i += 4)
            transparent = img.data[i] < 255;

        bool big = img.w > max || img.h > max;
        out = UiStylePicture();
        out.matte = transparent;
        if (!transparent && !big && PipelinePngMeta::IsPng(png))
        {
            out.png = png;
            out.width = img.w;
            out.height = img.h;
            return true;
        }

        if (transparent)
            img = img.Flatten(0x80, 0x80, 0x80);
        if (big)
        {
            double k = std::min((double)max/img.w, (double)max/img.h);
            img = img.Resize(std::max(1, (int)JsRound(img.w*k)), std::max(1, (int)JsRound(img.h*k)));
        }
        out.png = img.Png();
        out.width = img.w;
        out.height = img.h;
        return true;
    }

    bool StyleSourcePicture(const String& image, const PipelineImageOps::CropRect* frame, UiStylePicture& out)
    {
        if (!frame)
            return StylePictureForModel(image, out);

        UiLayout::UiImage whole;
        if (!UiLayout::UiImage::Decode(image, whole) || whole.IsEmpty())
            return false;

        int left = std::clamp((int)std::lround(frame->x*whole.w), 0, whole.w - 1);
        int top = std::clamp((int)std::lround(frame->y*whole.h), 0, whole.h - 1);
        int w = std::max(1, std::min(whole.w - left, (int)std::lround(frame->w*whole.w)));
        int h = std::max(1, std::min(whole.h - top, (int)std::lround(frame->h*whole.h)));
        UiLayout::UiImage cut = whole.Crop(left, top, w, h);
        if (!StylePictureForModel(cut.Png(), out))
            return false;

        double k = (double)out.width/std::max(1, cut.w);
        out.fullWidth = JsRound(whole.w*k);
        return true;
    }

    Coroutine<UiStyleExtractResult> ExtractTextStyle(Ref<PipelineExecContext> ctx, String image, String hint, String model,
                                                    std::optional<PipelineImageOps::CropRect> frame, String name, String role)
    {
        UiStyleExtractResult res;
        res.model = JsTrim(model).IsEmpty() ? String(TEXT_STYLE_AI_DEFAULT_MODEL) : JsTrim(model);
        hint = JsTrim(hint);

        UiStylePicture picture;
        if (!StyleSourcePicture(image, frame ? &*frame : nullptr, picture))
        {
            res.error = "The picture could not be read";
            co_return res;
        }

        StyleExtractRequest req;
        req.width = picture.width;
        req.height = picture.height;
        req.hint = hint;
        req.matte = picture.matte ? String(STYLE_MATTE) : String();
        req.fullWidth = picture.fullWidth;

        AiTextResult answer = co_await UiAiText(ctx, res.model, BuildStyleExtractPrompt(req), { AiImageRef{ "image/png", picture.png } });
        if (!answer.ok)
        {
            res.error = answer.error.IsEmpty() ? String("The model did not answer") : answer.error;
            co_return res;
        }

        String kind = ParseStyleExtractAnswer(answer.text, picture.fullWidth > 0 ? picture.fullWidth : picture.width, res.style,
                                              name, role);
        if (kind == "none")
            res.error = "No lettering found in the picture";
        else if (kind.IsEmpty())
            res.error = "The model (" + res.model + ") did not answer with a text style";
        else
            res.ok = true;
        co_return res;
    }

    String WithStyleOverrides(const String& styleJson, const String& name, const String& role)
    {
        Json style;
        if (!Json::Parse(styleJson, style) || !style.IsObject())
            return styleJson;
        return ApplyStyleOverrides(style, name, role).Dump();
    }

    UiDoc SampleDoc(const TextStyle& style, const String& text, const String& background)
    {
        TextReach reach = EffectsReach(style);
        double h = std::max(224.0, std::ceil(style.size*1.25 + reach.top + reach.bottom + 28));
        double w = std::max(JsRound(680*h/224), std::ceil(EstimateTextWidth(text, style)*1.03 + 36 + reach.left + reach.right + 8));
        UiDoc d = EmptyDoc("Sample", w, h);
        d.root = UiNode();
        d.root.id = "root";
        d.root.type = "screen";
        d.root.Set("color", Json(background));

        UiNode label;
        label.id = "sample";
        label.type = "text";
        label.Set("anchor", Json(String("stretch")));
        Json margin = Json::Array();
        for (double v : { 12.0, 18 + reach.left, 12.0, 18 + reach.right })
            margin.Add(Json(v));
        label.Set("margin", margin);
        label.Set("style", Json(style.name));
        label.Set("text", Json(text));
        d.root.SetChildren({ label });
        return d;
    }
}
