#pragma once

#include "o2/Assets/AssetInfo.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Events/DrawableCursorEventsListener.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Scene/UI/Widgets/GridLayoutScrollArea.h"
#include "o2/Utils/Editor/DragAndDrop.h"
#include "o2/Utils/FileSystem/FileInfo.h"
#include "o2/Utils/Types/Containers/Pair.h"

using namespace o2;

namespace o2
{
    class SceneEditableObject;
    class Sprite;
    class ContextMenu;
    class GridLayout;
    class Button;
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
        // How the assets are shown: icons in a grid, rows of a list or rows of a table with the columns
        enum class ViewMode { Grid, List, Table };

        // Key the assets are sorted by; folders go first
        enum class SortBy { Name, Type, Size, Modified };

    public:
        Function<void(const Vector<String>&)> onAssetsSelected; // Assets selected event
        Function<void()>                      onViewChanged;    // Called when the view mode or the sorting was changed by user

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

        // Shows the given assets as a flat list (search results)
        void ShowSearchResults(const Vector<Ref<AssetInfo>>& matches);

        // Creates asset and starts renaming
        void CreateAsset(const Type* assetType);

        // Highlights asset
        void HighlightAsset(const UID& id);

        // Selects asset
        void SelectAsset(const UID& id, bool scrsoll = true);

        // Deselects all assets
        void DeselectAllAssets();

        // Returns context menu
        const Ref<ContextMenu>& GetContextMenu() const;

        // Instantiates the actor asset into the scene with an undo action
        // (the double-click "open" behaviour for prototypes)
        void InstantiateAssetIntoScene(const AssetInfo& assetInfo);

        // Returns selected assets infos
        const Vector<Ref<AssetInfo>>& GetSelectedAssets() const;

        // Returns asset infos in the shown order
        const Vector<Ref<AssetInfo>>& GetAssetInfos() const;

        // Returns visible asset icons
        const Vector<Ref<AssetIcon>>& GetVisibleIcons() const;

        // Sets how the assets are shown; the selection stays
        void SetViewMode(ViewMode mode);

        // Returns how the assets are shown
        ViewMode GetViewMode() const;

        // Sets the key the assets are sorted by
        void SetSort(SortBy sortBy, bool descending = false);

        // Returns the key the assets are sorted by
        SortBy GetSortBy() const;

        // Returns is the sorting descending
        bool IsSortDescending() const;

        // Returns the header of the table with the columns buttons, null before the table is shown
        const Ref<Widget>& GetTableHeader() const;

        // Returns the type of the asset as the table shows it
        static WString GetTypeText(const AssetInfo& info);

        // Returns the size of the file as the table shows it, empty for a folder
        static WString GetSizeText(const AssetInfo& info, const FileInfo& file);

        // Returns the modification date of the file as the table shows it
        static WString GetDateText(const FileInfo& file);

        // Return asset icon under point
        Ref<AssetIcon> GetIconUnderPoint(const Vec2F& point) const;

        // Sets icon highlight drawable
        void SetHighlightDrawable(const Ref<IRectDrawable>& drawable);

        // Returns icon highlight drawable
        const Ref<IRectDrawable>& GetHighlightDrawable() const;

        // Sets highlight animation
        void SetHighlightAnimation(const Ref<AnimationClip>& animation);

        // Sets highlight layout
        void SetHighlightLayout(const Layout& layout);

        // Sets drawable of the hover: it slides to the icon under the cursor, as the one of a tree does
        void SetHoverDrawable(const Ref<IRectDrawable>& drawable);

        // Returns drawable of the hover
        const Ref<IRectDrawable>& GetHoverDrawable() const;

        // Sets layout of the hover around the icon
        void SetHoverLayout(const Layout& layout);

        // Returns the icon the hover goes to, null when it fades out
        const Ref<AssetIcon>& GetHoveredIcon() const;

        // Starts renaming of the selected asset: the name of its icon turns into an edit box
        void RenameSelectedAsset();

        // Sets selecting rectangle drawable
        void SetSelectingDrawable(const Ref<IRectDrawable>& drawable);

        // Returns selecting rectangle drawable
        const Ref<IRectDrawable>& GetSelectingDrawable() const;

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
        static constexpr float tableHeaderHeight = 20.0f; // Height of the header of the table
                                
        String mCurrentPath = "_"; // Current viewing path

        ViewMode mViewMode = ViewMode::Grid; // How the assets are shown
        SortBy   mSortBy = SortBy::Name;     // Key the assets are sorted by
        bool     mSortDescending = false;    // Is the sorting descending

        Ref<Widget>         mTableHeader;        // Header of the table with the columns buttons
        Vector<Ref<Button>> mTableHeaderButtons; // Buttons of the columns: name, type, size, modification date

        bool   mBaseLayoutsStored = false; // Are the layouts of the grid stored
        Layout mGridViewLayout;            // View layout of the grid
        Layout mGridClippingLayout;        // Clipping layout of the grid
        Vec2F  mGridItemsSpacing;          // Items spacing of the grid

        Map<UID, FileInfo> mFileInfos; // Files of the shown assets: the size and the modification date

        Vector<Ref<AssetInfo>> mAssetInfos;        // Asset infos in path @IGNORE
        Vector<Ref<AssetIcon>> mVisibleAssetIcons; // Visible asset icons

        Ref<ContextMenu> mContextMenu; // Assets Context menu
                                
        Vector<Ref<AssetInfo>>  mSelectedAssets;          // Selected assets icons @IGNORE
        Vector<AssetRef<Asset>> mSelectedPreloadedAssets; // Preloaded selected assets

        AssetRef<Asset> mNewAsset; // Temporary new asset. Used when creating new asset
                                
        Ref<AssetIcon>       mHighlightIcon;                            // Current highlighting asset icon
        Ref<AnimationClip>   mHighlighClip;                             // Node highlight animation clip @SERIALIZABLE 
        Ref<AnimationPlayer> mHighlightAnim = mmake<AnimationPlayer>(); // Icon highlight animation
        Ref<IRectDrawable>   mHighlightSprite;                          // Icon highlight drawable @SERIALIZABLE
        Layout               mHighlightLayout;                          // Icon highlight sprite layout @SERIALIZABLE
                                
        Ref<IRectDrawable> mHoverDrawable;    // Hover drawable @SERIALIZABLE
        Layout             mHoverLayout;      // Layout of the hover around the icon @SERIALIZABLE
        RectF              mCurrentHoverRect; // Current hover rectangle, slides to the target one
        RectF              mTargetHoverRect;  // Hover rectangle around the icon under the cursor
        Ref<AssetIcon>     mHoveredIcon;      // Icon under the cursor

        Map<String, Vector<Ref<AssetIcon>>> mIconsPool; // Assets icons pool
                                
        Ref<IRectDrawable>     mSelectionSprite;       // Icons selection rectangle drawable @SERIALIZABLE
        bool                   mSelecting = false;     // Is selecting icons 
        Vec2F                  mPressedPoint;          // Pressed point
        Vector<Ref<AssetInfo>> mCurrentSelectingInfos; // Selecting icons at current selection @IGNORE
                                
        bool                             mIsDraggingIcons = false;      // Is dragging icons
        bool                             mDragEnded = false;            // Is dragging ended
        Ref<AssetIcon>                   mDragIcon;                     // Dragging icon
        Vec2F                            mDragIconSize;                 // Size of the dragging icon
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

        // Called when cursor exits this object, the hover fades out
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Moves the hover to the icon, or fades it out for none
        void UpdateHover(const Ref<AssetIcon>& icon);

        // Moves scroll position and the hover with it
        void MoveScrollPosition(const Vec2F& delta) override;

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

        // Shows the frame of a video in its icon
        void SetupMediaIcon(const Ref<AssetIcon>& icon, const Ref<AssetInfo>& info);

        // Returns the file of the asset, read once per shown folder
        const FileInfo& GetFileInfo(const Ref<AssetInfo>& info);

        // Creates the header of the table
        void InitializeTableHeader();

        // Shows the sorting key and direction in the header of the table
        void UpdateTableHeader();

        // Draws the light lines between the columns of the table, from the header to the last row
        void DrawTableLines(bool header);

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

        // Starts renaming of the asset of the icon by its name
        void RenameAsset(const Ref<AssetIcon>& icon);

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

PRE_ENUM_META(Editor::AssetsIconsScrollArea::ViewMode);

PRE_ENUM_META(Editor::AssetsIconsScrollArea::SortBy);

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
    FIELD().PUBLIC().NAME(onViewChanged);
    FIELD().PROTECTED().DEFAULT_VALUE("_").NAME(mCurrentPath);
    FIELD().PROTECTED().DEFAULT_VALUE(ViewMode::Grid).NAME(mViewMode);
    FIELD().PROTECTED().DEFAULT_VALUE(SortBy::Name).NAME(mSortBy);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSortDescending);
    FIELD().PROTECTED().NAME(mTableHeader);
    FIELD().PROTECTED().NAME(mTableHeaderButtons);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mBaseLayoutsStored);
    FIELD().PROTECTED().NAME(mGridViewLayout);
    FIELD().PROTECTED().NAME(mGridClippingLayout);
    FIELD().PROTECTED().NAME(mGridItemsSpacing);
    FIELD().PROTECTED().NAME(mFileInfos);
    FIELD().PROTECTED().NAME(mVisibleAssetIcons);
    FIELD().PROTECTED().NAME(mContextMenu);
    FIELD().PROTECTED().NAME(mSelectedPreloadedAssets);
    FIELD().PROTECTED().NAME(mNewAsset);
    FIELD().PROTECTED().NAME(mHighlightIcon);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHighlighClip);
    FIELD().PROTECTED().DEFAULT_VALUE(mmake<AnimationPlayer>()).NAME(mHighlightAnim);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHighlightSprite);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHighlightLayout);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHoverDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHoverLayout);
    FIELD().PROTECTED().NAME(mCurrentHoverRect);
    FIELD().PROTECTED().NAME(mTargetHoverRect);
    FIELD().PROTECTED().NAME(mHoveredIcon);
    FIELD().PROTECTED().NAME(mIconsPool);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectionSprite);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSelecting);
    FIELD().PROTECTED().NAME(mPressedPoint);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsDraggingIcons);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mDragEnded);
    FIELD().PROTECTED().NAME(mDragIcon);
    FIELD().PROTECTED().NAME(mDragIconSize);
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
    FUNCTION().PUBLIC().SIGNATURE(void, ShowSearchResults, const Vector<Ref<AssetInfo>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, CreateAsset, const Type*);
    FUNCTION().PUBLIC().SIGNATURE(void, HighlightAsset, const UID&);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectAsset, const UID&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, DeselectAllAssets);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ContextMenu>&, GetContextMenu);
    FUNCTION().PUBLIC().SIGNATURE(void, InstantiateAssetIntoScene, const AssetInfo&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<AssetInfo>>&, GetSelectedAssets);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<AssetInfo>>&, GetAssetInfos);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<AssetIcon>>&, GetVisibleIcons);
    FUNCTION().PUBLIC().SIGNATURE(void, SetViewMode, ViewMode);
    FUNCTION().PUBLIC().SIGNATURE(ViewMode, GetViewMode);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSort, SortBy, bool);
    FUNCTION().PUBLIC().SIGNATURE(SortBy, GetSortBy);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSortDescending);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Widget>&, GetTableHeader);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(WString, GetTypeText, const AssetInfo&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(WString, GetSizeText, const AssetInfo&, const FileInfo&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(WString, GetDateText, const FileInfo&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<AssetIcon>, GetIconUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHighlightDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetHighlightDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHighlightAnimation, const Ref<AnimationClip>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHighlightLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHoverDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetHoverDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHoverLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<AssetIcon>&, GetHoveredIcon);
    FUNCTION().PUBLIC().SIGNATURE(void, RenameSelectedAsset);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectingDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetSelectingDrawable);
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
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHover, const Ref<AssetIcon>&);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveScrollPosition, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnScrolled, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetsSelected);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateCuttingAssets);
    FUNCTION().PROTECTED().SIGNATURE(void, BeginSelecting);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateSelection, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, CompleteSelecting);
    FUNCTION().PROTECTED().SIGNATURE(void, SortAssetInfos);
    FUNCTION().PROTECTED().SIGNATURE(void, SetupMediaIcon, const Ref<AssetIcon>&, const Ref<AssetInfo>&);
    FUNCTION().PROTECTED().SIGNATURE(const FileInfo&, GetFileInfo, const Ref<AssetInfo>&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeTableHeader);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTableHeader);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawTableLines, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, RegObjectsCreationAction);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeContext);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeCreateContext);
    FUNCTION().PROTECTED().SIGNATURE(Ref<AssetIcon>, GetAssetIconFromPool, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, FreeAssetIconToPool, const Ref<AssetIcon>&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<AssetIcon>, FindVisibleIcon, const Ref<AssetInfo>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetDblClick, const Ref<AssetIcon>&);
    FUNCTION().PROTECTED().SIGNATURE(void, RenameAsset, const Ref<AssetIcon>&);
    FUNCTION().PROTECTED().SIGNATURE(void, StartAssetRenaming, const Ref<AssetIcon>&, const String&, const Function<void(const String&)>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCopyPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCutPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextPastePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextDeletePressed);
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
