#pragma once

#include <optional>
#include <set>
#include <string>

#include "o2/Utils/Types/Containers/Map.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

namespace Editor::UiLayout
{
    // Map that keeps its keys in insertion order, as a JavaScript object or Map does
    template<typename _value>
    class OrderedMap
    {
    public:
        // Returns the value, null when missing
        const _value* Find(const String& key) const
        {
            for (auto& entry : mEntries)
            {
                if (entry.first == key)
                    return &entry.second;
            }
            return nullptr;
        }

        _value* Find(const String& key) { return const_cast<_value*>(static_cast<const OrderedMap*>(this)->Find(key)); }

        // Returns true when the key is present
        bool Has(const String& key) const { return Find(key) != nullptr; }

        // Sets the value keeping the key's place, or appends it
        void Set(const String& key, const _value& value)
        {
            if (auto existing = Find(key))
                *existing = value;
            else
                mEntries.push_back({ key, value });
        }

        // Returns the value, appending a default one when missing
        _value& operator[](const String& key)
        {
            if (auto existing = Find(key))
                return *existing;

            mEntries.push_back({ key, _value() });
            return mEntries.back().second;
        }

        // Removes the key
        void Remove(const String& key)
        {
            mEntries.erase(std::remove_if(mEntries.begin(), mEntries.end(), [&](const auto& e) { return e.first == key; }), mEntries.end());
        }

        int Count() const { return (int)mEntries.size(); }

        auto begin() const { return mEntries.begin(); }
        auto end() const { return mEntries.end(); }
        auto begin() { return mEntries.begin(); }
        auto end() { return mEntries.end(); }

    private:
        std::vector<std::pair<String, _value>> mEntries;
    };

    // Kinds the analysis names elements by
    extern const Vector<String> uiKinds;

    // Position of a grid cell
    struct UiCell
    {
        String of; // Id of the grid element
        int    row = 0;
        int    col = 0;
    };

    // -----------------------------------------------------------------------------------------------
    // One element of the node's working model: absolute design pixels, parents before children. The
    // analysis fills it, the texts are measured by their ink; it tells the cuts what to keep and what to erase
    // -----------------------------------------------------------------------------------------------
    struct UiEl: public RefCounterable
    {
        String id;
        String parent;
        String kind; // panel, image, button, text, progress, group, grid, input
        Box    box;
        String look; // Artwork key, empty for none

        String text;
        String color;  // #RRGGBB, empty for none
        String weight; // regular, medium, bold; empty for none
        String align;  // left, center, right; empty for none
        String stroke; // Outline colour, empty for none

        std::optional<double> strokeWidth;
        std::optional<double> fontSize;
        std::optional<Box>    ink; // Measured visible glyph box

        std::optional<double> value; // Progress fill ratio

        std::optional<int> rows;
        std::optional<int> cols;

        std::optional<UiCell> cell; // Grid cell position
        std::optional<int>    slot; // Text in a grid cell: 0 the main label, 1+ the smaller ones

        std::optional<Box> rough; // The box the analysis gave, before any measurement

        String measured; // How the box was obtained: analysis or ink

    public:
        // Returns a deep copy
        Ref<UiEl> Clone() const;
    };

    // The screen behind the UI as the analysis describes it
    struct UiBackground
    {
        String kind = "color"; // image or color
        String color = "#808080";
        String describe;
    };

    // ------------------------------------------------------------
    // The working model: the elements and their looks by look key
    // ------------------------------------------------------------
    struct UiDoc
    {
        int w = 0, h = 0;
        UiBackground         background;
        OrderedMap<String>   looks;
        Vector<Ref<UiEl>>    els;

    public:
        // Returns a deep copy (elements cloned)
        UiDoc Clone() const;

        // Returns the element, null when missing
        Ref<UiEl> ById(const String& id) const;

        // Returns the index of the element in the list, -1 when missing
        int IndexOf(const Ref<UiEl>& e) const;

        // Elements whose parent is the id, in list order
        Vector<Ref<UiEl>> ChildrenOf(const String& id) const;

        // Every element below the id, depth first in list order
        Vector<Ref<UiEl>> DescendantsOf(const String& id) const;

        // The element without taking a reference: what the parallel cut jobs read
        const UiEl* Find(const String& id) const;

        // Adds the ids of every element below the id, without taking references
        void CollectDescendantIds(const String& id, std::set<std::string>& out) const;
    };

    // Elements that draw a sprite
    bool HasLook(const UiEl& e);

    // Panels, inputs and progress tracks stretch: cut once and nine-sliced
    bool IsStretchy(const UiEl& e);

    // A file-safe identifier: lower case, runs of other characters as one underscore, at most 48 characters
    String SafeKey(const String& s, const String& fallback = "el");
}
