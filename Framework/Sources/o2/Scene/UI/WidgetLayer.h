#pragma once

#include "o2/Render/IRectDrawable.h"
#include "o2/Scene/UI/WidgetLayerLayout.h"
#include "o2/Utils/Basic/ITree.h"
#include "o2/Utils/Editor/Attributes/ExpandedByDefaultAttribute.h"
#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2/Utils/Math/Layout.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    class Widget;

#if IS_EDITOR
    typedef SceneEditableObject WidgetLayerBase;
#else
    struct WidgetLayerBase : public ISerializable, public RefCounterable, public ICloneableRef {};
#endif

    // ---------------------
    // Widget drawable layer
    // ---------------------
    class WidgetLayer : public WidgetLayerBase
    {
    public:
        PROPERTIES(WidgetLayer);
        PROPERTY(bool, enabled, SetEnabled, IsEnabled);                   // Enable property @SCRIPTABLE
        PROPERTY(float, depth, SetDepth, GetDepth);                       // Drawing depth (higher depths will draw later) @SCRIPTABLE
        PROPERTY(float, transparency, SetTransparency, GetTransparency);  // Drawable transparency property @SCRIPTABLE
        PROPERTY(Ref<IRectDrawable>, drawable, SetDrawable, GetDrawable); // Drawable property @EXPANDED_BY_DEFAULT @SCRIPTABLE

        ACCESSOR(Ref<WidgetLayer>, child, String, GetChild, GetAllChildLayers); // Child layer accessor

    public:
        WidgetLayerLayout layout; // Drawable layout @SERIALIZABLE @SCRIPTABLE

        String name;                // Name of layer @SERIALIZABLE @SCRIPTABLE
        Layout interactableLayout;  // Interactable area layout @SERIALIZABLE @SCRIPTABLE

    public:
        // Default constructor @SCRIPTABLE
        WidgetLayer(RefCounter* refCounter);

        // Copy-constructor
        WidgetLayer(RefCounter* refCounter, const WidgetLayer& other);

        // Destructor
        ~WidgetLayer();

        // Copy-operator
        WidgetLayer& operator=(const WidgetLayer& other);

        // Returns pointer to owner widget @SCRIPTABLE
        const WeakRef<Widget>& GetOwnerWidget() const;

        // Returns link to prototype @SCRIPTABLE
        const Ref<WidgetLayer>& GetPrototypeLink() const;

        // Sets layer drawable
        void SetDrawable(const Ref<IRectDrawable>& drawable);

        // Returns layer drawable
        const Ref<IRectDrawable>& GetDrawable() const;

        // Draws drawable @SCRIPTABLE
        void Draw();

        // Returns is layer enabled @SCRIPTABLE
        bool IsEnabled() const OPTIONAL_OVERRIDE;

        // Returns is layer enabled and all parent are enabled too @SCRIPTABLE
        bool IsEnabledInHierarchy() const OPTIONAL_OVERRIDE;

        // Sets enabling of layer @SCRIPTABLE
        void SetEnabled(bool enabled) OPTIONAL_OVERRIDE;

        // Sets parent layer @SCRIPTABLE
        void SetParent(const Ref<WidgetLayer>& parent);

        // Returns parent layer @SCRIPTABLE
        const WeakRef<WidgetLayer>& GetParent() const;

        // Adds new child layer and returns him @SCRIPTABLE
        Ref<WidgetLayer> AddChild(const Ref<WidgetLayer>& node);

        // Remove child layer and releases him if needs @SCRIPTABLE
        void RemoveChild(const Ref<WidgetLayer>& layer);

        // Removes and releases all children nodes @SCRIPTABLE
        void RemoveAllChildren();

        // Return child layers @SCRIPTABLE
        Vector<Ref<WidgetLayer>>& GetChildren();

        // Returns constant child layers
        const Vector<Ref<WidgetLayer>>& GetChildren() const;

        // Adds child layer @SCRIPTABLE
        Ref<WidgetLayer> AddChildLayer(const String& name, const Ref<IRectDrawable>& drawable, const Layout& layout = Layout::BothStretch(),
                                       float depth = 0.0f);

        // Returns child layer by path @SCRIPTABLE
        Ref<WidgetLayer> GetChild(const String& path);

        // Returns child layer by name @SCRIPTABLE
        Ref<WidgetLayer> FindChild(const String& name);

        // Returns child layer with type
        template<typename _type>
        Ref<_type> FindChild() const;

        // Returns all child layers @SCRIPTABLE
        Vector<Ref<WidgetLayer>> GetAllChilds() const;

        // Sets depth
        void SetDepth(float depth);

        // Returns depth
        float GetDepth() const;

        // Sets transparency and updates this and children result transparencies
        void SetTransparency(float transparency);

        // Returns transparency
        float GetTransparency();

        // Returns result transparency @SCRIPTABLE
        float GetResTransparency() const;

        // Returns true if layer is under point @SCRIPTABLE
        bool IsUnderPoint(const Vec2F& point);

        // Returns layout rectangle @SCRIPTABLE
        const RectF& GetRect() const;

        SERIALIZABLE(WidgetLayer);
        CLONEABLE_REF(WidgetLayer);

    protected:
        struct ICopyVisitor : public RefCounterable
        {
            virtual ~ICopyVisitor() {}
            virtual void OnCopy(const WidgetLayer* source, WidgetLayer* target) = 0;
        };

        struct MakePrototypeCloneVisitor : public ICopyVisitor
        {
            void OnCopy(const WidgetLayer* source, WidgetLayer* target) override;
        };

        struct InstantiatePrototypeCloneVisitor : public ICopyVisitor
        {
            void OnCopy(const WidgetLayer* source, WidgetLayer* target) override;
        };

    protected:
        mutable Ref<ICopyVisitor> mCopyVisitor; // Copy visitor. Used when instantiating prefab

        Ref<WidgetLayer> mPrototypeLink; // Linked prototype layer

        Ref<IRectDrawable> mDrawable; // Drawable @SERIALIZABLE

        SceneUID mUID = 0; // Scene editor uid @SERIALIZABLE @IGNORE_DELTA_SEARCH

        bool mEnabled = true; // Is layer enabled

        float mTransparency = 1.0f;    // Layer transparency @SERIALIZABLE
        float mResTransparency = 1.0f; // Result drawable transparency, depends on parent transparency

        float mDepth = 0.0f; // Depth of drawable @SERIALIZABLE

        RectF mAbsolutePosition; // Result absolute drawable position
        RectF mInteractableArea; // Intractable area, depends on interactableLayout

        WeakRef<Widget> mOwnerWidget; // Owner widget pointer 

        WeakRef<WidgetLayer>     mParent;   // Pointer to parent layer 
        Vector<Ref<WidgetLayer>> mChildren; // Children layers

        bool mUpdatingLayout = false; // It is true when updating layout now, prevents recursive layout updating 

    protected:
        // It is called after reference initialization at object construction, sets layout owner
        void PostRefConstruct();

        // Beginning serialization callback
        void SerializeBasicOverride(DataValue& node) const;

        // Completion deserialization callback
        void DeserializeBasicOverride(const DataValue& node);

        // Regular serializing without prototype
        void SerializeRaw(DataValue& node) const;

        // Regular deserializing without prototype
        void DeserializeRaw(const DataValue& node);

        // Regular serializing with prototype
        void SerializeWithProto(DataValue& node) const;

        // Regular deserializing with prototype
        void DeserializeWithProto(const DataValue& node);

        // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

        // Sets owner widget for this and children
        void SetOwnerWidget(const Ref<Widget>& owner);

        // Called when layout was changed, calls owner widget layout updating
        void OnLayoutChanged();

        // Updates drawable rect, calling when widget's layout was changed
        void UpdateLayout();

        // Called when transparency was changed and updates children transparencies
        void UpdateResTransparency();

        // Called when owner widget including in scene. Registers editable object and it's children
        void OnAddToScene();

        // Called when owner widget excluding in scene. Unregisters editable object and it's children
        void OnRemoveFromScene();

        // Returns dictionary with all child layers
        Map<String, Ref<WidgetLayer>> GetAllChildLayers();

#if IS_EDITOR
    public:
        PROPERTY(bool, locked, SetLocked, IsLocked); // Is locked property

    public:
        // Returns true when object is on scene
        bool IsOnScene() const override;

        // Returns unique id
        SceneUID GetID() const override;

        // Generates new random id 
        void GenerateNewID(bool childs = true) override;

        // Returns name of object
        const String& GetName() const override;

        // Sets name of object
        void SetName(const String& name) override;

        // Returns object's link to prototype
        Ref<SceneEditableObject> GetEditableLink() const override;

        // Returns list of object's children
        Vector<Ref<SceneEditableObject>> GetEditableChildren() const override;

        // Returns object's parent object. Return nullptr when it is a root scene object
        Ref<SceneEditableObject> GetEditableParent() const override;

        // Sets parent object. nullptr means make this object as root. idx is place in parent children. idx == -1 means last
        void SetEditableParent(const Ref<SceneEditableObject>& object, int idx = -1) override;

        // Adds child. idx is place in parent children. idx == -1 means last
        void AddEditableChild(const Ref<SceneEditableObject>& object, int idx = -1) override;

        // Sets index in siblings - children of parent
        void SetIndexInSiblings(int idx) override;

        // Checks that this object can be added as child to another object
        bool CanBeParentedTo(const Type& parentType) override;

        // Returns is that type of object can be enabled and disabled
        bool IsSupportsDisabling() const override;

        // Returns is that type of object can be locked
        bool IsSupportsLocking() const override;

        // Returns is object locked, override when it's supports
        bool IsLocked() const override;

        // Returns is object locked and all parent are locked too
        bool IsLockedInHierarchy() const override;

        // Sets locking of object, override when it's supports
        void SetLocked(bool locked) override;

        // Returns is that type of object can be transformed
        bool IsSupportsTransforming() const override;

        // Returns transform, override when it's supports
        Basis GetTransform() const override;

        // Sets transform of object, override when it's supports
        void SetTransform(const Basis& transform) override;

        // Updates transform immediately
        void UpdateTransform() OPTIONAL_OVERRIDE;

        // Returns is object supports pivot 
        bool IsSupportsPivot() const override;

        // Sets transform pivot point
        void SetPivot(const Vec2F& pivot) override;

        // Returns transform pivot
        Vec2F GetPivot() const override;

        // Returns is that type of object can be transformed with layout
        bool IsSupportsLayout() const override;

        // Returns layout, override when it's supports
        Layout GetLayout() const override;

        // Sets layout of object, override when it's supports
        void SetLayout(const Layout& layout) override;

        // Called when something changed in this object
        void OnChanged() override;

    protected:
        bool mIsLocked = false; // Is locked

    protected:
        // Called before making prototype from this object
        void BeginMakePrototype() const override;

        // Called before instantiate from this object
        void BeginInstantiatePrototype() const override;

#endif // IS_EDITOR

        friend class Widget;
        friend class WidgetLayerLayout;

        FRIEND_REF_MAKE();
    };

    template<typename _type>
    Ref<_type> WidgetLayer::FindChild() const
    {
        for (auto& child : mChildren)
            if (child->mDrawable && child->mDrawable->GetType() == TypeOf(_type))
                return DynamicCast<_type>(child->mDrawable);

        for (auto& child : mChildren) {
            auto res = child->FindChild<_type>();
            if (res)
                return res;
        }

        return nullptr;
    }
}
// --- META ---

CLASS_BASES_META(o2::WidgetLayer)
{
    BASE_CLASS(o2::WidgetLayerBase);
}
END_META;
CLASS_FIELDS_META(o2::WidgetLayer)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(enabled);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(depth);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(transparency);
    FIELD().PUBLIC().EXPANDED_BY_DEFAULT_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(drawable);
    FIELD().PUBLIC().NAME(child);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(layout);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(name);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(interactableLayout);
    FIELD().PROTECTED().NAME(mCopyVisitor);
    FIELD().PROTECTED().NAME(mPrototypeLink);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mDrawable);
    FIELD().PROTECTED().IGNORE_DELTA_SEARCH_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(mUID);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mEnabled);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mTransparency);
    FIELD().PROTECTED().DEFAULT_VALUE(1.0f).NAME(mResTransparency);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mDepth);
    FIELD().PROTECTED().NAME(mAbsolutePosition);
    FIELD().PROTECTED().NAME(mInteractableArea);
    FIELD().PROTECTED().NAME(mOwnerWidget);
    FIELD().PROTECTED().NAME(mParent);
    FIELD().PROTECTED().NAME(mChildren);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mUpdatingLayout);
#if  IS_EDITOR
    FIELD().PUBLIC().NAME(locked);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsLocked);
#endif
}
END_META;
CLASS_METHODS_META(o2::WidgetLayer)
{

    typedef Map<String, Ref<WidgetLayer>> _tmp1;

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const WidgetLayer&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const WeakRef<Widget>&, GetOwnerWidget);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const Ref<WidgetLayer>&, GetPrototypeLink);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetDrawable);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsEnabled);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsEnabledInHierarchy);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetEnabled, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetParent, const Ref<WidgetLayer>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const WeakRef<WidgetLayer>&, GetParent);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<WidgetLayer>, AddChild, const Ref<WidgetLayer>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, RemoveChild, const Ref<WidgetLayer>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, RemoveAllChildren);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vector<Ref<WidgetLayer>>&, GetChildren);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<WidgetLayer>>&, GetChildren);
    FUNCTION().PUBLIC().SIGNATURE(Ref<WidgetLayer>, AddChildLayer, const String&, const Ref<IRectDrawable>&, const Layout&, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<WidgetLayer>, GetChild, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<WidgetLayer>, FindChild, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vector<Ref<WidgetLayer>>, GetAllChilds);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDepth, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDepth);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTransparency, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTransparency);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetResTransparency);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const RectF&, GetRect);
    FUNCTION().PROTECTED().SIGNATURE(void, PostRefConstruct);
    FUNCTION().PROTECTED().SIGNATURE(void, SerializeBasicOverride, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, DeserializeBasicOverride, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, SerializeRaw, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, DeserializeRaw, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, SerializeWithProto, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, DeserializeWithProto, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetOwnerWidget, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLayoutChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayout);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateResTransparency);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
    FUNCTION().PROTECTED().SIGNATURE(_tmp1, GetAllChildLayers);
#if  IS_EDITOR
    FUNCTION().PUBLIC().SIGNATURE(bool, IsOnScene);
    FUNCTION().PUBLIC().SIGNATURE(SceneUID, GetID);
    FUNCTION().PUBLIC().SIGNATURE(void, GenerateNewID, bool);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, SetName, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<SceneEditableObject>, GetEditableLink);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<SceneEditableObject>>, GetEditableChildren);
    FUNCTION().PUBLIC().SIGNATURE(Ref<SceneEditableObject>, GetEditableParent);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEditableParent, const Ref<SceneEditableObject>&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, AddEditableChild, const Ref<SceneEditableObject>&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIndexInSiblings, int);
    FUNCTION().PUBLIC().SIGNATURE(bool, CanBeParentedTo, const Type&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsDisabling);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsLocking);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLocked);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLockedInHierarchy);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLocked, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsTransforming);
    FUNCTION().PUBLIC().SIGNATURE(Basis, GetTransform);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTransform, const Basis&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateTransform);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsPivot);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPivot, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetPivot);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsLayout);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, BeginMakePrototype);
    FUNCTION().PROTECTED().SIGNATURE(void, BeginInstantiatePrototype);
#endif
}
END_META;
// --- END META ---
