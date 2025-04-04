#pragma once

#include "o2/Assets/Types/AnimationAsset.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/ISceneDrawable.h"
#include "o2/Scene/SceneLayer.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Utils/Editor/Attributes/AnimatableAttribute.h"
#include "o2/Utils/Editor/Attributes/DefaultTypeAttribute.h"
#include "o2/Utils/Editor/Attributes/DontDeleteAttribute.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"
#include "o2/Utils/Editor/Attributes/InvokeOnChangeAttribute.h"
#include "o2/Utils/Math/Layout.h"

namespace o2
{
    class IRectDrawable;
    class WidgetLayer;
    class WidgetLayout;
    class WidgetLayoutData;

    // ------------------------------------------------------
    // Basic UI Widget. Its a simple and basic element of UI, 
    // everything other UI's are based on this
    // ------------------------------------------------------
    class Widget : public Actor
    {
    public:
        PROPERTIES(Widget);

        PROPERTY(bool, enabledForcibly, SetEnabledForcible, IsEnabled); // Enable property, works forcibly @EDITOR_IGNORE @ANIMATABLE 

        PROPERTY(float, transparency, SetTransparency, GetTransparency); // Transparency property 
        GETTER(float, resTransparency, GetResTransparency);              // Result transparency getter, depends on parent transparency @EDITOR_IGNORE @ANIMATABLE

        GETTER(Vector<Ref<Widget>>, childrenWidgets, GetChildrenNonConst); // Widget children getter

        GETTER(Vector<Ref<WidgetLayer>>, layers, GetLayers); // Layers getter
        GETTER(Vector<Ref<WidgetState>>, states, GetStates); // States getter

        ACCESSOR(Ref<Widget>, childWidget, String, GetChildWidget, GetAllChilds);                // Widget child accessor by path like "child/subchild/somechild"
        ACCESSOR(Ref<Widget>, internalWidget, String, GetInternalWidget, GetAllInternalWidgets); // Widget internals accessor by path like "child/subchild/somechild"
        ACCESSOR(Ref<WidgetLayer>, layer, String, GetLayer, GetAllLayers);                       // Widget layer accessor by path like "layer/sublayer/target"
        ACCESSOR(Ref<WidgetState>, state, String, GetStateObject, GetAllStates);                 // Widget state accessor by name

    public:
        WidgetLayout* const layout; // Widget layout @EDITOR_IGNORE

    public:
        Function<void()> onLayoutUpdated; // Layout change event @EDITOR_IGNORE
        Function<void()> onFocused;       // Widget focused event @EDITOR_IGNORE
        Function<void()> onUnfocused;     // Widget unfocused event @EDITOR_IGNORE
        Function<void()> onShow;          // Widget showing vent @EDITOR_IGNORE
        Function<void()> onHide;          // Widget hiding event @EDITOR_IGNORE

    public:
        // Default constructor @SCRIPTABLE
        explicit Widget(RefCounter* refCounter, ActorCreateMode mode = ActorCreateMode::Default);

        // Widget constructor from prototype
        Widget(RefCounter* refCounter, const AssetRef<ActorAsset>& prototype, ActorCreateMode mode = ActorCreateMode::Default);

        // Widget constructor with components
        Widget(RefCounter* refCounter, Vector<Ref<Component>> components, ActorCreateMode mode = ActorCreateMode::Default);

        // Copy-constructor
        Widget(RefCounter* refCounter, const Widget& other);

        // Virtual destructor
        virtual ~Widget();

        // Copy-operator
        Widget& operator=(const Widget& other);

        // Updates layers, states and widget
        void Update(float dt) override;

        // Updates childs
        void UpdateChildren(float dt) override;

        // Updates self transform, dependent parents and children transforms
        void UpdateTransform() override;

        // Updates children and internal children transforms
        void UpdateChildrenTransforms() override;

        // Draws widget and child widgets with not overridden depth
        void Draw() override;

        // Forcible drawing in area with transparency @SCRIPTABLE
        void ForceDraw(const RectF& area, float transparency);

        // Sets layout dirty, and update it in update loop @SCRIPTABLE
        void SetLayoutDirty();

        // Returns parent widget @SCRIPTABLE
        const WeakRef<Widget>& GetParentWidget() const;

        // Returns children rectangle layout @SCRIPTABLE
        const RectF& GetChildrenWorldRect() const;

        // Returns child widget by path (like "root/some node/other node/target node") @SCRIPTABLE
        Ref<Widget> GetChildWidget(const String& path) const;

        // Add child actor @SCRIPTABLE
        Ref<Actor> AddChild(const Ref<Actor>& actor) override;

        // Add child actor @SCRIPTABLE
        Ref<Actor> AddChild(const Ref<Actor>& actor, int index) override;

        // Adds child widget and returns them @SCRIPTABLE
        Ref<Widget> AddChildWidget(const Ref<Widget>& widget);

        // Adds child widget at position and returns them
        Ref<Widget> AddChildWidget(const Ref<Widget>& widget, int position);

        // Returns constant children widgets vector
        const Vector<Ref<Widget>>& GetChildWidgets() const;

        // Sets index position in parent or scene @SCRIPTABLE
        void SetIndexInSiblings(int index) override;

        // Adds layer @SCRIPTABLE
        Ref<WidgetLayer> AddLayer(const Ref<WidgetLayer>& layer);

        // Adds layer @SCRIPTABLE_NAME(AddLayerNew)
        Ref<WidgetLayer> AddLayer(const String& name, const Ref<IRectDrawable>& drawable,
                                  const Layout& layout = Layout::BothStretch(), float depth = 0.0f);

        // Removes layer @SCRIPTABLE
        void RemoveLayer(const Ref<WidgetLayer>& layer);

        // Removes layer @SCRIPTABLE_NAME(RemoveLayerByPath)
        void RemoveLayer(const String& path);

        // Removes all layers @SCRIPTABLE
        void RemoveAllLayers();

        // Returns layer by path. Returns null if layer isn't exist @SCRIPTABLE
        Ref<WidgetLayer> GetLayer(const String& path) const;

        // Returns layer by name in layers hierarchy. Returns null if layer isn't exist @SCRIPTABLE
        Ref<WidgetLayer> FindLayer(const String& name) const;

        // Searches layer with drawable with specified type
        template<typename _type>
        Ref<_type> GetLayerDrawableByType();

        // Returns layer by path. Returns null if layer isn't exist or layer drawable has different type
        template<typename _type>
        Ref<_type> GetLayerDrawable(const String& path) const;

        // Returns all layers @SCRIPTABLE
        const Vector<Ref<WidgetLayer>>& GetLayers() const;

        // Adds new state with name @SCRIPTABLE_NAME(AddStateNew)
        Ref<WidgetState> AddState(const String& name);

        // Adds new state with name and animation @SCRIPTABLE_NAME(AddStateWithClip)
        Ref<WidgetState> AddState(const String& name, const Ref<AnimationClip>& animation);

        // Adds new state with name and animation @SCRIPTABLE_NAME(AddStateWithClipAsset)
        Ref<WidgetState> AddState(const String& name, const AssetRef<AnimationAsset>& animation);

        // Adds state @SCRIPTABLE
        Ref<WidgetState> AddState(const Ref<WidgetState>& state, bool showAnimErrors = true);

        // Removes state by name @SCRIPTABLE_NAME(RemoveStateByName)
        bool RemoveState(const String& name);

        // Removes state @SCRIPTABLE
        bool RemoveState(const Ref<WidgetState>& state);

        // Removes all states @SCRIPTABLE
        void RemoveAllStates();

        // Sets state value @SCRIPTABLE
        void SetState(const String& name, bool state);

        // Sets state value immediately @SCRIPTABLE
        void SetStateForcible(const String& name, bool state);

        // Returns state value @SCRIPTABLE
        bool GetState(const String& name) const;

        // Returns state object by name @SCRIPTABLE
        Ref<WidgetState> GetStateObject(const String& name) const;

        // Returns all states @SCRIPTABLE
        const Vector<Ref<WidgetState>>& GetStates() const;

        // Sets widget's transparency
        void SetTransparency(float transparency);

        // Returns widget's transparency
        float GetTransparency() const;

        // Returns widget's result transparency (depends on parent's result transparency) @SCRIPTABLE
        float GetResTransparency() const;

        // Sets visibility @SCRIPTABLE
        void SetEnabledForcible(bool visible);

        // Sets visibility to true @SCRIPTABLE
        void Show(bool forcible = false);

        // Sets visibility to false @SCRIPTABLE
        void Hide(bool forcible = false);

        // Focus this widget @SCRIPTABLE
        void Focus();

        // Unfocus this widget @SCRIPTABLE
        void Unfocus();

        // Returns is this widget focused @SCRIPTABLE
        bool IsFocused() const;

        // Returns is this widget can be focused @SCRIPTABLE
        virtual bool IsFocusable() const;

        // Sets widget can be focused @SCRIPTABLE
        void SetFocusable(bool focusable);

        // Returns true if point is under drawable @SCRIPTABLE
        bool IsUnderPoint(const Vec2F& point) override;

        // Sets parent,  doesn't adds to parent's children but adds to internal children @SCRIPTABLE
        void SetInternalParent(const Ref<Widget>& parent, bool worldPositionStays = false);

        // Adds widget to internal children @SCRIPTABLE
        void AddInternalWidget(const Ref<Widget>& widget, bool worldPositionStays = false);

		// Removes widget from internal children @SCRIPTABLE
		void RemoveInternalWidget(const Ref<Widget>& widget);

        // Returns internal child widget by path (like "root/some node/other node/target node") @SCRIPTABLE
        Ref<Widget> GetInternalWidget(const String& path) const;

        // Returns internal child widget by path (like "root/some node/other node/target node")
        template<typename _type>
        Ref<_type> GetInternalWidgetByType(const String& path) const;

        // Searches widget with name in internal widgets hierarchy @SCRIPTABLE
        Ref<Widget> FindInternalWidget(const String& name) const;

        // Searches widget with type and name in internal widgets hierarchy
        template<typename _type>
        Ref<_type> FindInternalWidgetByType(const String& name) const;

        // Searches widget with type in internal widgets hierarchy
        template<typename _type>
        Ref<_type> FindInternalWidgetByType() const;

        // Searches actor with id in this and this children @SCRIPTABLE
        Ref<Actor> FindActorById(SceneUID id) override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(Widget);
        CLONEABLE_REF(Widget);

    protected:
        using Actor::mState;

        Vector<Ref<WidgetLayer>> mLayers; // Layers array @DONT_DELETE @DEFAULT_TYPE(o2::WidgetLayer)
        Vector<Ref<WidgetState>> mStates; // States array @DONT_DELETE @DEFAULT_TYPE(o2::WidgetState) @EDITOR_PROPERTY @INVOKE_ON_CHANGE(OnStatesListChanged)

        WeakRef<Widget>     mParentWidget;    // Parent widget. When parent is not widget, this field will be null 
        Vector<Ref<Widget>> mChildWidgets;    // Children widgets, a part of all children @DONT_DELETE @DEFAULT_TYPE(o2::Widget)
        Vector<Ref<Widget>> mInternalWidgets; // Internal widgets, used same as children widgets, but not really children @DONT_DELETE @DEFAULT_TYPE(o2::Widget)

        float mTransparency = 1.0f;    // Widget transparency @SERIALIZABLE
        float mResTransparency = 1.0f; // Widget result transparency, depends on parent's result transparency

        Vector<Ref<WidgetLayer>> mDrawingLayers;    // Layers ordered by depth, which drawing before children (depth < 1000) @DONT_DELETE @DEFAULT_TYPE(o2::WidgetLayer)
        Vector<Ref<WidgetLayer>> mTopDrawingLayers; // Layers ordered by depth, which drawing after children (depth > 1000) @DONT_DELETE @DEFAULT_TYPE(o2::WidgetLayer)

        Ref<WidgetState> mFocusedState;        // Focused widget state @DONT_DELETE @DEFAULT_TYPE(o2::WidgetState)
        bool             mIsFocused = false;   // Is widget focused
        bool             mIsFocusable = false; // Is widget can be focused @SERIALIZABLE

        Ref<WidgetState> mVisibleState; // Widget visibility state @DONT_DELETE @DEFAULT_TYPE(o2::WidgetState)

        bool mIsClipped = false; // Is widget fully clipped by some scissors

        RectF mBounds;           // Widget bounds by drawing layers
        RectF mBoundsWithChilds; // Widget with childs bounds

    protected:
        // Regular serializing without prototype
        void SerializeRaw(DataValue& node) const override;

        // Regular deserializing without prototype
        void DeserializeRaw(const DataValue& node) override;

        // Regular serializing with prototype
        void SerializeWithProto(DataValue& node) const override;

        // Regular deserializing with prototype
        void DeserializeWithProto(const DataValue& node) override;

        // Completion deserialization callback; initializes layers and children
        void OnDeserialized(const DataValue& node) override;

        // Completion deserialization delta callback; initializes layers and children
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

        // Updates result read enable flag
        void UpdateResEnabled(bool withChildren = true) override;

        // Updates enabling
        void UpdateResEnabledInHierarchy(bool withChildren = true) override;

        // Called when transformation was changed and updated
        void OnTransformUpdated() override;

        // Called when actor enabled in hierarchy
        void OnEnabled() override;

        // Called when actor disabled in hierarchy
        void OnDisabled() override;

        // Called when parent changed
        void OnParentChanged(const Ref<Actor>& oldParent) override;

        // Called when actor children has rearranged; updates inherited depth drawables list
        void OnChildrenChanged() override;

        // Called when child actor was added
        void OnChildAdded(const Ref<Actor>& child) override;

        // Called when child actor was removed
        void OnChildRemoved(const Ref<Actor>& child) override;

        // Called when actor including from scene, including this to layer drawables
        void OnAddToScene() override;

        // Called when actor excluding from scene, removes this from layer drawables
        void OnRemoveFromScene() override;

		// Draws widget's layers (below children)
        virtual void DrawLayers();

		// Draws widget's top layers (above children)
		virtual void DrawTopLayers();

		// Draws internal children
        virtual void DrawInternalChildren();

        // Updates child widgets list
        void UpdateChildWidgetsList();

        // Returns layout data reference
        WidgetLayoutData& GetLayoutData();

        // Returns layout data reference
        const WidgetLayoutData& GetLayoutData() const;

        // Sets children world rectangle. Here you can override children position
        virtual void SetChildrenWorldRect(const RectF& childrenWorldRect);

        // Moves widget's to delta and checks for clipping
        virtual void MoveAndCheckClipping(const Vec2F& delta, const RectF& clipArea);

        // Called when child widget was added
        virtual void OnChildAdded(const Ref<Widget>& child);

        // Called when child widget was removed
        virtual void OnChildRemoved(const Ref<Widget>& child);

        // Called when widget was selected
        virtual void OnFocused();

        // Called when widget was deselected
        virtual void OnUnfocused();

        // Returns layout width with children
        virtual float GetMinWidthWithChildren() const;

        // Returns layout height with children
        virtual float GetMinHeightWithChildren() const;

        // Returns layout width weight with children
        virtual float GetWidthWeightWithChildren() const;

        // Returns layout height weight with children
        virtual float GetHeightWeightWithChildren() const;

        // Updates bounds by drawing layers
        virtual void UpdateBounds();

        // Updates bound with children
        virtual void UpdateBoundsWithChilds();

        // Checks widget clipping by area
        virtual void CheckClipping(const RectF& clipArea);

        // Updates transparency for this and children widgets
        virtual void UpdateTransparency();

        // Updates result visibility
        virtual void UpdateVisibility(bool updateLayout = true);

        // Updates layers layouts, calls after updating widget layout
        virtual void UpdateLayersLayouts();

        // Called when child widget was selected
        virtual void OnChildFocused(const Ref<Widget>& child);

        // Called when layer added and updates drawing sequence
        virtual void OnLayerAdded(const Ref<WidgetLayer>& layer);

        // Called when widget state was added
        virtual void OnStateAdded(const Ref<WidgetState>& state);

        // Called from editor, refreshes states
        void OnStatesListChanged();

        // Draws debug frame by mAbsoluteRect
        void DrawDebugFrame();

        // Updates layers drawing sequence
        void UpdateLayersDrawingSequence();

        // Sets new target for all states animations
        void RetargetStatesAnimations();

        // Sets parent widget, used for property
        void SetParentWidget(const Ref<Widget>& widget);

        // Returns children widgets (for property)
        Vector<Ref<Widget>>& GetChildrenNonConst();

        // Returns layers (for property)
        Vector<Ref<WidgetLayer>>& GetLayersNonConst();

        // Returns states (for property)
        Vector<Ref<WidgetState>>& GetStatesNonConst();

        // Returns dictionary of all layers by names
        Map<String, Ref<WidgetLayer>> GetAllLayers();

        // Returns dictionary of all children by names
        Map<String, Ref<Widget>> GetAllChilds();

        // Returns dictionary of all internal widgets by names
        Map<String, Ref<Widget>> GetAllInternalWidgets();

        // Returns dictionary of all states by names
        Map<String, Ref<WidgetState>> GetAllStates();

#if IS_SCRIPTING_SUPPORTED
    public:
        // Returns layout for scripting @SCRIPTABLE_NAME(GetLayout)
        WidgetLayout* GetLayoutScript();
#endif

        friend class ContextMenu;
        friend class CustomDropDown;
        friend class CustomList;
        friend class CustomList2;
        friend class DropDown;
        friend class EditBox;
        friend class GridLayout;
        friend class GridLayoutScrollArea;
        friend class HorizontalLayout;
        friend class HorizontalProgress;
        friend class HorizontalScrollBar;
        friend class Label;
        friend class List;
        friend class LongList;
        friend class MenuPanel;
        friend class PopupWidget;
        friend class Scene;
        friend class ScrollArea;
        friend class Spoiler;
        friend class Tree;
        friend class TreeNode;
        friend class UIManager;
        friend class VerticalLayout;
        friend class VerticalProgress;
        friend class VerticalScrollBar;
        friend class WidgetLayer;
        friend class WidgetLayout;
        friend class Window;

        FRIEND_REF_MAKE();

#if IS_EDITOR
    public:
        class LayersEditable : public SceneEditableObject
        {
        public:
            WeakRef<Widget> widget;

            SceneUID UID = Math::Random();

            WeakRef<LayersEditable> prototypeLink;

        public:
            // Default constructor
            LayersEditable(RefCounter* refCounter);

            // Default constructor with own widget
            LayersEditable(RefCounter* refCounter, const Ref<Widget>& widget);

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

            // Returns is that type of object can be deleted from editor
            bool IsSupportsDeleting() const override;

            // Returns transform, override when it's supports
            Basis GetTransform() const override;

            SERIALIZABLE(LayersEditable);

            friend class Widget;
        };

        class InternalChildrenEditableEditable : public SceneEditableObject
        {
        public:
            WeakRef<Widget> widget;

            SceneUID UID = Math::Random();

            WeakRef<InternalChildrenEditableEditable> prototypeLink;

        public:
            // Default constructor
            InternalChildrenEditableEditable(RefCounter* refCounter);

            // Default constructor with own widget
            InternalChildrenEditableEditable(RefCounter* refCounter, const Ref<Widget>& widget);

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

            // Sets parent object. nullptr means make this object as root
            void SetEditableParent(const Ref<SceneEditableObject>& object, int idx = -1) override;

            // Adds child. idx is place in parent children. idx == -1 means last
            void AddEditableChild(const Ref<SceneEditableObject>& object, int idx = -1) override;

            // Sets index in siblings - children of parent
            void SetIndexInSiblings(int idx) override;

            // Returns is that type of object can be deleted from editor
            bool IsSupportsDeleting() const override;

            // Returns transform, override when it's supports
            Basis GetTransform() const override;

            SERIALIZABLE(InternalChildrenEditableEditable);

            friend class Widget;
        };

    public:
        static bool isEditorLayersVisible;           // Is widgets layers visible in hierarchy
        static bool isEditorInternalChildrenVisible; // Is widgets internal children visible in hierarchy

        Ref<LayersEditable>                   layersEditable;           // @EDITOR_IGNORE
        Ref<InternalChildrenEditableEditable> internalChildrenEditable; // @EDITOR_IGNORE

    public:
        // Sets parent object. nullptr means make this object as root
        void SetEditableParent(const Ref<SceneEditableObject>& object, int idx = -1) override;

        // Returns object's parent object. Return nullptr when it is a root scene object
        Ref<SceneEditableObject> GetEditableParent() const override;

        // Returns list of object's children
        Vector<Ref<SceneEditableObject>> GetEditableChildren() const override;

        // Adds child. idx is place in parent children. idx == -1 means last
        void AddEditableChild(const Ref<SceneEditableObject>& object, int idx = -1) override;

        // Returns is that type of object can be transformed
        bool IsSupportsTransforming() const override;

        // Returns transform, override when it's supports
        Basis GetTransform() const override;

        // Sets transform of object, override when it's supports
        void SetTransform(const Basis& transform) override;

        // Returns is that type of object can be transformed with layout
        bool IsSupportsLayout() const override;

        // Returns layout, override when it's supports
        Layout GetLayout() const override;

        // Sets layout of object, override when it's supports
        void SetLayout(const Layout& layout) override;

        // Returns pointer to owner editable object
        Ref<SceneEditableObject> GetEditableOwner() override;

    protected:
        // Initializes layers and internal children editables
        void InitEditables();

        friend class LayersEditable;
        friend class InternalChildrenEditableEditable;
#endif // IS_EDITOR
    };
}

#include "o2/Scene/UI/WidgetLayer.h"

namespace o2
{
    template<typename _type>
    Ref<_type> Widget::GetLayerDrawableByType()
    {
        for (auto& layer : mLayers)
        {
            if (layer->GetDrawable() && layer->GetDrawable()->GetType() == TypeOf(_type))
                return DynamicCast<_type>(layer->GetDrawable());
        }

        for (auto& layer : mLayers)
        {
            auto res = layer->FindChild<_type>();
            if (res)
                return res;
        }

        return nullptr;
    }

    template<typename _type>
    Ref<_type> Widget::GetLayerDrawable(const String& path) const
    {
        auto layer = GetLayer(path);
        if (layer && layer->GetDrawable() && layer->GetDrawable()->GetType() == TypeOf(_type))
            return DynamicCast<_type>(layer->GetDrawable());

        return nullptr;
    }

    template<typename _type>
    Ref<_type> Widget::GetInternalWidgetByType(const String& path) const
    {
        return DynamicCast<_type>(GetInternalWidget(path));
    }

    template<typename _type>
    Ref<_type> Widget::FindInternalWidgetByType(const String& name) const
    {
        for (auto& widget : mInternalWidgets)
        {
            if (widget->GetName() == name)
            {
                if (auto res = DynamicCast<_type>(widget))
                    return res;
            }

            if (auto res = widget->FindChildByTypeAndName<_type>((String)name))
                return res;

            if (auto internalWidget = widget->FindInternalWidget(name))
            {
                if (auto res = DynamicCast<_type>(internalWidget))
                    return res;
            }
        }

        return nullptr;
    }

    template<typename _type>
    Ref<_type> Widget::FindInternalWidgetByType() const
    {
        for (auto& widget : mInternalWidgets)
        {
            if (auto res = DynamicCast<_type>(widget))
                return res;

            if (auto res = widget->FindChildByType<_type>())
                return res;

            if (auto res = widget->FindInternalWidgetByType<_type>())
                return res;
        }

        return nullptr;
    }

}
// --- META ---

CLASS_BASES_META(o2::Widget)
{
    BASE_CLASS(o2::Actor);
}
END_META;
CLASS_FIELDS_META(o2::Widget)
{
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().NAME(enabledForcibly);
    FIELD().PUBLIC().NAME(transparency);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().NAME(resTransparency);
    FIELD().PUBLIC().NAME(childrenWidgets);
    FIELD().PUBLIC().NAME(layers);
    FIELD().PUBLIC().NAME(states);
    FIELD().PUBLIC().NAME(childWidget);
    FIELD().PUBLIC().NAME(internalWidget);
    FIELD().PUBLIC().NAME(layer);
    FIELD().PUBLIC().NAME(state);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(layout);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(onLayoutUpdated);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(onFocused);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(onUnfocused);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(onShow);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(onHide);
    FIELD().PROTECTED().DEFAULT_TYPE_ATTRIBUTE(o2::WidgetLayer).DONT_DELETE_ATTRIBUTE().NAME(mLayers);
    FIELD().PROTECTED().DEFAULT_TYPE_ATTRIBUTE(o2::WidgetState).DONT_DELETE_ATTRIBUTE().EDITOR_PROPERTY_ATTRIBUTE().INVOKE_ON_CHANGE_ATTRIBUTE(OnStatesListChanged).NAME(mStates);
    FIELD().PROTECTED().NAME(mParentWidget);
    FIELD().PROTECTED().DEFAULT_TYPE_ATTRIBUTE(o2::Widget).DONT_DELETE_ATTRIBUTE().NAME(mChildWidgets);
    FIELD().PROTECTED().DEFAULT_TYPE_ATTRIBUTE(o2::Widget).DONT_DELETE_ATTRIBUTE().NAME(mInternalWidgets);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mTransparency);
    FIELD().PROTECTED().DEFAULT_VALUE(1.0f).NAME(mResTransparency);
    FIELD().PROTECTED().NAME(mDrawingLayers);
    FIELD().PROTECTED().DEFAULT_TYPE_ATTRIBUTE(o2::WidgetState).DONT_DELETE_ATTRIBUTE().NAME(mFocusedState);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsFocused);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mIsFocusable);
    FIELD().PROTECTED().DEFAULT_TYPE_ATTRIBUTE(o2::WidgetState).DONT_DELETE_ATTRIBUTE().NAME(mVisibleState);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsClipped);
    FIELD().PROTECTED().NAME(mBounds);
    FIELD().PROTECTED().NAME(mBoundsWithChilds);
#if  IS_EDITOR
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(layersEditable);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(internalChildrenEditable);
#endif
}
END_META;
CLASS_METHODS_META(o2::Widget)
{

    typedef Map<String, Ref<WidgetLayer>> _tmp1;
    typedef Map<String, Ref<Widget>> _tmp2;
    typedef Map<String, Ref<Widget>> _tmp3;
    typedef Map<String, Ref<WidgetState>> _tmp4;

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(RefCounter*, ActorCreateMode);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AssetRef<ActorAsset>&, ActorCreateMode);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, Vector<Ref<Component>>, ActorCreateMode);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Widget&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateChildren, float);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateTransform);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateChildrenTransforms);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, ForceDraw, const RectF&, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetLayoutDirty);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const WeakRef<Widget>&, GetParentWidget);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const RectF&, GetChildrenWorldRect);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<Widget>, GetChildWidget, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<Actor>, AddChild, const Ref<Actor>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<Actor>, AddChild, const Ref<Actor>&, int);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<Widget>, AddChildWidget, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, AddChildWidget, const Ref<Widget>&, int);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<Widget>>&, GetChildWidgets);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetIndexInSiblings, int);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<WidgetLayer>, AddLayer, const Ref<WidgetLayer>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<WidgetLayer>, AddLayer, const String&, const Ref<IRectDrawable>&, const Layout&, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, RemoveLayer, const Ref<WidgetLayer>&);
    FUNCTION().PUBLIC().SCRIPTABLE_NAME_ATTRIBUTE(RemoveLayerByPath).SIGNATURE(void, RemoveLayer, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, RemoveAllLayers);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<WidgetLayer>, GetLayer, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<WidgetLayer>, FindLayer, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const Vector<Ref<WidgetLayer>>&, GetLayers);
    FUNCTION().PUBLIC().SCRIPTABLE_NAME_ATTRIBUTE(AddStateNew).SIGNATURE(Ref<WidgetState>, AddState, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_NAME_ATTRIBUTE(AddStateWithClip).SIGNATURE(Ref<WidgetState>, AddState, const String&, const Ref<AnimationClip>&);
    FUNCTION().PUBLIC().SCRIPTABLE_NAME_ATTRIBUTE(AddStateWithClipAsset).SIGNATURE(Ref<WidgetState>, AddState, const String&, const AssetRef<AnimationAsset>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<WidgetState>, AddState, const Ref<WidgetState>&, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_NAME_ATTRIBUTE(RemoveStateByName).SIGNATURE(bool, RemoveState, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, RemoveState, const Ref<WidgetState>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, RemoveAllStates);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetState, const String&, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetStateForcible, const String&, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, GetState, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<WidgetState>, GetStateObject, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const Vector<Ref<WidgetState>>&, GetStates);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTransparency, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTransparency);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetResTransparency);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetEnabledForcible, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Show, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Hide, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Focus);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Unfocus);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsFocused);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsFocusable);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetFocusable, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetInternalParent, const Ref<Widget>&, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, AddInternalWidget, const Ref<Widget>&, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, RemoveInternalWidget, const Ref<Widget>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<Widget>, GetInternalWidget, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<Widget>, FindInternalWidget, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<Actor>, FindActorById, SceneUID);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, SerializeRaw, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, DeserializeRaw, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, SerializeWithProto, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, DeserializeWithProto, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateResEnabled, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateResEnabledInHierarchy, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnParentChanged, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildrenChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildAdded, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildRemoved, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawLayers);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawTopLayers);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawInternalChildren);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateChildWidgetsList);
    FUNCTION().PROTECTED().SIGNATURE(WidgetLayoutData&, GetLayoutData);
    FUNCTION().PROTECTED().SIGNATURE(const WidgetLayoutData&, GetLayoutData);
    FUNCTION().PROTECTED().SIGNATURE(void, SetChildrenWorldRect, const RectF&);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveAndCheckClipping, const Vec2F&, const RectF&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildAdded, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildRemoved, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFocused);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUnfocused);
    FUNCTION().PROTECTED().SIGNATURE(float, GetMinWidthWithChildren);
    FUNCTION().PROTECTED().SIGNATURE(float, GetMinHeightWithChildren);
    FUNCTION().PROTECTED().SIGNATURE(float, GetWidthWeightWithChildren);
    FUNCTION().PROTECTED().SIGNATURE(float, GetHeightWeightWithChildren);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateBounds);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateBoundsWithChilds);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckClipping, const RectF&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTransparency);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateVisibility, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayersLayouts);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildFocused, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLayerAdded, const Ref<WidgetLayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateAdded, const Ref<WidgetState>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStatesListChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawDebugFrame);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayersDrawingSequence);
    FUNCTION().PROTECTED().SIGNATURE(void, RetargetStatesAnimations);
    FUNCTION().PROTECTED().SIGNATURE(void, SetParentWidget, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Ref<Widget>>&, GetChildrenNonConst);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Ref<WidgetLayer>>&, GetLayersNonConst);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Ref<WidgetState>>&, GetStatesNonConst);
    FUNCTION().PROTECTED().SIGNATURE(_tmp1, GetAllLayers);
    FUNCTION().PROTECTED().SIGNATURE(_tmp2, GetAllChilds);
    FUNCTION().PROTECTED().SIGNATURE(_tmp3, GetAllInternalWidgets);
    FUNCTION().PROTECTED().SIGNATURE(_tmp4, GetAllStates);
#if  IS_SCRIPTING_SUPPORTED
    FUNCTION().PUBLIC().SCRIPTABLE_NAME_ATTRIBUTE(GetLayout).SIGNATURE(WidgetLayout*, GetLayoutScript);
#endif
#if  IS_EDITOR
    FUNCTION().PUBLIC().SIGNATURE(void, SetEditableParent, const Ref<SceneEditableObject>&, int);
    FUNCTION().PUBLIC().SIGNATURE(Ref<SceneEditableObject>, GetEditableParent);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<SceneEditableObject>>, GetEditableChildren);
    FUNCTION().PUBLIC().SIGNATURE(void, AddEditableChild, const Ref<SceneEditableObject>&, int);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsTransforming);
    FUNCTION().PUBLIC().SIGNATURE(Basis, GetTransform);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTransform, const Basis&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsLayout);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<SceneEditableObject>, GetEditableOwner);
    FUNCTION().PROTECTED().SIGNATURE(void, InitEditables);
#endif
}
END_META;

#if  IS_EDITOR
CLASS_BASES_META(o2::Widget::LayersEditable)
{
    BASE_CLASS(o2::SceneEditableObject);
}
END_META;
CLASS_FIELDS_META(o2::Widget::LayersEditable)
{
#if  IS_EDITOR
    FIELD().PUBLIC().NAME(widget);
    FIELD().PUBLIC().DEFAULT_VALUE(Math::Random()).NAME(UID);
    FIELD().PUBLIC().NAME(prototypeLink);
#endif
}
END_META;
CLASS_METHODS_META(o2::Widget::LayersEditable)
{

#if  IS_EDITOR
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Ref<Widget>&);
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
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsDeleting);
    FUNCTION().PUBLIC().SIGNATURE(Basis, GetTransform);
#endif
}
END_META;
#endif

#if  IS_EDITOR
CLASS_BASES_META(o2::Widget::InternalChildrenEditableEditable)
{
    BASE_CLASS(o2::SceneEditableObject);
}
END_META;
CLASS_FIELDS_META(o2::Widget::InternalChildrenEditableEditable)
{
#if  IS_EDITOR
    FIELD().PUBLIC().NAME(widget);
    FIELD().PUBLIC().DEFAULT_VALUE(Math::Random()).NAME(UID);
    FIELD().PUBLIC().NAME(prototypeLink);
#endif
}
END_META;
CLASS_METHODS_META(o2::Widget::InternalChildrenEditableEditable)
{

#if  IS_EDITOR
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Ref<Widget>&);
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
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsDeleting);
    FUNCTION().PUBLIC().SIGNATURE(Basis, GetTransform);
#endif
}
END_META;
#endif
// --- END META ---
