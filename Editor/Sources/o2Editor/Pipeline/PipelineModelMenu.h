#pragma once

#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // List a model field belongs to; also the bucket its Recent list is kept in
    enum class PipelineModelKind { Text, Image, Video, Tts, ElevenLabs };

    // API a model runs on as far as its id tells; the order is the order of the provider chips
    enum class PipelineModelProvider { Google, OpenAi, Kling, ElevenLabs, OpenRouter };

    // Group a model id belongs to
    struct PipelineModelGroupKey
    {
        String                key;      // Provider key, or "openrouter:<vendor>"
        String                label;    // Header text
        PipelineModelProvider provider = PipelineModelProvider::Google; // API of the group
    };

    // One model line of the model menu
    struct PipelineMenuModel
    {
        String                id;            // Model id as the node config stores it
        String                name;          // Name the row shows: without the provider part in its group, whole in Recent and Current
        String                full;          // Name that stands on its own, the tooltip
        bool                  alpha = false; // Renders a transparent background itself; image lists only
        PipelineModelProvider provider = PipelineModelProvider::Google; // API the id belongs to
        Vector<String>        fields;        // What the search matches, each on its own: row name, full name, id, group label

        // Rows are equal when every field is
        bool operator==(const PipelineMenuModel& other) const
        {
            return id == other.id && name == other.name && full == other.full && alpha == other.alpha &&
                provider == other.provider && fields == other.fields;
        }
    };

    // Group of the model menu
    struct PipelineMenuGroup
    {
        String                    key;           // "current", "recent", a provider key or "openrouter:<vendor>"
        String                    label;         // Header text
        bool                      mixed = false; // Recent and Current hold rows of several providers
        PipelineModelProvider     provider = PipelineModelProvider::Google; // API of the rows unless mixed
        Vector<PipelineMenuModel> models;        // Rows in the order of the source list

        // Groups are equal when every field is
        bool operator==(const PipelineMenuGroup& other) const
        {
            return key == other.key && label == other.label && mixed == other.mixed && provider == other.provider && models == other.models;
        }
    };

    // What the chips and the search keep
    struct PipelineMenuFilter
    {
        String                query;              // Search text
        bool                  anyProvider = true; // False while a provider chip is on
        PipelineModelProvider provider = PipelineModelProvider::Google; // Provider of the chip that is on
        bool                  alphaOnly = false;  // The transparent background chip is on
    };

    // Group as the menu shows it
    struct PipelineVisibleGroup
    {
        PipelineMenuGroup         group;          // The group
        Vector<PipelineMenuModel> shown;          // Rows left after the filter
        bool                      folded = false; // Only the header is shown

        // Visible groups are equal when every field is
        bool operator==(const PipelineVisibleGroup& other) const
        {
            return group == other.group && shown == other.shown && folded == other.folded;
        }
    };

    // --------------------------------------------------------------------------------------------------
    // Rules of the model menu, the same as AssetsLine's shared/modelMenu.ts and the Unity plugin's: the
    // group of an id, the group order, which groups start folded, the search, Recent and the custom id row
    // --------------------------------------------------------------------------------------------------
    namespace PipelineModelMenu
    {
        const int recentMax = 5; // Rows Recent holds

        // Returns the name of the kind as the Recent file stores it: text, image, video, tts, elevenlabs
        String KindName(PipelineModelKind kind);

        // Returns the key of a provider group: google, openai, kling, elevenlabs, openrouter
        String ProviderKey(PipelineModelProvider provider);

        // Returns the header of a provider group: "Google Gemini", "OpenAI", ...
        String ProviderLabel(PipelineModelProvider provider);

        // Returns the caption of the provider chip: "Google", "OpenAI", ...
        String ChipLabel(PipelineModelProvider provider);

        // Returns the id without the "models/" prefix of Gemini's long form
        String BareId(const String& id);

        // Returns the vendor of a vendor/model id, a leading "~" dropped
        String VendorOf(const String& id);

        // Returns the name of an OpenRouter vendor: the known label, else its dash separated words capitalised
        String VendorLabel(const String& vendor);

        // Returns the group of a model id, from the id alone
        PipelineModelGroupKey GroupOf(const String& id);

        // Returns the name of a model inside its group: the product name without the provider part
        String RowName(const String& id);

        // Returns true when a row of the list shows the transparent background badge
        bool IsAlpha(PipelineModelKind kind, const String& id);

        // Builds the groups in display order from the offered ids (duplicates dropped), the node's value and the
        // recently picked ids (newest first)
        Vector<PipelineMenuGroup> BuildGroups(const Vector<String>& ids, PipelineModelKind kind, const String& current,
                                              const Vector<String>& recent);

        // Returns true when the group starts folded: an OpenRouter vendor group not holding the node's value
        bool FoldedByDefault(const PipelineMenuGroup& group, const String& current);

        // Returns true when every whitespace separated token of the query is in one of the fields, as typed or with
        // everything but letters and digits dropped from both ("gpt55" finds "gpt-5.5"); fields are never joined,
        // so a token cannot match across two of them
        bool Matches(const String& query, const Vector<String>& fields);

        // Returns the groups the menu shows with their rows left after the filter; a group without rows is dropped.
        // A search or the transparent background chip opens every group, otherwise a manual toggle wins over the default
        Vector<PipelineVisibleGroup> VisibleGroups(const Vector<PipelineMenuGroup>& groups, const PipelineMenuFilter& filter,
                                                   const String& current, const Map<String, bool>& toggled);

        // Returns the providers present, in chip order
        Vector<PipelineModelProvider> ProvidersOf(const Vector<PipelineMenuGroup>& groups);

        // Returns true when some row renders a transparent background itself
        bool HasAlpha(const Vector<PipelineMenuGroup>& groups);

        // Returns the id the "use as model id" row offers: the trimmed query without whitespace that no row has; empty for none
        String CustomIdFor(const String& query, const Vector<PipelineMenuGroup>& groups);

        // Returns the ids of the rows the highlight moves over, in order: the rows of the open groups, then the custom id
        Vector<String> PickableIds(const Vector<PipelineVisibleGroup>& groups, const String& customId);

        // Returns the row the menu opens on: the first one holding the node's value, else the first row
        int InitialHighlight(const Vector<String>& pickable, const String& current);

        // Returns the row a changed filter highlights among the model rows (the custom id row is never an exact
        // match): the one whose id is the trimmed query, else the first
        int HighlightFor(const String& query, const Vector<String>& modelIds);

        // Returns the list with the id moved to the front, capped
        Vector<String> PushRecent(const Vector<String>& list, const String& id, int max = recentMax);

        // Returns the path of the file the Recent lists are kept in, under the pipeline work folder
        String GetRecentPath();

        // Returns the Recent list of the kind, newest first
        Vector<String> LoadRecent(PipelineModelKind kind);

        // Stores the Recent list of the kind
        void SaveRecent(PipelineModelKind kind, const Vector<String>& list);

        // Moves the picked id to the front of the kind's Recent list and stores it
        void RememberPick(PipelineModelKind kind, const String& id);
    }
}
// --- META ---

PRE_ENUM_META(Editor::PipelineModelKind);

PRE_ENUM_META(Editor::PipelineModelProvider);
// --- END META ---
