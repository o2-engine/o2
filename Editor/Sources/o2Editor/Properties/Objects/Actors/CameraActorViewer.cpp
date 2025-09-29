#include "o2Editor/stdafx.h"
#include "CameraActorViewer.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Properties/Basic/BooleanProperty.h"
#include "o2Editor/Properties/Basic/ColorProperty.h"
#include "o2Editor/Properties/Basic/EnumProperty.h"
#include "o2Editor/Properties/Basic/SceneLayersListProperty.h"
#include "o2Editor/Properties/Basic/Vector2FloatProperty.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/UI/ImageSlicesEditorWidget.h"

namespace Editor
{
    void CameraActorViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        const Type& cameraActorType = TypeOf(CameraActor);

        // Basic properties
        o2EditorProperties.BuildFieldType<ColorProperty>(mSpoiler, cameraActorType, "drawLayers", "",
                                                         mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);

        o2EditorProperties.BuildFieldType<BooleanProperty>(mSpoiler, cameraActorType, "fillBackground", "",
                                                                   mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);

        o2EditorProperties.BuildFieldType<SceneLayersListProperty>(mSpoiler, cameraActorType, "fillColor", "",
                                                                   mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);

        // Type
        mTypeProperty = o2EditorProperties.BuildFieldType<EnumProperty>(mSpoiler, cameraActorType, "mType", "",
                                                                        mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);

        mTypeProperty->onChanged += [&](auto& x, bool) { OnTypeSelected(); };

        mHiddenTypeProperties = o2UI.CreateWidget<VerticalLayout>();
        mHiddenTypeProperties->expandWidth = true;
        mHiddenTypeProperties->expandHeight = false;
        mHiddenTypeProperties->fitByChildren = true;
        mSpoiler->AddChild(mHiddenTypeProperties);

        // Size properties
        mSizePropertySpoiler = o2UI.CreateWidget<Spoiler>();
        mHiddenTypeProperties->AddChild(mSizePropertySpoiler);

        mSizeProperty = o2EditorProperties.BuildFieldType<Vec2FProperty>(mSizePropertySpoiler, cameraActorType, "mFixedOrFittedSize", "",
                                                                         mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);

        mSizeProperty->SetCaption("Size");

        // Units properties
        mUnitsPropertySpoiler = o2UI.CreateWidget<Spoiler>();
        mHiddenTypeProperties->AddChild(mUnitsPropertySpoiler);

        mUnitsProperty = o2EditorProperties.BuildFieldType<EnumProperty>(mUnitsPropertySpoiler, cameraActorType, "mUnits", "",
                                                                         mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
    }

    void CameraActorViewer::OnTypeSelected()
    {
        CameraActor::Type type = (CameraActor::Type)(mTypeProperty->GetCommonValue());

        mSizePropertySpoiler->SetExpanded(type == CameraActor::Type::FittedSize || type == CameraActor::Type::FixedSize);
        mUnitsPropertySpoiler->SetExpanded(type == CameraActor::Type::PhysicalCorrect);
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<o2::CameraActor>);
// --- META ---

DECLARE_CLASS(Editor::CameraActorViewer, Editor__CameraActorViewer);
// --- END META ---
