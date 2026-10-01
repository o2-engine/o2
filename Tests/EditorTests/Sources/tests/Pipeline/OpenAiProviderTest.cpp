#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/OpenAiProvider.h"

using namespace o2;
using namespace Editor;

namespace
{
    DataDocument Parse(const String& json)
    {
        DataDocument doc;
        EXPECT_TRUE(doc.LoadFromData(json)) << json;
        return doc;
    }

    String Answer(const String& message)
    {
        return "{\"id\":\"chatcmpl-1\",\"choices\":[{\"finish_reason\":\"stop\",\"message\":{\"role\":\"assistant\"," + message + "}}]}";
    }

    // Bytes with a zero, a line break and a boundary-like run inside: a form must carry them as they are
    String BinaryBytes()
    {
        String bytes = "\x89PNG";
        bytes += '\0';
        bytes += "\r\n--not-a-boundary\r\n";
        bytes += (char)0xff;
        bytes += "tail";
        return bytes;
    }
}

TEST(OpenAiProvider, ChatBodyCarriesThePromptAndTheImages)
{
    DataDocument text;
    OpenAiProvider::BuildChatBody(text, "gpt-5.5", "hi", {});
    EXPECT_EQ(String(text["model"].GetString()), String("gpt-5.5"));
    EXPECT_EQ(String(text["messages"][0]["role"].GetString()), String("user"));
    ASSERT_TRUE(text["messages"][0]["content"].IsString());
    EXPECT_EQ(String(text["messages"][0]["content"].GetString()), String("hi"));
    EXPECT_FALSE(text.FindMember("seed"));
    EXPECT_FALSE(text.FindMember("modalities"));

    DataDocument vision;
    OpenAiProvider::BuildChatBody(vision, "gpt-5.5", "look", { { "", "raw" }, { "image/jpeg", "jpg" } });
    auto& parts = vision["messages"][0]["content"];
    ASSERT_TRUE(parts.IsArray());
    ASSERT_EQ(parts.GetElementsCount(), 3);
    EXPECT_EQ(String(parts[0]["type"].GetString()), String("text"));
    EXPECT_EQ(String(parts[0]["text"].GetString()), String("look"));
    EXPECT_EQ(String(parts[1]["type"].GetString()), String("image_url"));
    EXPECT_EQ(String(parts[1]["image_url"]["url"].GetString()), PipelineUtils::BytesToDataUrl("raw", "image/png"));
    EXPECT_EQ(String(parts[2]["image_url"]["url"].GetString()), PipelineUtils::BytesToDataUrl("jpg", "image/jpeg"));
}

TEST(OpenAiProvider, GenerationsBodyAsksForOnePngImage)
{
    DataDocument body;
    OpenAiProvider::BuildGenerationsBody(body, "gpt-image-1", "a coin");
    EXPECT_EQ(String(body["model"].GetString()), String("gpt-image-1"));
    EXPECT_EQ(String(body["prompt"].GetString()), String("a coin"));
    EXPECT_EQ((int)body["n"], 1);
    EXPECT_EQ(String(body["size"].GetString()), String("1024x1024"));
    EXPECT_EQ(String(body["output_format"].GetString()), String("png"));
    EXPECT_FALSE(body.FindMember("response_format"));
    EXPECT_FALSE(body.FindMember("seed"));

    DataDocument dalle;
    OpenAiProvider::BuildGenerationsBody(dalle, "dall-e-3", "");
    EXPECT_EQ(String(dalle["prompt"].GetString()), String("Generate the image."));
    EXPECT_EQ(String(dalle["response_format"].GetString()), String("b64_json"));
    EXPECT_FALSE(dalle.FindMember("output_format"));
}

TEST(OpenAiProvider, EditsFormCarriesTheFieldsAndTheReferences)
{
    String bytes = BinaryBytes();
    auto form = OpenAiProvider::BuildEditsBody("gpt-image-1", "make it gold", { { "", bytes }, { "image/jpeg", "second" } }, "test-boundary");
    EXPECT_EQ(form.boundary, String("test-boundary"));
    EXPECT_EQ(form.contentType, String("multipart/form-data; boundary=test-boundary"));

    String expected;
    expected += "--test-boundary\r\nContent-Disposition: form-data; name=\"model\"\r\n\r\ngpt-image-1\r\n";
    expected += "--test-boundary\r\nContent-Disposition: form-data; name=\"prompt\"\r\n\r\nmake it gold\r\n";
    expected += "--test-boundary\r\nContent-Disposition: form-data; name=\"n\"\r\n\r\n1\r\n";
    expected += "--test-boundary\r\nContent-Disposition: form-data; name=\"size\"\r\n\r\n1024x1024\r\n";
    expected += "--test-boundary\r\nContent-Disposition: form-data; name=\"image[]\"; filename=\"ref0.png\"\r\nContent-Type: image/png\r\n\r\n";
    expected += bytes;
    expected += "\r\n";
    expected += "--test-boundary\r\nContent-Disposition: form-data; name=\"image[]\"; filename=\"ref1.png\"\r\nContent-Type: image/jpeg\r\n\r\nsecond\r\n";
    expected += "--test-boundary--\r\n";
    ASSERT_EQ(form.body.Length(), expected.Length());
    EXPECT_TRUE(form.body == expected);
    EXPECT_GT(form.body.Length(), (int)strlen(form.body.Data())) << "the zero byte of the image is inside the body";
}

// A transparent background is asked from the model itself, in both image requests
TEST(OpenAiProvider, TransparentBackgroundIsAFieldOfTheImageRequests)
{
    DataDocument plain;
    OpenAiProvider::BuildGenerationsBody(plain, "gpt-image-1", "a coin");
    EXPECT_FALSE(plain.FindMember("background"));

    DataDocument transparent;
    OpenAiProvider::BuildGenerationsBody(transparent, "gpt-image-1", "a coin", true);
    EXPECT_EQ(String(transparent["background"].GetString()), String("transparent"));
    EXPECT_EQ(String(transparent["output_format"].GetString()), String("png"));
    EXPECT_EQ(String(transparent["prompt"].GetString()), String("a coin"));

    auto opaque = OpenAiProvider::BuildEditsBody("gpt-image-1", "p", { { "image/png", "bytes" } }, "b");
    EXPECT_FALSE(opaque.body.Contains("name=\"background\""));
    EXPECT_FALSE(opaque.body.Contains("name=\"output_format\""));

    auto form = OpenAiProvider::BuildEditsBody("gpt-image-1", "p", { { "image/png", "bytes" } }, "b", true);
    String expected;
    expected += "--b\r\nContent-Disposition: form-data; name=\"model\"\r\n\r\ngpt-image-1\r\n";
    expected += "--b\r\nContent-Disposition: form-data; name=\"prompt\"\r\n\r\np\r\n";
    expected += "--b\r\nContent-Disposition: form-data; name=\"n\"\r\n\r\n1\r\n";
    expected += "--b\r\nContent-Disposition: form-data; name=\"size\"\r\n\r\n1024x1024\r\n";
    expected += "--b\r\nContent-Disposition: form-data; name=\"background\"\r\n\r\ntransparent\r\n";
    expected += "--b\r\nContent-Disposition: form-data; name=\"output_format\"\r\n\r\npng\r\n";
    expected += "--b\r\nContent-Disposition: form-data; name=\"image[]\"; filename=\"ref0.png\"\r\nContent-Type: image/png\r\n\r\nbytes\r\n";
    expected += "--b--\r\n";
    EXPECT_TRUE(form.body == expected) << form.body;
}

TEST(OpenAiProvider, EditsFormGetsAFreshBoundaryThatTheBodyUses)
{
    auto first = OpenAiProvider::BuildEditsBody("gpt-image-1", "", { { "image/png", "bytes" } });
    auto second = OpenAiProvider::BuildEditsBody("gpt-image-1", "", { { "image/png", "bytes" } });
    EXPECT_GE(first.boundary.Length(), 24);
    EXPECT_NE(first.boundary, second.boundary);
    EXPECT_EQ(first.contentType, "multipart/form-data; boundary=" + first.boundary);
    EXPECT_TRUE(first.body.StartsWith("--" + first.boundary + "\r\n"));
    EXPECT_TRUE(first.body.EndsWith("--" + first.boundary + "--\r\n"));
    EXPECT_TRUE(first.body.Contains("name=\"prompt\"\r\n\r\nGenerate the image.\r\n"));
}

TEST(OpenAiProvider, TextIsAStringOrTextParts)
{
    auto plain = Parse(Answer("\"content\":\"  hello world \\n\""));
    AiTextResult text = OpenAiProvider::ParseTextResponse(plain, "gpt-5.5");
    ASSERT_TRUE(text.ok) << text.error;
    EXPECT_EQ(text.text, String("hello world"));

    auto parts = Parse(Answer("\"content\":[{\"type\":\"text\",\"text\":\"one \"},{\"type\":\"text\",\"text\":\"two\"}]"));
    AiTextResult joined = OpenAiProvider::ParseTextResponse(parts, "gpt-5.5");
    ASSERT_TRUE(joined.ok) << joined.error;
    EXPECT_EQ(joined.text, String("one two"));

    auto empty = Parse(Answer("\"content\":null"));
    AiTextResult none = OpenAiProvider::ParseTextResponse(empty, "gpt-5.5");
    EXPECT_FALSE(none.ok);
    EXPECT_TRUE(none.error.StartsWith("OpenAI returned no text (model gpt-5.5)")) << none.error;
}

TEST(OpenAiProvider, ImageComesAsBase64OrAsAnAddress)
{
    String bytes = BinaryBytes();
    String url;
    auto doc = Parse("{\"created\":1,\"data\":[{\"b64_json\":\"" + PipelineUtils::Base64Encode(bytes) + "\"}]}");
    AiBytesResult image = OpenAiProvider::ParseImageResponse(doc, "gpt-image-1", url);
    ASSERT_TRUE(image.ok) << image.error;
    EXPECT_TRUE(image.data == bytes);
    EXPECT_EQ(image.mimeType, String("image/png"));
    EXPECT_TRUE(url.IsEmpty());

    auto linked = Parse("{\"created\":1,\"data\":[{\"url\":\"https://files.example/result.png\"}]}");
    AiBytesResult pending = OpenAiProvider::ParseImageResponse(linked, "dall-e-3", url);
    EXPECT_FALSE(pending.ok);
    EXPECT_TRUE(pending.error.IsEmpty()) << pending.error;
    EXPECT_EQ(url, String("https://files.example/result.png"));

    auto empty = Parse("{\"created\":1,\"data\":[]}");
    AiBytesResult none = OpenAiProvider::ParseImageResponse(empty, "gpt-image-1", url);
    EXPECT_FALSE(none.ok);
    EXPECT_EQ(none.error, String("OpenAI returned no image data (model gpt-image-1)"));
    EXPECT_TRUE(url.IsEmpty());
}

TEST(OpenAiProvider, ErrorsAreReadFromTheBody)
{
    auto quota = Parse("{\"error\":{\"message\":\"You exceeded your current quota\",\"type\":\"insufficient_quota\",\"param\":null,\"code\":\"insufficient_quota\"}}");
    EXPECT_EQ(OpenAiProvider::ParseError(quota), String("You exceeded your current quota (code insufficient_quota)"));
    EXPECT_TRUE(OpenAiProvider::IsQuotaError(quota));
    EXPECT_TRUE(OpenAiProvider::StatusHint(429, quota).Contains("no credits"));
    EXPECT_TRUE(AiHttp::IsFinalError(429, quota.SaveAsString()));

    auto rate = Parse("{\"error\":{\"message\":\"Rate limit reached\",\"type\":\"requests\",\"code\":\"rate_limit_exceeded\"}}");
    EXPECT_FALSE(OpenAiProvider::IsQuotaError(rate));
    EXPECT_TRUE(OpenAiProvider::StatusHint(429, rate).IsEmpty());
    EXPECT_FALSE(AiHttp::IsFinalError(429, rate.SaveAsString()));
    EXPECT_FALSE(AiHttp::IsFinalError(500, quota.SaveAsString()));

    EXPECT_TRUE(OpenAiProvider::StatusHint(401, rate).Contains("key was rejected"));
    EXPECT_TRUE(OpenAiProvider::StatusHint(404, rate).Contains("no such model"));
    EXPECT_TRUE(OpenAiProvider::ParseError(Parse("{\"data\":[]}")).IsEmpty());

    String url;
    AiTextResult text = OpenAiProvider::ParseTextResponse(quota, "gpt-5.5");
    EXPECT_FALSE(text.ok);
    EXPECT_TRUE(text.error.Contains("You exceeded your current quota")) << text.error;
    AiBytesResult image = OpenAiProvider::ParseImageResponse(quota, "gpt-image-1", url);
    EXPECT_FALSE(image.ok);
    EXPECT_TRUE(image.error.Contains("You exceeded your current quota")) << image.error;
}

TEST(OpenAiProvider, RawPostKeepsTheBodyAndItsContentType)
{
    auto form = OpenAiProvider::BuildEditsBody("gpt-image-1", "p", { { "image/png", BinaryBytes() } }, "b");
    Map<String, String> headers;
    headers["Authorization"] = "Bearer test";
    auto request = AiHttp::MakeRawPost("https://api.openai.com/v1/images/edits", form.body, form.contentType, headers, 300.0f);
    EXPECT_TRUE(request->body == form.body);
    EXPECT_EQ(request->body.Length(), form.body.Length());
    EXPECT_EQ(request->headers["Content-Type"], String("multipart/form-data; boundary=b"));
    EXPECT_EQ(request->headers["Authorization"], String("Bearer test"));
    EXPECT_EQ(request->method, HttpMethod::Post);
    EXPECT_FALSE(request->useCookies);
}
