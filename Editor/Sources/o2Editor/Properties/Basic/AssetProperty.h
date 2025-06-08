#pragma once

#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Editor/DragAndDrop.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Windows/AssetsWindow/AssetsIconsScroll.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Properties/IPropertyField.h"

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(ObjectViewer);

    // -------------------------
    // Editor asset property box
    // -------------------------
    class AssetProperty : public TPropertyField<AssetRef<Asset>>, public DragDropArea, public KeyboardEventsListener
    {
    public:
        // Default constructor
        AssetProperty(RefCounter* refCounter);

        // Copy constructor
        AssetProperty(RefCounter* refCounter, const AssetProperty& other);

        // Copy operator
        AssetProperty& operator=(const AssetProperty& other);

        // Returns editing by this field type
        const Type* GetValueType() const override;

        // Returns editing by this field type by static function, can't be changed during runtime
        static const Type* GetValueTypeStatic();

        // Sets value asset id
        void SetAssetId(const UID& id);

        // Sets asset type
        void SetAssetType(const Type* assetType);

        // Specializes field info
        void SetFieldInfo(const FieldInfo* fieldInfo) override;

        // Sets property caption
        void SetCaption(const WString& text) override;

        // Returns property caption
        WString GetCaption() const override;

        // Adds remove button
        Ref<Button> GetRemoveButton() override;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        SERIALIZABLE(AssetProperty);
        CLONEABLE_REF(AssetProperty);

    protected:
        Ref<Widget>  mBox;               // Property edit box
        Ref<Text>    mNameText;          // Asset name text
        Ref<Spoiler> mSpoiler;           // Spoiler
        Ref<Label>   mCaption;           // Property name caption
        Ref<Button>  mCreateInstanceBtn; // Create instance button
        Ref<Button>  mEditBtn;           // Edit asset button

        Ref<HorizontalLayout> mHeaderContainer; // Asset controls container: create, save and remove

        Ref<ObjectViewer> mAssetObjectViewer; // Asset instance viewer. Created if required

        const Type* mAssetType = nullptr; // Type of asset

        bool mAvailableToHaveInstance = false; // Is asset can own an instance

    protected:
        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Sets common value asset id
        void SetCommonAssetId(const UID& id);

        // Sets asset id, checks value changed, calls onChangeCompleted
        void SetAssetIdByUser(const UID& id);

        // Called when create instance button pressed, creates asset instance
        void OnCreateInstancePressed();

        // Called when edit button pressed, logs asset name
        void OnEditPressed();

        // Called when remove instance button pressed, removes asset instance
        void OnRemoveInstancePressed();

        // Called when save instance button pressed, saves asset instance as asset
        void OnSaveInstancePressed();

        // Called when type specialized during setting value proxy
        void OnTypeSpecialized(const Type& type) override;

        // Returns value from proxy
        AssetRef<Asset> GetProxy(const Ref<IAbstractValueProxy>& proxy) const override;

        // Sets value to proxy
        void SetProxy(const Ref<IAbstractValueProxy>& proxy, const AssetRef<Asset>& value) override;

        // Updates value view
        void UpdateValueView() override;

        // Is required refresh view every time
        bool IsAlwaysRefresh() const override;

        // Called when cursor enters this object
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when key was pressed
        void OnKeyPressed(const Input::Key& key) override;

        // Called when some selectable listeners was dropped to this
        void OnDropped(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some drag listeners was entered to this area
        void OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some drag listeners was exited from this area
        void OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group) override;

        REF_COUNTERABLE_IMPL(TPropertyField<AssetRef<Asset>>);
    };
}
// --- META ---

CLASS_BASES_META(Editor::AssetProperty)
{
    BASE_CLASS(Editor::TPropertyField<AssetRef<Asset>>);
    BASE_CLASS(o2::DragDropArea);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::AssetProperty)
{
    FIELD().PROTECTED().NAME(mBox);
    FIELD().PROTECTED().NAME(mNameText);
    FIELD().PROTECTED().NAME(mSpoiler);
    FIELD().PROTECTED().NAME(mCaption);
    FIELD().PROTECTED().NAME(mCreateInstanceBtn);
    FIELD().PROTECTED().NAME(mEditBtn);
    FIELD().PROTECTED().NAME(mHeaderContainer);
    FIELD().PROTECTED().NAME(mAssetObjectViewer);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mAssetType);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mAvailableToHaveInstance);
}
END_META;
CLASS_METHODS_META(Editor::AssetProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AssetProperty&);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetValueType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetValueTypeStatic);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAssetId, const UID&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAssetType, const Type*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFieldInfo, const FieldInfo*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Button>, GetRemoveButton);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, SetCommonAssetId, const UID&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetAssetIdByUser, const UID&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCreateInstancePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveInstancePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSaveInstancePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTypeSpecialized, const Type&);
    FUNCTION().PROTECTED().SIGNATURE(AssetRef<Asset>, GetProxy, const Ref<IAbstractValueProxy>&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetProxy, const Ref<IAbstractValueProxy>&, const AssetRef<Asset>&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsAlwaysRefresh);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDropped, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragEnter, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragExit, const Ref<ISelectableDragableObjectsGroup>&);
}
END_META;
// --- END META ---
