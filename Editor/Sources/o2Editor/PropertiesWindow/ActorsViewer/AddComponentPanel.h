#pragma once

#include "o2/Events/CursorEventsArea.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Tree.h"

using namespace o2;

namespace o2
{
	class EditBox;
	class Button;
}

namespace Editor
{
	class ActorViewer;
	class ComponentsTree;

	// --------------------------------------------------------------------------------
	// Add component to actor panel. Shows filter input, add button and components tree
	// --------------------------------------------------------------------------------
	class AddComponentPanel : public Widget, public CursorEventsArea, public KeyboardEventsListener
	{
	public:
		// Default constructor
		AddComponentPanel();

		// Constructor
		AddComponentPanel(ActorViewer* viewer);

		// Draws widget, calls OnDrawn for CursorEventsListener
		void Draw();

		// Returns filter widget
		EditBox* GetFilter() const;

		// Returns tree widget
		ComponentsTree* GetTree() const;

		// Returns true if point is in this object
		bool IsUnderPoint(const Vec2F& point) override;

		// Returns true when input events can be handled by down listeners
		bool IsInputTransparent() const override;

		// Returns create menu category in editor
		static String GetCreateMenuCategory();

		SERIALIZABLE(AddComponentPanel);

	private:
		ActorViewer* mViewer = nullptr; // Owner actors viewer

		EditBox*        mFilterBox = nullptr; // Components names filter edit box, updates list of component when edit
		Button*         mAddButton = nullptr; // Add button
		ComponentsTree* mTree = nullptr;      // Components tree

	private:
		// It is called when add button pressed. Adds selected component to target actors from viewer
		void OnAddPressed();

		// Creates component by type
		void CreateComponent(const ObjectType* objType);

		// It is called when tree node was double clicked
		void OnNodeDblClick(TreeNode* nodeWidget);

		// It is called when key was released. When returns has pressed, component is creating
		void OnKeyReleased(const Input::Key& key);
	};
	
	// ---------------------------------------------------------------------
	// Components tree. Builds data tree by available components with filter
	// ---------------------------------------------------------------------
	class ComponentsTree : public Tree
	{
	public:
		struct NodeData
		{
			NodeData*         parent = nullptr;
			Vector<NodeData*> children;

			String name;
			String path;
			String icon = "ui/UI4_component_icon.png";

			const Type* type = nullptr;

		public:
			~NodeData();

			void Clear();
			NodeData* AddChild(const String& name, const Type* type);
		};

	public:
		// Default constructor
		ComponentsTree();

		// Copy-constructor
		ComponentsTree(const ComponentsTree& other);

		// Copy operator
		ComponentsTree& operator=(const ComponentsTree& other);

		// Refreshes components list
		void Refresh();

		// Sets filter and refreshes tree
		void SetFilter(const WString& filter);

		// Returns create menu category in editor
		static String GetCreateMenuCategory();

		SERIALIZABLE(ComponentsTree);

	private:
		WString  mFilterStr; // Filtering string
		NodeData mRoot; // Root properties data node

	private:
		// Updates visible nodes (calculates range and initializes nodes), enables editor mode
		void UpdateVisibleNodes() override;

		// Gets tree node from pool or creates new, enables editor mode
		TreeNode* CreateTreeNodeWidget() override;

		// Returns object's parent
		void* GetObjectParent(void* object) override;

		// Returns object's children
		Vector<void*> GetObjectChilds(void* object) override;

		// Returns debugging string for object
		String GetObjectDebug(void* object) override;

		// Sets nodeWidget data by object
		void FillNodeDataByObject(TreeNode* nodeWidget, void* object) override;

		void OnDeserialized(const DataValue& node) override;

		friend class ComponentsTreeNode;
	};

	// --------------------------------------------------------------------------
	// Components tree node. Can bea  group of components or the component itself
	// --------------------------------------------------------------------------
	class ComponentsTreeNode : public TreeNode
	{
	public:
		ComponentsTree::NodeData* data = nullptr;

	public:
		// Default constructor
		ComponentsTreeNode();

		// Copy-constructor
		ComponentsTreeNode(const ComponentsTreeNode& other);

		// Copy operator
		ComponentsTreeNode& operator=(const ComponentsTreeNode& other);

		// Initializes node by data
		void Setup(ComponentsTree::NodeData* data, ComponentsTree* tree);

		// Returns create menu category in editor
		static String GetCreateMenuCategory();

		SERIALIZABLE(ComponentsTreeNode);

	private:
		Text*   mName;
		Sprite* mIcon;

		ComponentsTree* mTree = nullptr;

	private:
		// It is called on deserialization, initializes controls
		void OnDeserialized(const DataValue& node) override;

		// initializes controls and widgets
		void InitializeControls();

		friend class ComponentsTree;
	};
}

CLASS_BASES_META(Editor::AddComponentPanel)
{
	BASE_CLASS(o2::Widget);
	BASE_CLASS(o2::CursorEventsArea);
	BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::AddComponentPanel)
{
	FIELD().DEFAULT_VALUE(nullptr).NAME(mViewer).PRIVATE();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mFilterBox).PRIVATE();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mAddButton).PRIVATE();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mTree).PRIVATE();
}
END_META;
CLASS_METHODS_META(Editor::AddComponentPanel)
{

	PUBLIC_FUNCTION(void, Draw);
	PUBLIC_FUNCTION(EditBox*, GetFilter);
	PUBLIC_FUNCTION(ComponentsTree*, GetTree);
	PUBLIC_FUNCTION(bool, IsUnderPoint, const Vec2F&);
	PUBLIC_FUNCTION(bool, IsInputTransparent);
	PUBLIC_STATIC_FUNCTION(String, GetCreateMenuCategory);
	PRIVATE_FUNCTION(void, OnAddPressed);
	PRIVATE_FUNCTION(void, CreateComponent, const ObjectType*);
	PRIVATE_FUNCTION(void, OnNodeDblClick, TreeNode*);
	PRIVATE_FUNCTION(void, OnKeyReleased, const Input::Key&);
}
END_META;

CLASS_BASES_META(Editor::ComponentsTree)
{
	BASE_CLASS(o2::Tree);
}
END_META;
CLASS_FIELDS_META(Editor::ComponentsTree)
{
	FIELD().NAME(mFilterStr).PRIVATE();
	FIELD().NAME(mRoot).PRIVATE();
}
END_META;
CLASS_METHODS_META(Editor::ComponentsTree)
{

	PUBLIC_FUNCTION(void, Refresh);
	PUBLIC_FUNCTION(void, SetFilter, const WString&);
	PUBLIC_STATIC_FUNCTION(String, GetCreateMenuCategory);
	PRIVATE_FUNCTION(void, UpdateVisibleNodes);
	PRIVATE_FUNCTION(TreeNode*, CreateTreeNodeWidget);
	PRIVATE_FUNCTION(void*, GetObjectParent, void*);
	PRIVATE_FUNCTION(Vector<void*>, GetObjectChilds, void*);
	PRIVATE_FUNCTION(String, GetObjectDebug, void*);
	PRIVATE_FUNCTION(void, FillNodeDataByObject, TreeNode*, void*);
	PRIVATE_FUNCTION(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(Editor::ComponentsTreeNode)
{
	BASE_CLASS(o2::TreeNode);
}
END_META;
CLASS_FIELDS_META(Editor::ComponentsTreeNode)
{
	FIELD().DEFAULT_VALUE(nullptr).NAME(data).PUBLIC();
	FIELD().NAME(mName).PRIVATE();
	FIELD().NAME(mIcon).PRIVATE();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mTree).PRIVATE();
}
END_META;
CLASS_METHODS_META(Editor::ComponentsTreeNode)
{

	PUBLIC_FUNCTION(void, Setup, ComponentsTree::NodeData*, ComponentsTree*);
	PUBLIC_STATIC_FUNCTION(String, GetCreateMenuCategory);
	PRIVATE_FUNCTION(void, OnDeserialized, const DataValue&);
	PRIVATE_FUNCTION(void, InitializeControls);
}
END_META;
