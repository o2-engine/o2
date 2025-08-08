#pragma once

#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/ITrackControl.h"

using namespace o2;

namespace o2
{
    class AnimationClip;
}

namespace Editor
{
    FORWARD_CLASS_REF(AnimationWindow);
    FORWARD_CLASS_REF(KeyHandlesSheet);

    // ---------------------
    // animation tracks tree
    // ---------------------
    class AnimationTree : public Tree
    {
    public:
        // Default constructor
        AnimationTree(RefCounter* refCounter);

        // Copy-constructor
        AnimationTree(RefCounter* refCounter, const AnimationTree& other);

        // Copy-constructor
        AnimationTree(const AnimationTree& other);

        // Destructor. Clearing tree data
        ~AnimationTree();

        // Copy-operator
        AnimationTree& operator=(const AnimationTree& other);

        // Draws widget
        void Draw() override;

        // Sets animation and updates tree structure
        void SetAnimation(const Ref<AnimationClip>& animation);

        // Called when animation changed, checks count of animation tracks, updates tree structure if needed
        void OnAnimationChanged();

        // Sets width of tree part
        void SetTreeWidth(float width);

        // Sets color of track by path
        void SetAnimationValueColor(const String& path, const Color4& color);

        // Returns Animation track line number by world position, dependent on scroll
        float GetLineNumber(float worldPosition) const;

        // Returns world position of Animation track line
        float GetLineWorldPosition(float lineNumber) const;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(AnimationTree);
        CLONEABLE_REF(AnimationTree);

    public:
        struct TrackNode: public RefCounterable
        {
            String name;
            String path;

            Color4 color;

            Ref<IAnimationTrack>          track;
            Ref<IAnimationTrack::IPlayer> player;

            Ref<ITrackControl> trackControl;

            WeakRef<TrackNode>     parent;
            Vector<Ref<TrackNode>> children;
        };

    private:
        Ref<AnimationWindow> mAnimationWindow; // Animation window

        int mAnimationValuesCount = 0; // Last stored animation tracks count. Used in checking of animation changes for tracking new values

        Ref<TrackNode> mRootValue;     // Root animation properties tree node
        Ref<ContextMenu> mContextMenu; // Context menu

        Vector<Ref<TrackNode>> mPrevSelectedNodes; // Previous selected nodes before new selection

        float mTreeWidth = 100.0f; // Tree - part width

    private:
        // Initializes context menu
        void InitializeContext();

        // Rebuilds animation tracks tree - mRootValues
        void RebuildAnimationTree();

        // Adds Animation track to tree. Creates intermediate nodes when required
        void AddAnimationTrack(const Ref<IAnimationTrack>& track, const Ref<IAnimationTrack::IPlayer>& player = nullptr);

        // Updates tree node width
        void UpdateTreeWidth();

        // Sets curve view mode
        void SetCurveViewMode(bool enable);

        // Returns object's parent
        void* GetObjectParent(void* object) override;

        // Returns object's children
        Vector<void*> GetObjectChilds(void* object) override;

        // Returns debugging string for object
        String GetObjectDebug(void* object) override;

        // Sets nodeWidget data by object
        void FillNodeDataByObject(const Ref<TreeNode>& nodeWidget, void* object) override;

        // Free node data
        void FreeNodeData(const Ref<TreeNode>& nodeWidget, void* object) override;

        // Updates visible nodes (calculates range and initializes nodes), updates tree width on visible nodes
        void UpdateVisibleNodes() override;

        // Called when tree node clicked by right button, opening context menu
        void OnNodeRBClick(const Ref<TreeNode>& node) override;

        // Called when list of selected objects was changed
        void OnNodesSelectionChanged(Vector<void*> objects) override;

        // Gets tree node from pool or creates new, in editor scope
        Ref<TreeNode> CreateTreeNodeWidget() override;

        // Removes selected property
        void OnDeletePropertyPressed();

        friend class AnimationTreeNode;
        friend class AnimationWindow;
    };

    // -------------------
    // Animation tree node
    // -------------------
    class AnimationTreeNode : public TreeNode
    {
    public:
        // Default constructor
        AnimationTreeNode(RefCounter* refCounter);

        // Copy-constructor
        AnimationTreeNode(RefCounter* refCounter, const AnimationTreeNode& other);

        // Copy operator
        AnimationTreeNode& operator=(const AnimationTreeNode& other);

        // Sets object and updates track control
        void Setup(const Ref<AnimationTree::TrackNode>& node, const Ref<AnimationTimeline>& timeline, const Ref<KeyHandlesSheet>& handlesSheet);

        // Free node, unregister track control
        void Free();

        // Sets width of tree part and control part
        void SetTreeWidth(float width);

        // Called from handles sheet, when user double clicked, creates new key under cursor
        void OnDoubleClicked(const Input::Cursor& cursor);

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(AnimationTreeNode);
        CLONEABLE_REF(AnimationTreeNode);

    protected:
        float mPropertyBorder = 2.0f;
        float mAddKeyButtonSize = 25.0f;
        float mPropertySize = 130.0f;

        Ref<AnimationTree::TrackNode> mData; // Editing Animation track data

        Ref<AnimationTimeline> mTimeline;     // Animation timeline pointer, passes into track controller
        Ref<KeyHandlesSheet>   mHandlesSheet; // Handles sheet group, passes into track controller

        Ref<Text> mNameDrawable; // Object name drawable

        Ref<ITrackControl> mTrackControl;     // Animation track editor

        static Map<const Type*, Vector<Ref<ITrackControl>>> mTrackControlsCache; // Shared track controls cache

    protected:
        // Called on deserialization, initializes controls
        void OnDeserialized(const DataValue& node) override;

        // initializes controls and widgets
        void InitializeControls();

        // Initializes suitable track control for Animation track by type. Caching track controls
        void InitilizeTrackControl();

        // Free track control and put it unto buffer
        void FreeTrackControl();

        // Updates drag handles positions on timeline
        void UpdateTrackControlView();

        friend class AnimationTree;
    };
}
// --- META ---

CLASS_BASES_META(Editor::AnimationTree)
{
    BASE_CLASS(o2::Tree);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationTree)
{
    FIELD().PRIVATE().NAME(mAnimationWindow);
    FIELD().PRIVATE().DEFAULT_VALUE(0).NAME(mAnimationValuesCount);
    FIELD().PRIVATE().NAME(mRootValue);
    FIELD().PRIVATE().NAME(mContextMenu);
    FIELD().PRIVATE().NAME(mPrevSelectedNodes);
    FIELD().PRIVATE().DEFAULT_VALUE(100.0f).NAME(mTreeWidth);
}
END_META;
CLASS_METHODS_META(Editor::AnimationTree)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AnimationTree&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const AnimationTree&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAnimation, const Ref<AnimationClip>&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnAnimationChanged);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTreeWidth, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAnimationValueColor, const String&, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLineNumber, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLineWorldPosition, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeContext);
    FUNCTION().PRIVATE().SIGNATURE(void, RebuildAnimationTree);
    FUNCTION().PRIVATE().SIGNATURE(void, AddAnimationTrack, const Ref<IAnimationTrack>&, const Ref<IAnimationTrack::IPlayer>&);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateTreeWidth);
    FUNCTION().PRIVATE().SIGNATURE(void, SetCurveViewMode, bool);
    FUNCTION().PRIVATE().SIGNATURE(void*, GetObjectParent, void*);
    FUNCTION().PRIVATE().SIGNATURE(Vector<void*>, GetObjectChilds, void*);
    FUNCTION().PRIVATE().SIGNATURE(String, GetObjectDebug, void*);
    FUNCTION().PRIVATE().SIGNATURE(void, FillNodeDataByObject, const Ref<TreeNode>&, void*);
    FUNCTION().PRIVATE().SIGNATURE(void, FreeNodeData, const Ref<TreeNode>&, void*);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateVisibleNodes);
    FUNCTION().PRIVATE().SIGNATURE(void, OnNodeRBClick, const Ref<TreeNode>&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnNodesSelectionChanged, Vector<void*>);
    FUNCTION().PRIVATE().SIGNATURE(Ref<TreeNode>, CreateTreeNodeWidget);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDeletePropertyPressed);
}
END_META;

CLASS_BASES_META(Editor::AnimationTreeNode)
{
    BASE_CLASS(o2::TreeNode);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationTreeNode)
{
    FIELD().PROTECTED().DEFAULT_VALUE(2.0f).NAME(mPropertyBorder);
    FIELD().PROTECTED().DEFAULT_VALUE(25.0f).NAME(mAddKeyButtonSize);
    FIELD().PROTECTED().DEFAULT_VALUE(130.0f).NAME(mPropertySize);
    FIELD().PROTECTED().NAME(mData);
    FIELD().PROTECTED().NAME(mTimeline);
    FIELD().PROTECTED().NAME(mHandlesSheet);
    FIELD().PROTECTED().NAME(mNameDrawable);
    FIELD().PROTECTED().NAME(mTrackControl);
}
END_META;
CLASS_METHODS_META(Editor::AnimationTreeNode)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AnimationTreeNode&);
    FUNCTION().PUBLIC().SIGNATURE(void, Setup, const Ref<AnimationTree::TrackNode>&, const Ref<AnimationTimeline>&, const Ref<KeyHandlesSheet>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Free);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTreeWidth, float);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDoubleClicked, const Input::Cursor&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, InitilizeTrackControl);
    FUNCTION().PROTECTED().SIGNATURE(void, FreeTrackControl);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTrackControlView);
}
END_META;
// --- END META ---
