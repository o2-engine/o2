#pragma once

#include "o2Editor/Core/UIStyle/BasicUIStyle.h"

using namespace o2;

namespace Editor
{
    class EditorUIStyleBuilder: public BasicUIStyleBuilder
    {
    public:
        void RebuildDockableWndStyle();
        void RebuildPlayStopButtonStyle();
        void RebuildPauseButtonStyle();
        void RebuildStepButtonStyle();
        void RebuildRoundDropDown();
        void RebuildMenuPanelDropDown();
        void RebuildSearchButton();
        void RebuildListTreeToggle();
        void RebuildRevertBtn();
        void RebuildActorsTreeNodeEnableToggle();
        void RebuildActorsTreeLockToggle();
        void RebuildActorsTreeLinkBtn();
        void RebuildActorsTree();
        void RebuildDrawOrderTree();
        void RebuildPanelDownButton();
        void RebuildTrashDownPanelButton();
        void RebuildMessagesDownPanelToggle();
        void RebuildWarningsDownPanelToggle();
        void RebuildErrorsDownPanelToggle();
        void RebuildFilterMenuButton();
        void RebuildTreeMenuButton();
        void RebuildFoldersTree();
        void RebuildRegularAssetIcon();
        void RebuildAssetsGridScroll();
        void RebuildLinkBtn();
        void RebuildSinglelineEditboxProperty();
        void RebuildEditorDropdown();
        void RebuildRedEditBoxStyle();
        void RebuildGreenEditBoxStyle();

        void RebuildPreviewMenuToggle();
        void RebuildRecordMenuToggle();
        void RebuildRewindLeftMenuButton();
        void RebuildMoveLeftMenuButton();
        void RebuildPlayStopMenuToggle();
        void RebuildRewindRightMenuButton();
        void RebuildMoveRightMenuButton();
        void RebuildLoopMenuToggle();
        void RebuildAddKeyMenuButton();
        void RebuildCurvesMenuToggle();
        void RebuildPropertiesMenuButton();

        void RebuildAddKeyButton();
        void RebuildAddKeyDotButton();

        void RebuildAddSmallBtn();
        void RebuildRemoveSmallBtn();

        void RebuildCreateAssetInstanceBtn();
        void RebuildRemoveAssetInstanceBtn();
        void RebuildSaveAssetInstanceBtn();

        void RebuildFloatProperty();
        void RebuildRedFloatProperty();
        void RebuildGreenFloatProperty();
        void RebuildIntegerProperty();
        void RebuildActorProperty();
        void RebuildBoolProperty();
        void RebuildBorderFProperty();
        void RebuildBorderIProperty();
        void RebuildColorPropety();
        void RebuildComponentProperty();
        void RebuildCurveProperty();
        void RebuildColorGradientProperty();
        void RebuildEnumProperty();
        void RebuildLayerProperty();
        void RebuildRectFProperty();
        void RebuildRectIProperty();
        void RebuildStringProperty();
        void RebuildWStringProperty();
        void RebuildTagProperty();
        void RebuildVector2FProperty();
        void RebuildVector2IProperty();
        void RebuildColoredVector2Property();
        void RebuildAssetPropety();
        void RebuildSceneLayersListProperty();

        void RebuildPropertiesWithCaptins();

        void RebuildActorHeadEnableToggle();
        void RebuildActorHeadEnableProperty();

        void RebuildActorHeadName();
        void RebuildActorHeadNameProperty();
        void RebuildActorHeadDepthProperty();

        void RebuildActorHeadLockToggle();
        void RebuildActorHeadLockProperty();

        void RebuildActorHeadActorAssetProperty();

        void RebuildActorHeadTags();
        void RebuildActorHeadTagsProperty();

        void RebuildActorHeadLayer();
        void RebuildActorHeadLayerProperty();

        void RebuildAcceptPrototypeBtn();
        void RebuildRevertPrototypeBtn();
        void RebuildBreakPrototypeBtn();
        void RebuildPlayGreenBtn();
        void RebuildComponentOptionsBtn();
        void RebuildComponentSaveBtn();
        void RebuildHorWideScrollbar();
        void RebuildVerWideScrollbar();
        void RebuildHorWideProgressbar();
        void RebuildVerWideProgressbar();

        void RebuildAnimationTree();
        void RebuildAnimationPropertiesTree();

        void RebuildSpoilerWithHead();

        void RebuildAddComponentButton();
        void RebuildAddComponentsTree();

        void RebuildAnimationStateViewerEditButton();
        void RebuildAnimationStateViewerPlayToggle();
        void RebuildAnimationStateViewerLoopToggle();
        void RebuildAnimationStateViewerProgressBar();
        void RebuildAnimationSpoiler();

        void RebuildBacklessDropdownButton();
        void RebuildMemoryAnalyzerTree();

		void RebuildASGStateWidget();

        void RebuildEditorUIManager(const String& stylesFolder, bool saveStyle = true, bool checkEditedDate = true);

        IOBJECT(EditorUIStyleBuilder);

    private: 
        template<typename _property_type>
        void BuildPropertyWithCaption(const String& propertyStyle, const String& propertyWithCaptionStyle);

    };

    template<typename _property_type>
    void EditorUIStyleBuilder::BuildPropertyWithCaption(const String& propertyStyle, const String& propertyWithCaptionStyle)
    {
        Ref<_property_type> property = o2UI.CreateWidget<_property_type>(propertyStyle);

        auto label = o2UI.CreateWidget<Label>();
        label->name = "propertyName";
        label->horAlign = HorAlign::Left;
        label->layout->widthWeight = 0.6f;
        label->horOverflow = Label::HorOverflow::Dots;

        property->AddChild(label, 0);
        property->borderLeft = 10;

        o2UI.AddWidgetStyle(property, propertyWithCaptionStyle);
    }
}
// --- META ---

CLASS_BASES_META(Editor::EditorUIStyleBuilder)
{
    BASE_CLASS(o2::BasicUIStyleBuilder);
}
END_META;
CLASS_FIELDS_META(Editor::EditorUIStyleBuilder)
{
}
END_META;
CLASS_METHODS_META(Editor::EditorUIStyleBuilder)
{

    FUNCTION().PUBLIC().SIGNATURE(void, RebuildDockableWndStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildPlayStopButtonStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildPauseButtonStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildStepButtonStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRoundDropDown);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildMenuPanelDropDown);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildSearchButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildListTreeToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRevertBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorsTreeNodeEnableToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorsTreeLockToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorsTreeLinkBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorsTree);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildDrawOrderTree);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildPanelDownButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildTrashDownPanelButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildMessagesDownPanelToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildWarningsDownPanelToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildErrorsDownPanelToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildFilterMenuButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildTreeMenuButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildFoldersTree);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRegularAssetIcon);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAssetsGridScroll);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildLinkBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildSinglelineEditboxProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildEditorDropdown);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRedEditBoxStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildGreenEditBoxStyle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildPreviewMenuToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRecordMenuToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRewindLeftMenuButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildMoveLeftMenuButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildPlayStopMenuToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRewindRightMenuButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildMoveRightMenuButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildLoopMenuToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAddKeyMenuButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildCurvesMenuToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildPropertiesMenuButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAddKeyButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAddKeyDotButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAddSmallBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRemoveSmallBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildCreateAssetInstanceBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRemoveAssetInstanceBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildSaveAssetInstanceBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildFloatProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRedFloatProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildGreenFloatProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildIntegerProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildBoolProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildBorderFProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildBorderIProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildColorPropety);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildComponentProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildCurveProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildColorGradientProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildEnumProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildLayerProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRectFProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRectIProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildStringProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildWStringProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildTagProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildVector2FProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildVector2IProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildColoredVector2Property);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAssetPropety);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildSceneLayersListProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildPropertiesWithCaptins);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorHeadEnableToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorHeadEnableProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorHeadName);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorHeadNameProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorHeadDepthProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorHeadLockToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorHeadLockProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorHeadActorAssetProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorHeadTags);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorHeadTagsProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorHeadLayer);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildActorHeadLayerProperty);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAcceptPrototypeBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildRevertPrototypeBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildBreakPrototypeBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildPlayGreenBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildComponentOptionsBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildComponentSaveBtn);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildHorWideScrollbar);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildVerWideScrollbar);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildHorWideProgressbar);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildVerWideProgressbar);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAnimationTree);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAnimationPropertiesTree);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildSpoilerWithHead);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAddComponentButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAddComponentsTree);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAnimationStateViewerEditButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAnimationStateViewerPlayToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAnimationStateViewerLoopToggle);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAnimationStateViewerProgressBar);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAnimationSpoiler);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildBacklessDropdownButton);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildMemoryAnalyzerTree);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildASGStateWidget);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildEditorUIManager, const String&, bool, bool);
}
END_META;
// --- END META ---
