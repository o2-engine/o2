#include "o2/stdafx.h"
#include "SceneEditableObject.h"

#include "o2/Scene/Scene.h"

namespace o2
{
#if IS_EDITOR
    SceneEditableObject::SceneEditableObject(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    SceneEditableObject::~SceneEditableObject()
    {}

    void SceneEditableObject::Update(float dt)
    {}

    bool SceneEditableObject::IsOnScene() const
    {
        return true;
    }

    void SceneEditableObject::Destroy()
    {}

    SceneUID SceneEditableObject::GetID() const
    {
        return 0;
    }

    void SceneEditableObject::GenerateNewID(bool childs /*= true*/)
    {}

    const String& SceneEditableObject::GetName() const
    {
        return String::empty;
    }

    void SceneEditableObject::SetName(const String& name)
    {}

    Ref<SceneEditableObject> SceneEditableObject::GetEditableLink() const
    {
        return nullptr;
    }

    bool SceneEditableObject::IsEditableLinkedTo(const Ref<SceneEditableObject>& link) const
    {
        if (auto thisLink = GetEditableLink())
        {
            while (thisLink)
            {
                if (thisLink == link)
                    return true;

                thisLink = thisLink->GetEditableLink();
            }
        }

        return false;
    }

    bool SceneEditableObject::IsSupportsLinking() const
    {
        return true;
    }

    Vector<Ref<SceneEditableObject>> SceneEditableObject::GetEditableChildren() const
    {
        return Vector<Ref<SceneEditableObject>>();
    }

    void SceneEditableObject::GetAllEditableChildren(Vector<Ref<SceneEditableObject>>& children)
    {
        auto thisChildren = GetEditableChildren();

        children.Add(thisChildren);

        for (auto& child : thisChildren)
            child->GetAllEditableChildren(children);
    }

    Ref<SceneEditableObject> SceneEditableObject::GetEditableParent() const
    {
        return nullptr;
    }

    void SceneEditableObject::SetEditableParent(const Ref<SceneEditableObject>& object, int idx /*= -1*/)
    {}

    void SceneEditableObject::AddEditableChild(const Ref<SceneEditableObject>& object, int idx /*= -1*/)
    {}

    void SceneEditableObject::SetIndexInSiblings(int idx)
    {}

    bool SceneEditableObject::CanBeParentedTo(const Type& parentType)
    {
        return parentType.IsBasedOn(TypeOf(Actor));
    }

    bool SceneEditableObject::IsSupportsDisabling() const
    {
        return false;
    }

    bool SceneEditableObject::IsEnabled() const
    {
        return true;
    }

    bool SceneEditableObject::IsEnabledInHierarchy() const
    {
        auto parent = GetEditableParent();
        return IsEnabled() && (parent ? parent->IsEnabledInHierarchy() : true);
    }

    void SceneEditableObject::SetEnabled(bool enabled)
    {}

    bool SceneEditableObject::IsSupportsLocking() const
    {
        return false;
    }

    bool SceneEditableObject::IsLocked() const
    {
        return false;
    }

    bool SceneEditableObject::IsLockedInHierarchy() const
    {
        return false;
    }

    void SceneEditableObject::SetLocked(bool locked)
    {}

    bool SceneEditableObject::IsSupportsTransforming() const
    {
        return false;
    }

    Basis SceneEditableObject::GetTransform() const
    {
        return Basis::Identity();
    }

    void SceneEditableObject::SetTransform(const Basis& transform)
    {}

    void SceneEditableObject::UpdateTransform()
    {}

    bool SceneEditableObject::IsSupportsPivot() const
    {
        return false;
    }

    void SceneEditableObject::SetPivot(const Vec2F& pivot)
    {}

    Vec2F SceneEditableObject::GetPivot() const
    {
        return Vec2F();
    }

    bool SceneEditableObject::IsSupportsLayout() const
    {
        return false;
    }

    Layout SceneEditableObject::GetLayout() const
    {
        return Layout();
    }

    void SceneEditableObject::SetLayout(const Layout& layout)
    {}

    bool SceneEditableObject::IsSupportsSnapping() const
    {
        return true;
    }

    bool SceneEditableObject::IsSupportsDeleting() const
    {
        return true;
    }

    void SceneEditableObject::OnChanged()
    {}

    void SceneEditableObject::OnLockChanged()
    {}

    void SceneEditableObject::OnNameChanged()
    {}

    void SceneEditableObject::OnChildrenChanged()
    {}

    void SceneEditableObject::OnEditableParentChanged(const Ref<SceneEditableObject>& oldParent)
    {}

    void SceneEditableObject::GetDifferences(ActorDifferences& differences) const
    {}
#endif
}
// --- META ---

#if  IS_EDITOR
DECLARE_CLASS(o2::SceneEditableObject, o2__SceneEditableObject);
#endif
// --- END META ---
