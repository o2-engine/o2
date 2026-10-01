#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/OpenRouterProvider.h"

using namespace o2;
using namespace Editor;

namespace
{
    const char* kModel = "google/gemini-3.1-flash-image";

    DataDocument Parse(const String& json)
    {
        DataDocument doc;
        EXPECT_TRUE(doc.LoadFromData(json)) << json;
        return doc;
    }

    String ImageAnswer(const String& message)
    {
        return "{\"id\":\"gen-1\",\"choices\":[{\"finish_reason\":\"stop\",\"message\":{\"role\":\"assistant\"," + message + "}}]}";
    }
}

TEST(OpenRouterProvider, ImageComesFromTheImagesOfTheMessage)
{
    String bytes = "\x89PNG-bytes";
    auto doc = Parse(ImageAnswer("\"content\":\"Here it is\",\"images\":[{\"type\":\"image_url\",\"image_url\":{\"url\":\"" +
                                 PipelineUtils::BytesToDataUrl(bytes, "image/png") + "\"}}]"));
    AiBytesResult result = OpenRouterProvider::ParseImageResponse(doc, kModel);
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.data, bytes);
    EXPECT_EQ(result.mimeType, String("image/png"));
}

TEST(OpenRouterProvider, ImageComesFromTheContentPartsToo)
{
    String bytes = "jpeg-bytes";
    auto doc = Parse(ImageAnswer("\"content\":[{\"type\":\"text\",\"text\":\"done\"},{\"type\":\"image_url\",\"image_url\":{\"url\":\"" +
                                 PipelineUtils::BytesToDataUrl(bytes, "image/jpeg") + "\"}}]"));
    AiBytesResult result = OpenRouterProvider::ParseImageResponse(doc, kModel);
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.data, bytes);
    EXPECT_EQ(result.mimeType, String("image/jpeg"));
}

TEST(OpenRouterProvider, TextOnlyAnswerFailsAnImageCall)
{
    auto doc = Parse(ImageAnswer("\"content\":\"I cannot draw that\""));
    AiBytesResult result = OpenRouterProvider::ParseImageResponse(doc, kModel);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, String("OpenRouter returned text instead of an image (model google/gemini-3.1-flash-image): I cannot draw that"));

    auto empty = Parse(ImageAnswer("\"content\":\"\""));
    AiBytesResult none = OpenRouterProvider::ParseImageResponse(empty, kModel);
    EXPECT_FALSE(none.ok);
    EXPECT_EQ(none.error, String("OpenRouter returned no image data (model google/gemini-3.1-flash-image)"));
}

TEST(OpenRouterProvider, TextIsAStringOrTextParts)
{
    auto plain = Parse(ImageAnswer("\"content\":\"  hello world \\n\""));
    AiTextResult text = OpenRouterProvider::ParseTextResponse(plain, "openai/gpt-5.5");
    ASSERT_TRUE(text.ok) << text.error;
    EXPECT_EQ(text.text, String("hello world"));

    auto parts = Parse(ImageAnswer("\"content\":[{\"type\":\"text\",\"text\":\"one \"},{\"type\":\"text\",\"text\":\"two\"}]"));
    AiTextResult joined = OpenRouterProvider::ParseTextResponse(parts, "openai/gpt-5.5");
    ASSERT_TRUE(joined.ok) << joined.error;
    EXPECT_EQ(joined.text, String("one two"));

    auto empty = Parse(ImageAnswer("\"content\":null"));
    EXPECT_FALSE(OpenRouterProvider::ParseTextResponse(empty, "openai/gpt-5.5").ok);
}

// A 200 answer that carries an error object is a failure
TEST(OpenRouterProvider, ErrorObjectInTheBodyFailsTheCall)
{
    auto doc = Parse("{\"error\":{\"code\":402,\"message\":\"Insufficient credits\",\"metadata\":{}}}");
    AiTextResult text = OpenRouterProvider::ParseTextResponse(doc, "openai/gpt-5.5");
    EXPECT_FALSE(text.ok);
    EXPECT_TRUE(text.error.Contains("Insufficient credits")) << text.error;
    AiBytesResult image = OpenRouterProvider::ParseImageResponse(doc, kModel);
    EXPECT_FALSE(image.ok);
    EXPECT_TRUE(image.error.Contains("Insufficient credits")) << image.error;
}

TEST(OpenRouterProvider, ImagesBodyHasNoSeedAndNoSize)
{
    DataDocument plain;
    OpenRouterProvider::BuildImagesBody(plain, "openai/gpt-image-1", "a coin", {}, false);
    EXPECT_EQ(String(plain["model"].GetString()), String("openai/gpt-image-1"));
    EXPECT_EQ(String(plain["prompt"].GetString()), String("a coin"));
    EXPECT_EQ((int)plain["n"], 1);
    for (auto key : { "input_references", "background", "output_format", "seed", "size", "quality", "modalities", "messages" })
        EXPECT_FALSE(plain.FindMember(key)) << key;

    DataDocument transparent;
    OpenRouterProvider::BuildImagesBody(transparent, "openai/gpt-5-image-mini", "  ", {}, true);
    EXPECT_EQ(String(transparent["prompt"].GetString()), String("Generate the image."));
    EXPECT_EQ(String(transparent["background"].GetString()), String("transparent"));
    EXPECT_EQ(String(transparent["output_format"].GetString()), String("png"));
    EXPECT_FALSE(transparent.FindMember("input_references"));
    EXPECT_FALSE(transparent.FindMember("seed"));

    DataDocument edit;
    OpenRouterProvider::BuildImagesBody(edit, "openai/gpt-image-1-mini", "make it gold", { { "", "raw" }, { "image/jpeg", "jpg" } }, true);
    auto& references = edit["input_references"];
    ASSERT_TRUE(references.IsArray());
    ASSERT_EQ(references.GetElementsCount(), 2);
    EXPECT_EQ(String(references[0]["type"].GetString()), String("image_url"));
    EXPECT_EQ(String(references[0]["image_url"]["url"].GetString()), PipelineUtils::BytesToDataUrl("raw", "image/png"));
    EXPECT_EQ(String(references[1]["image_url"]["url"].GetString()), PipelineUtils::BytesToDataUrl("jpg", "image/jpeg"));
    EXPECT_EQ(String(edit["background"].GetString()), String("transparent"));

    DataDocument opaqueEdit;
    OpenRouterProvider::BuildImagesBody(opaqueEdit, "openai/gpt-image-1-mini", "make it gold", { { "image/png", "raw" } }, false);
    EXPECT_EQ(opaqueEdit["input_references"].GetElementsCount(), 1);
    EXPECT_FALSE(opaqueEdit.FindMember("background"));
    EXPECT_FALSE(opaqueEdit.FindMember("output_format"));
}

TEST(OpenRouterProvider, ImagesAnswerCarriesTheBytesAndTheirType)
{
    String bytes = "\x89PNG-bytes";
    auto doc = Parse("{\"created\":1,\"data\":[{\"b64_json\":\"" + PipelineUtils::Base64Encode(bytes) +
                     "\",\"media_type\":\"image/webp\"}],\"usage\":{\"cost\":0.0022}}");
    AiBytesResult image = OpenRouterProvider::ParseImagesResponse(doc, "openai/gpt-5-image-mini");
    ASSERT_TRUE(image.ok) << image.error;
    EXPECT_EQ(image.data, bytes);
    EXPECT_EQ(image.mimeType, String("image/webp"));

    auto untyped = Parse("{\"data\":[{\"b64_json\":\"" + PipelineUtils::Base64Encode(bytes) + "\"}]}");
    AiBytesResult png = OpenRouterProvider::ParseImagesResponse(untyped, "openai/gpt-5-image-mini");
    ASSERT_TRUE(png.ok) << png.error;
    EXPECT_EQ(png.mimeType, String("image/png"));

    auto empty = Parse("{\"created\":1,\"data\":[]}");
    AiBytesResult none = OpenRouterProvider::ParseImagesResponse(empty, "openai/gpt-5-image-mini");
    EXPECT_FALSE(none.ok);
    EXPECT_EQ(none.error, String("OpenRouter returned no image data (model openai/gpt-5-image-mini)"));
}

// A validation failure has its own shape, and the message is read from it all the same
TEST(OpenRouterProvider, ErrorsOfBothShapesAreRead)
{
    auto zod = Parse("{\"success\":false,\"error\":{\"name\":\"ZodError\",\"message\":\"Invalid input: expected string\"}}");
    EXPECT_EQ(OpenRouterProvider::ParseError(zod), String("Invalid input: expected string"));
    AiBytesResult image = OpenRouterProvider::ParseImagesResponse(zod, "openai/gpt-image-1");
    EXPECT_FALSE(image.ok);
    EXPECT_EQ(image.error, String("OpenRouter image call failed (model openai/gpt-image-1): Invalid input: expected string"));

    auto refused = Parse("{\"error\":{\"message\":\"No provider for openai/gpt-5.4-image-2 supports the requested parameter(s): background\",\"code\":400}}");
    EXPECT_EQ(OpenRouterProvider::ParseError(refused),
              String("No provider for openai/gpt-5.4-image-2 supports the requested parameter(s): background (code 400)"));

    // The shared reader of a failed status sees the message of either shape
    AiHttpResult http;
    http.status = 400;
    http.error = "OpenRouter images call: HTTP 400";
    http.body = zod.SaveAsString();
    http.jsonParsed = http.json.LoadFromData(http.body);
    EXPECT_EQ(AiHttp::DescribeError(http, "OpenRouter images call", "openai/gpt-image-1"),
              String("OpenRouter images call: HTTP 400 (model openai/gpt-image-1): Invalid input: expected string"));
}

TEST(OpenRouterProvider, RequestBodyFollowsTheChatCompletionsShape)
{
    DataDocument text;
    OpenRouterProvider::BuildRequestBody(text, "openai/gpt-5.5", "hi", {}, false, 7);
    EXPECT_EQ(String(text["model"].GetString()), String("openai/gpt-5.5"));
    EXPECT_TRUE(text["messages"][0]["content"].IsString());
    EXPECT_EQ(String(text["messages"][0]["role"].GetString()), String("user"));
    EXPECT_FALSE(text.FindMember("modalities"));
    EXPECT_FALSE(text.FindMember("seed"));
    EXPECT_TRUE((bool)text["usage"]["include"]);

    DataDocument image;
    OpenRouterProvider::BuildRequestBody(image, kModel, "draw", { { "", "raw" } }, true, 7);
    auto& parts = image["messages"][0]["content"];
    ASSERT_TRUE(parts.IsArray());
    ASSERT_EQ(parts.GetElementsCount(), 2);
    EXPECT_EQ(String(parts[0]["type"].GetString()), String("text"));
    EXPECT_EQ(String(parts[1]["image_url"]["url"].GetString()), PipelineUtils::BytesToDataUrl("raw", "image/png"));
    EXPECT_EQ(String(image["modalities"][0].GetString()), String("image"));
    EXPECT_EQ((int)image["seed"], 7);

    DataDocument random;
    OpenRouterProvider::BuildRequestBody(random, kModel, "draw", {}, true, -1);
    EXPECT_FALSE(random.FindMember("seed"));
}
