#pragma once

#include "o2/Render/Pipeline/RenderPass.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // ------------------------------------------------------------------------
    // Render pipeline: ordered list of render passes executed for a camera.
    // Serializable, so a customized pipeline can be stored on a camera actor
    // ------------------------------------------------------------------------
    class RenderPipeline: public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        // Default constructor
        RenderPipeline();

        // Copy-constructor, clones passes
        RenderPipeline(const RenderPipeline& other);

        // Virtual destructor
        virtual ~RenderPipeline() {}

        // Adds pass at the end
        void AddPass(const Ref<RenderPass>& pass);

        // Inserts pass at position
        void InsertPass(const Ref<RenderPass>& pass, int position);

        // Removes pass
        void RemovePass(const Ref<RenderPass>& pass);

        // Returns first pass with specified type
        template<typename _pass_type>
        Ref<_pass_type> GetPass() const;

        // Returns passes list
        const Vector<Ref<RenderPass>>& GetPasses() const;

        // Executes enabled passes in order
        virtual void Execute(RenderPassContext& context);

        SERIALIZABLE(RenderPipeline);
        CLONEABLE_REF(RenderPipeline);

    protected:
        Vector<Ref<RenderPass>> mPasses; // Ordered passes list @SERIALIZABLE @EDITOR_PROPERTY @EXPANDED_BY_DEFAULT
    };

    template<typename _pass_type>
    Ref<_pass_type> RenderPipeline::GetPass() const
    {
        for (auto& pass : mPasses)
        {
            if (auto typed = DynamicCast<_pass_type>(pass))
                return typed;
        }

        return nullptr;
    }
}
// --- META ---

CLASS_BASES_META(o2::RenderPipeline)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::RenderPipeline)
{
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().EXPANDED_BY_DEFAULT_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mPasses);
}
END_META;
CLASS_METHODS_META(o2::RenderPipeline)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const RenderPipeline&);
    FUNCTION().PUBLIC().SIGNATURE(void, AddPass, const Ref<RenderPass>&);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertPass, const Ref<RenderPass>&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, RemovePass, const Ref<RenderPass>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<RenderPass>>&, GetPasses);
    FUNCTION().PUBLIC().SIGNATURE(void, Execute, RenderPassContext&);
}
END_META;
// --- END META ---
