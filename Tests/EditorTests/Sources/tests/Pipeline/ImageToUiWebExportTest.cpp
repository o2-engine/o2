#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include <functional>
#include <regex>

#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorCreationMode.h"
#include "o2/Scene/ActorRefResolver.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

using namespace o2;
using namespace Editor;

// The AI image to UI prototype the AssetsLine web editor builds (shared/uiExport/o2.ts) reads back like the o2 editor's
// own export of the same layout. Gated: O2_UI_EXPORT_PROTO (the web's .proto), O2_UI_EXPORT_GOLDEN (the editor's)

namespace
{
    // Asset references pointed at ids and paths no asset has: a headless test must not load images or fonts
    std::string Detached(const std::string& text)
    {
        std::string s = std::regex_replace(text, std::regex("\"path\": \"([^\"]*)\""), "\"path\": \"__detached__/$1\"");
        return std::regex_replace(s, std::regex("\"(id|mFontAssetId)\": \"[0-9a-f]{32}\""), "\"$1\": \"00000000000000000000000000000001\"");
    }

    std::string WithoutIds(const std::string& text)
    {
        return std::regex_replace(text, std::regex("\"(Id|mUID)\": [0-9]+"), "\"$1\": 0");
    }

    Ref<Actor> Load(const String& path)
    {
        DataDocument doc;
        String text = PipelineUtils::ReadFileBytes(path);
        if (text.IsEmpty() || !doc.LoadFromData(String(Detached(text.Data()).c_str())))
            return nullptr;

        Ref<Actor> actor;
        doc["mActor"].Get(actor);
        ActorRefResolver::ResolveRefs(true);
        return actor;
    }

    std::string Dump(const Ref<Actor>& actor)
    {
        DataDocument out;
        out["mActor"] = actor;
        return WithoutIds(out.SaveAsString().Data());
    }

    int CountActors(const Ref<Actor>& actor)
    {
        int n = 1;
        for (auto& child : actor->GetChildren())
            n += CountActors(child);
        return n;
    }
}

TEST(ImageToUiWebExport, ReadsBackLikeTheEditorsOwnPrototype)
{
    const char* web = getenv("O2_UI_EXPORT_PROTO");
    const char* golden = getenv("O2_UI_EXPORT_GOLDEN");
    if (!web || !golden)
        GTEST_SKIP() << "set O2_UI_EXPORT_PROTO and O2_UI_EXPORT_GOLDEN";

    PushEditorScopeOnStack scope;
    auto prevMode = Actor::GetDefaultCreationMode();
    Actor::SetDefaultCreationMode(ActorCreateMode::NotInScene);

    auto ours = Load(web);
    auto theirs = Load(golden);
    ASSERT_TRUE(ours) << web;
    ASSERT_TRUE(theirs) << golden;
    EXPECT_EQ(ours->GetType().GetName(), theirs->GetType().GetName());
    EXPECT_EQ(CountActors(ours), CountActors(theirs));
    printf("web prototype: %s, %d actors\n", ours->GetType().GetName().Data(), CountActors(ours));

    std::string a = Dump(ours), b = Dump(theirs);
    EXPECT_GT(a.size(), 1000u);
    if (a != b)
    {
        size_t i = 0;
        while (i < a.size() && i < b.size() && a[i] == b[i])
            i++;
        size_t from = i > 200 ? i - 200 : 0;
        ADD_FAILURE() << "first difference at " << i << ":\nweb:    " << a.substr(from, 400) << "\neditor: " << b.substr(from, 400);
    }

    Actor::SetDefaultCreationMode(prevMode);
}

// o2 writes the web prototype back field for field: the same members, defaults left out the same way, the same numbers.
// Gated: O2_UI_EXPORT_ROUNDTRIP (a web .proto), O2_UI_EXPORT_DUMP (where to write what o2 makes of it)
TEST(ImageToUiWebExport, WritesBackWhatItReads)
{
    const char* in = getenv("O2_UI_EXPORT_ROUNDTRIP");
    const char* out = getenv("O2_UI_EXPORT_DUMP");
    if (!in || !out)
        GTEST_SKIP() << "set O2_UI_EXPORT_ROUNDTRIP and O2_UI_EXPORT_DUMP";

    PushEditorScopeOnStack scope;
    auto prevMode = Actor::GetDefaultCreationMode();
    Actor::SetDefaultCreationMode(ActorCreateMode::NotInScene);

    auto actor = Load(in);
    ASSERT_TRUE(actor) << in;
    DataDocument doc;
    doc["mActor"] = actor;
    ASSERT_TRUE(PipelineUtils::WriteFileBytes(out, doc.SaveAsString()));

    Actor::SetDefaultCreationMode(prevMode);
}
