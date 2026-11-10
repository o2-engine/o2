#pragma once

#include "o2Editor/Pipeline/PipelineNodeType.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------------------------------------------
    // The groups the add-node menus show (AssetsLine's nodeGroups.ts): what a node is FOR, not the executor's category.
    // Six groups, each listed in a fixed order; a node type not named here falls into one by its category and output
    // ---------------------------------------------------------------------------------------------------------------
    struct PipelineNodeGroup
    {
        String         id;    // io, aiImage, imageTools, text, gameUi, av
        String         label; // The menu's name of the group
        String         title; // What the group holds, one line
        Vector<String> types; // Members in menu order; in io the inputs come first, then the results
    };

    namespace PipelineNodeGroups
    {
        // Returns the six groups in menu order
        const Vector<PipelineNodeGroup>& All();

        // Returns the id of the group a node type is listed in
        String GroupOf(const PipelineNodeSchema& schema);

        // Returns true for an entry of the io group that is a result (a finish) rather than an input
        bool IsResultNode(const PipelineNodeSchema& schema);

        // ----------------------------------------
        // A group with the schemas listed in it
        // ----------------------------------------
        struct Entry
        {
            const PipelineNodeGroup*          group = nullptr;
            Vector<const PipelineNodeSchema*> items;
        };

        // Returns the schemas by group, each in menu order (named members first, the rest by label); retired node types are
        // not offered and empty groups are left out
        Vector<Entry> Group(const Vector<const PipelineNodeSchema*>& schemas);
    }
}
