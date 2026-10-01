#include "o2Editor/stdafx.h"
#include "PipelineModelMenu.h"

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor::PipelineModelMenu
{
    static const Vector<PipelineModelProvider> providerOrder = {
        PipelineModelProvider::Google, PipelineModelProvider::OpenAi, PipelineModelProvider::Kling,
        PipelineModelProvider::ElevenLabs, PipelineModelProvider::OpenRouter
    };

    // OpenRouter vendors listed first, in this order; the rest follow A to Z by label
    static const Vector<String> vendorOrder = { "openai", "anthropic", "google", "x-ai", "deepseek", "meta-llama", "mistralai", "qwen" };

    static const String currentKey = "current";
    static const String recentKey = "recent";
    static const String openRouterPrefix = "openrouter:";

    String KindName(PipelineModelKind kind)
    {
        switch (kind)
        {
            case PipelineModelKind::Image: return "image";
            case PipelineModelKind::Video: return "video";
            case PipelineModelKind::Tts: return "tts";
            case PipelineModelKind::ElevenLabs: return "elevenlabs";
            default: return "text";
        }
    }

    String ProviderKey(PipelineModelProvider provider)
    {
        switch (provider)
        {
            case PipelineModelProvider::OpenAi: return "openai";
            case PipelineModelProvider::Kling: return "kling";
            case PipelineModelProvider::ElevenLabs: return "elevenlabs";
            case PipelineModelProvider::OpenRouter: return "openrouter";
            default: return "google";
        }
    }

    String ProviderLabel(PipelineModelProvider provider)
    {
        switch (provider)
        {
            case PipelineModelProvider::OpenAi: return "OpenAI";
            case PipelineModelProvider::Kling: return "Kling";
            case PipelineModelProvider::ElevenLabs: return "ElevenLabs";
            case PipelineModelProvider::OpenRouter: return "OpenRouter";
            default: return "Google Gemini";
        }
    }

    String ChipLabel(PipelineModelProvider provider)
    {
        return provider == PipelineModelProvider::Google ? String("Google") : ProviderLabel(provider);
    }

    String BareId(const String& id)
    {
        String bare = id.Trimed(" \n\r\t");
        return bare.StartsWith("models/") ? bare.SubStr(7) : bare;
    }

    String VendorOf(const String& id)
    {
        String bare = BareId(id);
        int slash = bare.Find("/");
        String vendor = slash < 0 ? bare : bare.SubStr(0, slash);
        return vendor.StartsWith("~") ? vendor.SubStr(1) : vendor;
    }

    String VendorLabel(const String& vendor)
    {
        static const Map<String, String> labels = {
            { "openai", "OpenAI" }, { "anthropic", "Anthropic" }, { "google", "Google" }, { "x-ai", "xAI" },
            { "meta-llama", "Meta" }, { "meta", "Meta" }, { "mistralai", "Mistral" }, { "deepseek", "DeepSeek" },
            { "qwen", "Qwen" }, { "z-ai", "Z.ai" }, { "moonshotai", "Moonshot" }, { "minimax", "MiniMax" },
            { "cohere", "Cohere" }, { "perplexity", "Perplexity" }, { "amazon", "Amazon" }, { "nvidia", "NVIDIA" },
            { "microsoft", "Microsoft" }, { "openrouter", "OpenRouter" }, { "black-forest-labs", "Black Forest Labs" },
            { "bytedance-seed", "ByteDance Seed" }, { "sourceful", "Sourceful" }, { "recraft", "Recraft" }
        };

        String label;
        if (labels.TryGetValue(vendor, label))
            return label;

        String result;
        for (auto& part : vendor.Split("-"))
        {
            if (!result.IsEmpty())
                result += " ";

            if (!part.IsEmpty())
            {
                String first;
                first += (char)toupper((unsigned char)part[0]);
                result += first + part.SubStr(1);
            }
        }
        return result;
    }

    PipelineModelGroupKey GroupOf(const String& id)
    {
        String bare = BareId(id);
        if (bare.Contains("/"))
        {
            String vendor = VendorOf(bare);
            return { openRouterPrefix + vendor, "OpenRouter \xC2\xB7 " + VendorLabel(vendor), PipelineModelProvider::OpenRouter };
        }

        String lower = bare.ToLowerCase();
        PipelineModelProvider provider = PipelineModelProvider::Google;
        if (PipelineUtils::IsOpenAiModelId(bare))
            provider = PipelineModelProvider::OpenAi;
        else if (lower.StartsWith("kling"))
            provider = PipelineModelProvider::Kling;
        else if (lower.StartsWith("eleven_"))
            provider = PipelineModelProvider::ElevenLabs;

        return { ProviderKey(provider), ProviderLabel(provider), provider };
    }

    static String WithoutSuffix(const String& text, const String& suffix)
    {
        return text.EndsWith(suffix) ? text.SubStr(0, text.Length() - suffix.Length()) : text;
    }

    String RowName(const String& id)
    {
        String bare = BareId(id);
        String pretty = PipelineUtils::PrettyModelName(bare);
        if (bare.Contains("/"))
        {
            // "Anthropic: Claude Sonnet 5.5 · OpenRouter": the group header names both the API and the vendor
            String name = WithoutSuffix(pretty, " \xC2\xB7 OpenRouter");
            int colon = name.Find(": ");
            return colon >= 0 && colon <= 40 ? name.SubStr(colon + 2) : name;
        }

        if (PipelineUtils::IsOpenAiModelId(bare))
            return WithoutSuffix(pretty, " \xC2\xB7 OpenAI");

        return pretty;
    }

    bool IsAlpha(PipelineModelKind kind, const String& id)
    {
        return kind == PipelineModelKind::Image && PipelineTransparency::SupportsNativeTransparency(id);
    }

    static PipelineMenuModel MakeRow(const String& id, PipelineModelKind kind, bool standalone)
    {
        auto group = GroupOf(id);
        String name = RowName(id);
        String full = PipelineUtils::PrettyModelName(id);

        PipelineMenuModel row;
        row.id = id;
        row.name = standalone ? full : name;
        row.full = full;
        row.alpha = IsAlpha(kind, id);
        row.provider = group.provider;
        row.fields = { name, full, id, group.label };
        return row;
    }

    // Sorting rank of a group key: Current, Recent, the provider groups, then the OpenRouter vendors
    static void RankOf(const PipelineMenuGroup& group, int& tier, int& order, String& label)
    {
        tier = 0; order = 0; label = "";
        if (group.key == currentKey)
            return;

        if (group.key == recentKey)
        {
            tier = 1;
            return;
        }

        if (!group.key.StartsWith(openRouterPrefix))
        {
            tier = 2;
            order = providerOrder.IndexOf(group.provider);
            return;
        }

        String vendor = group.key.SubStr(openRouterPrefix.Length());
        int index = vendorOrder.IndexOf(vendor);
        tier = 3;
        order = index < 0 ? vendorOrder.Count() : index;
        label = VendorLabel(vendor).ToLowerCase();
    }

    Vector<PipelineMenuGroup> BuildGroups(const Vector<String>& ids, PipelineModelKind kind, const String& current,
                                          const Vector<String>& recent)
    {
        Vector<String> offered;
        for (auto& id : ids)
        {
            if (!id.IsEmpty() && !offered.Contains(id))
                offered.Add(id);
        }

        Vector<PipelineMenuGroup> groups;
        for (auto& id : offered)
        {
            auto key = GroupOf(id);
            int index = groups.IndexOf([&](const PipelineMenuGroup& g) { return g.key == key.key; });
            if (index < 0)
            {
                PipelineMenuGroup created;
                created.key = key.key;
                created.label = key.label;
                created.provider = key.provider;
                groups.Add(created);
                index = groups.Count() - 1;
            }
            groups[index].models.Add(MakeRow(id, kind, false));
        }

        if (!current.IsEmpty() && !offered.Contains(current))
        {
            PipelineMenuGroup group;
            group.key = currentKey;
            group.label = "Current";
            group.mixed = true;
            group.models.Add(MakeRow(current, kind, true));
            groups.Add(group);
        }

        // Only what is still offered: a current value outside the list has its own group, listed twice it would
        // read as two different models
        Vector<String> recentIds;
        for (auto& id : recent)
        {
            if (recentIds.Count() < recentMax && offered.Contains(id) && !recentIds.Contains(id))
                recentIds.Add(id);
        }

        if (!recentIds.IsEmpty())
        {
            PipelineMenuGroup group;
            group.key = recentKey;
            group.label = "Recent";
            group.mixed = true;
            for (auto& id : recentIds)
                group.models.Add(MakeRow(id, kind, true));
            groups.Add(group);
        }

        // Stable: groups of an equal rank keep the order they were met in
        Vector<Pair<int, PipelineMenuGroup>> indexed;
        for (int i = 0; i < groups.Count(); i++)
            indexed.Add(Pair<int, PipelineMenuGroup>(i, groups[i]));

        indexed.Sort([](const Pair<int, PipelineMenuGroup>& a, const Pair<int, PipelineMenuGroup>& b)
        {
            int aTier, aOrder, bTier, bOrder;
            String aLabel, bLabel;
            RankOf(a.second, aTier, aOrder, aLabel);
            RankOf(b.second, bTier, bOrder, bLabel);
            if (aTier != bTier) return aTier < bTier;
            if (aOrder != bOrder) return aOrder < bOrder;
            int byLabel = strcmp(aLabel.Data(), bLabel.Data());
            if (byLabel != 0) return byLabel < 0;
            return a.first < b.first;
        });

        Vector<PipelineMenuGroup> sorted;
        for (auto& item : indexed)
            sorted.Add(item.second);

        return sorted;
    }

    bool FoldedByDefault(const PipelineMenuGroup& group, const String& current)
    {
        return !group.mixed && group.provider == PipelineModelProvider::OpenRouter &&
            !group.models.Any([&](const PipelineMenuModel& m) { return m.id == current; });
    }

    static String Squash(const String& text)
    {
        String result;
        for (int i = 0; i < text.Length(); i++)
        {
            char c = (char)tolower((unsigned char)text[i]);
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
                result += c;
        }
        return result;
    }

    static Vector<String> Tokens(const String& text)
    {
        Vector<String> tokens;
        String token;
        for (int i = 0; i < text.Length(); i++)
        {
            char c = text[i];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            {
                if (!token.IsEmpty())
                    tokens.Add(token);
                token = "";
            }
            else
                token += c;
        }

        if (!token.IsEmpty())
            tokens.Add(token);

        return tokens;
    }

    bool Matches(const String& query, const Vector<String>& fields)
    {
        auto tokens = Tokens(query.ToLowerCase());
        if (tokens.IsEmpty())
            return true;

        Vector<String> plain, squashed;
        for (auto& field : fields)
        {
            plain.Add(field.ToLowerCase());
            squashed.Add(Squash(field));
        }

        return tokens.All([&](const String& token)
        {
            String squashedToken = Squash(token);
            return plain.Any([&](const String& field) { return field.Contains(token); }) ||
                (!squashedToken.IsEmpty() && squashed.Any([&](const String& field) { return field.Contains(squashedToken); }));
        });
    }

    Vector<PipelineVisibleGroup> VisibleGroups(const Vector<PipelineMenuGroup>& groups, const PipelineMenuFilter& filter,
                                               const String& current, const Map<String, bool>& toggled)
    {
        bool searching = !filter.query.Trimed(" \n\r\t").IsEmpty() || filter.alphaOnly;
        Vector<PipelineVisibleGroup> result;
        for (auto& group : groups)
        {
            PipelineVisibleGroup visible;
            for (auto& model : group.models)
            {
                if ((filter.anyProvider || model.provider == filter.provider) && (!filter.alphaOnly || model.alpha) &&
                    Matches(filter.query, model.fields))
                {
                    visible.shown.Add(model);
                }
            }

            if (visible.shown.IsEmpty())
                continue;

            visible.group = group;
            bool manual = false;
            if (searching)
                visible.folded = false;
            else if (toggled.TryGetValue(group.key, manual))
                visible.folded = manual;
            else
                visible.folded = FoldedByDefault(group, current);

            result.Add(visible);
        }
        return result;
    }

    Vector<PipelineModelProvider> ProvidersOf(const Vector<PipelineMenuGroup>& groups)
    {
        Vector<PipelineModelProvider> result;
        for (auto provider : providerOrder)
        {
            bool present = groups.Any([&](const PipelineMenuGroup& g)
            {
                return g.models.Any([&](const PipelineMenuModel& m) { return m.provider == provider; });
            });

            if (present)
                result.Add(provider);
        }
        return result;
    }

    bool HasAlpha(const Vector<PipelineMenuGroup>& groups)
    {
        return groups.Any([](const PipelineMenuGroup& g) { return g.models.Any([](const PipelineMenuModel& m) { return m.alpha; }); });
    }

    String CustomIdFor(const String& query, const Vector<PipelineMenuGroup>& groups)
    {
        String id = query.Trimed(" \n\r\t");
        if (id.IsEmpty() || id.Contains(" ") || id.Contains("\t") || id.Contains("\n") || id.Contains("\r"))
            return "";

        bool listed = groups.Any([&](const PipelineMenuGroup& g)
        {
            return g.models.Any([&](const PipelineMenuModel& m) { return m.id == id; });
        });

        return listed ? String() : id;
    }

    Vector<String> PickableIds(const Vector<PipelineVisibleGroup>& groups, const String& customId)
    {
        Vector<String> ids;
        for (auto& group : groups)
        {
            if (group.folded)
                continue;

            for (auto& model : group.shown)
                ids.Add(model.id);
        }

        if (!customId.IsEmpty())
            ids.Add(customId);

        return ids;
    }

    int InitialHighlight(const Vector<String>& pickable, const String& current)
    {
        int index = pickable.IndexOf(current);
        return index < 0 ? 0 : index;
    }

    int HighlightFor(const String& query, const Vector<String>& modelIds)
    {
        int index = modelIds.IndexOf(query.Trimed(" \n\r\t"));
        return index < 0 ? 0 : index;
    }

    Vector<String> PushRecent(const Vector<String>& list, const String& id, int max /*= recentMax*/)
    {
        Vector<String> result = { id };
        for (auto& item : list)
        {
            if (result.Count() >= max)
                break;

            if (item != id)
                result.Add(item);
        }
        return result;
    }

    String GetRecentPath()
    {
        return PipelineUtils::GetWorkPath() + "RecentModels.json";
    }

    Vector<String> LoadRecent(PipelineModelKind kind)
    {
        Vector<String> result;
        DataDocument doc;
        if (!o2FileSystem.IsFileExist(GetRecentPath()) || !doc.LoadFromFile(GetRecentPath()) || !doc.IsObject())
            return result;

        auto list = doc.FindMember(KindName(kind).Data());
        if (!list || !list->IsArray())
            return result;

        for (auto& item : *list)
        {
            if (item.IsString())
                result.Add(item.GetString());
        }
        return result;
    }

    void SaveRecent(PipelineModelKind kind, const Vector<String>& list)
    {
        DataDocument doc;
        if (!o2FileSystem.IsFileExist(GetRecentPath()) || !doc.LoadFromFile(GetRecentPath()) || !doc.IsObject())
            doc.SetObject();

        // SetArray keeps the elements of a value that already is an array
        String key = KindName(kind);
        if (doc.FindMember(key.Data()))
            doc.RemoveMember(key.Data());

        auto& array = doc[key.Data()];
        array.SetArray();
        for (auto& id : list)
            array.AddElement() = id;

        o2FileSystem.FolderCreate(PipelineUtils::GetWorkPath(), true);
        doc.SaveToFile(GetRecentPath());
    }

    void RememberPick(PipelineModelKind kind, const String& id)
    {
        if (!id.IsEmpty())
            SaveRecent(kind, PushRecent(LoadRecent(kind), id));
    }
}
// --- META ---

ENUM_META(Editor::PipelineModelKind, Editor__PipelineModelKind)
{
    ENUM_ENTRY(ElevenLabs);
    ENUM_ENTRY(Image);
    ENUM_ENTRY(Text);
    ENUM_ENTRY(Tts);
    ENUM_ENTRY(Video);
}
END_ENUM_META;

ENUM_META(Editor::PipelineModelProvider, Editor__PipelineModelProvider)
{
    ENUM_ENTRY(ElevenLabs);
    ENUM_ENTRY(Google);
    ENUM_ENTRY(Kling);
    ENUM_ENTRY(OpenAi);
    ENUM_ENTRY(OpenRouter);
}
END_ENUM_META;
// --- END META ---
