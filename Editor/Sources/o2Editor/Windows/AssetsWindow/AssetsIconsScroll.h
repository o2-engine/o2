#pragma once

#include "o2/Assets/AssetInfo.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Events/DrawableCursorEventsListener.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Scene/UI/Widgets/GridLayoutScrollArea.h"
#include "o2/Utils/Editor/DragAndDrop.h"
#include "o2/Utils/Types/Containers/Pair.h"

using namespace o2;

namespace o2
{
    class SceneEditableObject;
    class Sprite;
    class ContextMenu;
    class GridLayout;
}

namespace Editor
{
    FORWARD_CLASS_REF(ActorProperty);
    FORWARD_CLASS_REF(ComponentProperty);
    FORWARD_CLASS_REF(SceneHierarchyTree);
    FORWARD_CLASS_REF(AssetIcon);

    // ------------------------
    // Assets icons scroll area
    // ------------------------
    class AssetsIconsScrollArea: public GridLayoutScrollArea, public DragDropArea, public KeyboardEventsListener,  public ISelectableDragableObjectsGroup
    {
    public:
        Function<void(const Vector<String>&)> onAssetsSelected; // Assets selected event

    public:
        // Default constructor
        AssetsIconsScrollArea(RefCounter* refCounter);

        // Copy-constructor
        AssetsIconsScrollArea(RefCounter* refCounter, const AssetsIconsScrollArea& other);

        // Copy-constructor
        AssetsIconsScrollArea(const AssetsIconsScrollArea& other);

        // Destructor
        ~AssetsIconsScrollArea();

        // Copy-operator
        AssetsIconsScrollArea& operator=(const AssetsIconsScrollArea& other);

        // Draws widget
        void Draw() override;

        // Updates widget
        void Update(float dt) override;

        // Sets current assets viewing path
        void SetViewingPath(const String& path);

        // Returns viewing path
        const String& GetViewingPath() const;

        // Updates assets path contents
        void UpdateAssetsByCurrentPath();

        // Creates asset and starts renaming
        void CreateAsset(const Type* assetType);

        // Highlights asset
        void HighlightAsset(const UID& id);

        // Selects asset
        void SelectAsset(const UID& id, bool scroll = true);

        // Deselects all assets
        void DeselectAllAssets();

        // Returns selected assets infos
        const Vector<Ref<AssetInfo>>& GetSelectedAssets() const;

        // Return asset icon under point
        Ref<AssetIcon> GetIconUnderPoint(const Vec2F& point) const;

        // Returns node highlight drawable
        const Ref<Sprite>& GetHighlightDrawable() const;

        // Sets highlight animation
        void SetHighlightAnimation(const Ref<AnimationClip>& animation);

        // Sets highlight layout
        void SetHighlightLayout(const Layout& layout);

        // Returns selecting rectangle drawable
        const Ref<Sprite>& GetSelectingDrawable() const;

        // Returns is this widget can be selected
        bool IsFocusable() const override;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns is listener scrollable
        bool IsScrollable() const override;

        // Returns true when input events can be handled by down listeners
        bool IsInputTransparent() const override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(AssetsIconsScrollArea);
        CLONEABLE_REF(AssetsIconsScrollArea);

    protected:
        const Vec2F mAssetIconSize = Vec2F(50, 60);
                                
        String mCurrentPath = "_"; // Current viewing path

        Vector<Ref<AssetInfo>> mAssetInfos;        // Asset infos in path @IGNORE
        Vector<Ref<AssetIcon>> mVisibleAssetIcons; // Visible asset icons

        Ref<ContextMenu> mContextMenu; // Assets Context menu
                                
        Vector<Ref<AssetInfo>> mSelectedAssets;          // Selected assets icons @IGNORE
        Vector<AssetRef<Asset>>     mSelectedPreloadedAssets; // Preloaded selected assets

        AssetRef<Asset> mNewAsset; // Temporary new asset. Used when creating new asset
                                
        Ref<AssetIcon>       mHighlightIcon;                            // Current highlighting asset icon
        Ref<AnimationClip>   mHighlighClip;                             // Node highlight animation clip @SERIALIZABLE 
        Ref<AnimationPlayer> mHighlightAnim = mmake<AnimationPlayer>(); // Icon highlight animation
        Ref<Sprite>          mHighlightSprite;                          // Icon highlight sprite @SERIALIZABLE
        Layout               mHighlightLayout;                          // Icon highlight sprite layout @SERIALIZABLE
                                
        Map<String, Vector<Ref<AssetIcon>>> mIconsPool; // Assets icons pool
                                
        Ref<Sprite>            mSelectionSprite;       // Icons selection rectangle sprite @SERIALIZABLE
        bool                   mSelecting = false;     // Is selecting icons 
        Vec2F                  mPressedPoint;          // Pressed point
        Vector<Ref<AssetInfo>> mCurrentSelectingInfos; // Selecting icons at current selection @IGNORE
                                
        bool                             mIsDraggingIcons = false;      // Is dragging icons
        bool                             mDragEnded = false;            // Is dragging ended
        Ref<AssetIcon>                   mDragIcon;                     // Dragging icon
        Vec2F                            mDragOffset;                   // Dragging offset from cursor to icon center
        Vector<Ref<SceneEditableObject>> mInstantiatedSceneDragObjects; // Instantiated objects when dragging asset above scene

        Vector<Pair<UID, String>> mCuttingAssets; // Current cutted assets
                                
        bool mChangePropertiesTargetsFromThis = false;

    protected:
        // Returns items count, calls getItemsCountFunc
        int GetItemsCount() const override;

        // Returns items in range from start to end, calls getItemsRangeFunc
        Vector<void*> GetItemsRange(int start, int end) const override;

        // Sets item widget, calls setupItemFunc
        void SetupItemWidget(const Ref<Widget>& widget, void* item) override;

        // Updates visible items
        void UpdateVisibleItems() override;

        // Called when widget was selected
        void OnFocused() override;

        // Called when widget was deselected
        void OnUnfocused() override;

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor moved on this (or moved outside when this was pressed)
        void OnCursorMoved(const Input::Cursor& cursor) override;

        // Called when right mouse button was released (only when right mouse button pressed this at previous time)
        void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

        // Called when scrolling
        void OnScrolled(float scroll) override;

        // Called when key was released
        void OnKeyReleased(const Input::Key& key) override;

        // Called when assets selection was changed
        void OnAssetsSelected();

        // Updates cutting assets
        void UpdateCuttingAssets();

        // Begins selecting icons
        void BeginSelecting();

        // Updates selection frame
        void UpdateSelection(const Input::Cursor& cursor);

        // Completes selecting
        void CompleteSelecting();

        // Sorts current asset infos
        void SortAssetInfos();

        // Registers objects creation undo action
        void RegObjectsCreationAction();

        // Initializes assets context menu
        void InitializeContext();

        // Initializes assets create context menu
        void InitializeCreateContext();

        // Returns asset icon from pool or creates new by style name
        Ref<AssetIcon> GetAssetIconFromPool(const String& style);

        // Frees icon to pool
        void FreeAssetIconToPool(const Ref<AssetIcon>& icon);

        // Returns asset icon if visible
        Ref<AssetIcon> FindVisibleIcon(const Ref<AssetInfo>& info);

        // Called when asset icon double clicked, starting editing name
        void OnAssetDblClick(const Ref<AssetIcon>& icon);

        // Starts asset icon renaming, calls onCompletedwhen completed
        void StartAssetRenaming(const Ref<AssetIcon>& icon, const String& name, const Function<void(const String&)>& onCompleted);

        // Called when context copy pressed
        void OnContextCopyPressed();

        // Called when context cut pressed
        void OnContextCutPressed();

        // Called when context paste pressed
        void OnContextPastePressed();

        // Called when context delete pressed
        void OnContextDeletePressed();

        // Called when context open pressed
        void OnContextOpenPressed();

        // Called when context show in explorer pressed
        void OnContextShowInExplorerPressed();

        // Instantiates dragging assets
        void InstantiateDraggingAssets();

        // Removes and clears instantiated dragging assets
        void ClearInstantiatedDraggingAssets();

        // Instantiate actor from asset info
        Ref<Actor> InstantiateAsset(const AssetInfo& assetInfo);

        // Dummy asset instantiate function from asset
        template<typename _type>
        Ref<Actor> InstantiateAsset(const _type& asset);

        // Instantiate actor from image asset
        Ref<Actor> InstantiateAsset(const AssetRef<ImageAsset>& asset);

        // Instantiate actor from actor asset
        Ref<Actor> InstantiateAsset(const AssetRef<ActorAsset>& asset);

        // Called when assets was changed from properties
        void OnAssetsPropertiesChanged();

        // Checks preloaded assets for saving
        void CheckPreloadedAssetsSaving();
        
// ISelectableDragableObjectsGroup implementation

        // Returns selected objects in group
        Vector<Ref<SelectableDragableObject>> GetSelectedDragObjects() const override;

        // Returns all objects in group 
        Vector<Ref<SelectableDragableObject>> GetAllObjects() const override;

        // Selects object
        void Select(const Ref<SelectableDragableObject>& object) override;

        // Selects object
        void Select(const Ref<SelectableDragableObject>& object, bool sendOnSelectionChanged);

        // Deselects object
        void Deselect(const Ref<SelectableDragableObject>& object) override;

        // Adds selectable object to group
        void AddSelectableObject(const Ref<SelectableDragableObject>& object) override;

        // Removes selectable object from group
        void RemoveSelectableObject(SelectableDragableObject* object) override;

        // Called when selectable draggable object was released
        void OnSelectableObjectCursorReleased(const Ref<SelectableDragableObject>& object, const Input::Cursor& cursor) override;

        // Called when selectable object was began to drag
        void OnSelectableObjectBeganDragging(const Ref<SelectableDragableObject>& object) override;

// DragDropArea implementation
        // Called when some drag listeners was entered to this area
        void OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some drag listeners was dragged above this area
        void OnDraggedAbove(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some drag listeners was exited from this area
        void OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some selectable listeners was dropped to this
        void OnDropped(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when dropped dragged scene tree nodes selected and started dragging from scene tree
        void OnDroppedFromSceneTree(const Ref<SceneHierarchyTree>& sceneTree);

        // Called when dropped dragged assets icons selected and started dragging from this
        void OnDroppedFromThis();

        // Begins dragging selected items
        void BeginDragging(const Ref<AssetIcon>& icon);

        // Ends dragging items
        void EndDragging(bool droppedToThis = false);

        // Updates dragging graphics
        void UpdateDraggingGraphics(); 
        
        REF_COUNTERABLE_IMPL(GridLayoutScrollArea);

        friend class AssetsWindow;
        friend class SceneEditScreen;
        friend class SceneHierarchyTree;
        friend class AssetIcon;
        friend class SceneTree;
    };

    template<typename _type>
    Ref<Actor> AssetsIconsScrollArea::InstantiateAsset(const _type& asset)
    {
        return nullptr;
    }
}
// --- META ---

CLASS_BASES_META(Editor::AssetsIconsScrollArea)
{
    BASE_CLASS(o2::GridLayoutScrollArea);
    BASE_CLASS(o2::DragDropArea);
    BASE_CLASS(o2::KeyboardEventsListener);
    BASE_CLASS(o2::ISelectableDragableObjectsGroup);
}
END_META;
CLASS_FIELDS_META(Editor::AssetsIconsScrollArea)
{
    FIELD().PUBLIC().NAME(onAssetsSelected);
    FIELD().PROTECTED().DEFAULT_VALUE(Vec2F(50, 60)).NAME(mAssetIconSize);
    FIELD().PROTECTED().DEFAULT_VALUE("_").NAME(mCurrentPath);
    FIELD().PROTECTED().NAME(mVisibleAssetIcons);
    FIELD().PROTECTED().NAME(mContextMenu);
    FIELD().PROTECTED().NAME(mSelectedPreloadedAssets);
    FIELD().PROTECTED().NAME(mNewAsset);
    FIELD().PROTECTED().NAME(mHighlightIcon);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHighlighClip);
    FIELD().PROTECTED().DEFAULT_VALUE(mmake<AnimationPlayer>()).NAME(mHighlightAnim);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHighlightSprite);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHighlightLayout);
    FIELD().PROTECTED().NAME(mIconsPool);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectionSprite);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSelecting);
    FIELD().PROTECTED().NAME(mPressedPoint);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsDraggingIcons);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mDragEnded);
    FIELD().PROTECTED().NAME(mDragIcon);
    FIELD().PROTECTED().NAME(mDragOffset);
    FIELD().PROTECTED().NAME(mInstantiatedSceneDragObjects);
    FIELD().PROTECTED().NAME(mCuttingAssets);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mChangePropertiesTargetsFromThis);
}
END_META;
CLASS_METHODS_META(Editor::AssetsIconsScrollArea)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AssetsIconsScrollArea&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const AssetsIconsScrollArea&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetViewingPath, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetViewingPath);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateAssetsByCurrentPath);
    FUNCTION().PUBLIC().SIGNATURE(void, CreateAsset, const Type*);
    FUNCTION().PUBLIC().SIGNATURE(void, HighlightAsset, const UID&);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectAsset, const UID&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, DeselectAllAssets);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<AssetInfo>>&, GetSelectedAssets);
    FUNCTION().PUBLIC().SIGNATURE(Ref<AssetIcon>, GetIconUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Sprite>&, GetHighlightDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHighlightAnimation, const Ref<AnimationClip>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHighlightLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Sprite>&, GetSelectingDrawable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFocusable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInputTransparent);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(int, GetItemsCount);
    FUNCTION().PROTECTED().SIGNATURE(Vector<void*>, GetItemsRange, int, int);
    FUNCTION().PROTECTED().SIGNATURE(void, SetupItemWidget, const Ref<Widget>&, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateVisibleItems);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFocused);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUnfocused);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnScrolled, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetsSelected);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateCuttingAssets);
    FUNCTION().PROTECTED().SIGNATURE(void, BeginSelecting);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateSelection, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, CompleteSelecting);
    FUNCTION().PROTECTED().SIGNATURE(void, SortAssetInfos);
    FUNCTION().PROTECTED().SIGNATURE(void, RegObjectsCreationAction);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeContext);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeCreateContext);
    FUNCTION().PROTECTED().SIGNATURE(Ref<AssetIcon>, GetAssetIconFromPool, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, FreeAssetIconToPool, const Ref<AssetIcon>&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<AssetIcon>, FindVisibleIcon, const Ref<AssetInfo>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetDblClick, const Ref<AssetIcon>&);
    FUNCTION().PROTECTED().SIGNATURE(void, StartAssetRenaming, const Ref<AssetIcon>&, const String&, const Function<void(const String&)>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCopyPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCutPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextPastePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextDeletePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextOpenPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextShowInExplorerPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, InstantiateDraggingAssets);
    FUNCTION().PROTECTED().SIGNATURE(void, ClearInstantiatedDraggingAssets);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Actor>, InstantiateAsset, const AssetInfo&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Actor>, InstantiateAsset, const AssetRef<ImageAsset>&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Actor>, InstantiateAsset, const AssetRef<ActorAsset>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetsPropertiesChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckPreloadedAssetsSaving);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Ref<SelectableDragableObject>>, GetSelectedDragObjects);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Ref<SelectableDragableObject>>, GetAllObjects);
    FUNCTION().PROTECTED().SIGNATURE(void, Select, const Ref<SelectableDragableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(void, Select, const Ref<SelectableDragableObject>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, Deselect, const Ref<SelectableDragableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(void, AddSelectableObject, const Ref<SelectableDragableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveSelectableObject, SelectableDragableObject*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectableObjectCursorReleased, const Ref<SelectableDragableObject>&, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectableObjectBeganDragging, const Ref<SelectableDragableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragEnter, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraggedAbove, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragExit, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDropped, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDroppedFromSceneTree, const Ref<SceneHierarchyTree>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDroppedFromThis);
    FUNCTION().PROTECTED().SIGNATURE(void, BeginDragging, const Ref<AssetIcon>&);
    FUNCTION().PROTECTED().SIGNATURE(void, EndDragging, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateDraggingGraphics);
}
END_META;
// --- END META ---
