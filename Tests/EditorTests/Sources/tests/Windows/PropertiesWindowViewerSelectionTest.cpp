#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Asset.h"
#include "o2/Assets/Types/DataAsset.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/Widgets/Label.h" // IPropertiesViewer.h -> IPropertyField.h uses Ref<Label> without forward-declaring it
#include "o2/Utils/Math/Curve.h"
#include "o2Editor/Windows/PropertiesWindow/IPropertiesViewer.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesViewerSelector.h"

using namespace o2;
using namespace Editor;

namespace
{
    // Minimal viewer that only reports a viewing type. The real viewers build styled
    // editor widgets in their constructors (fonts, atlases) which aren't available in the
    // headless editor test runner, so PropertiesViewerSelector::SelectFrom — the type
    // matching under test — is exercised with these stand-ins instead.
    class TypeStubViewer : public IPropertiesViewer
    {
    public:
        TypeStubViewer(RefCounter* refCounter, const Type* viewingType):
            IPropertiesViewer(refCounter), mViewingType(viewingType)
        {}

        const Type* GetViewingObjectType() const override { return mViewingType; }

    private:
        const Type* mViewingType = nullptr;
    };

    Ref<IPropertiesViewer> MakeStubViewer(const Type* viewingType)
    {
        return DynamicCast<IPropertiesViewer>(mmake<TypeStubViewer>(viewingType));
    }

    // Mirrors the real registry: an asset viewer, an actor viewer and a widget-layer viewer.
    Vector<Ref<IPropertiesViewer>> MakeViewers()
    {
        return {
            MakeStubViewer(&TypeOf(Asset)),
            MakeStubViewer(&TypeOf(Actor)),
            MakeStubViewer(&TypeOf(WidgetLayer)),
        };
    }

    enum ViewerIndex { AssetViewer = 0, ActorViewer = 1, WidgetLayerViewer = 2 };

    template<typename T>
    IObject* AsObject(const Ref<T>& ref)
    {
        return dynamic_cast<IObject*>(ref.Get());
    }
}

// ----------------------------------------------------------------------------
// Viewer selection by target type
// ----------------------------------------------------------------------------
TEST(PropertiesWindowViewerSelection, EmptyTargetsSelectNoViewer)
{
    EXPECT_EQ(PropertiesViewerSelector::SelectFrom({}, MakeViewers()), nullptr);
}

TEST(PropertiesWindowViewerSelection, ActorTargetSelectsActorViewer)
{
    auto viewers = MakeViewers();
    auto actor = mmake<Actor>(ActorCreateMode::NotInScene);

    auto selected = PropertiesViewerSelector::SelectFrom({ AsObject(actor) }, viewers);

    EXPECT_EQ(selected, viewers[ActorViewer]);
}

TEST(PropertiesWindowViewerSelection, AssetTargetSelectsAssetViewer)
{
    auto viewers = MakeViewers();
    auto asset = mmake<DataAsset>();

    auto selected = PropertiesViewerSelector::SelectFrom({ AsObject(asset) }, viewers);

    EXPECT_EQ(selected, viewers[AssetViewer]);
}

TEST(PropertiesWindowViewerSelection, WidgetLayerTargetSelectsWidgetLayerViewer)
{
    auto viewers = MakeViewers();
    auto layer = mmake<WidgetLayer>();

    auto selected = PropertiesViewerSelector::SelectFrom({ AsObject(layer) }, viewers);

    EXPECT_EQ(selected, viewers[WidgetLayerViewer]);
}

TEST(PropertiesWindowViewerSelection, DerivedActorTypeSelectsActorViewer)
{
    // Widget derives from Actor; the match is by IsBasedOn, not exact type.
    auto viewers = MakeViewers();
    auto widget = mmake<Widget>(ActorCreateMode::NotInScene);

    auto selected = PropertiesViewerSelector::SelectFrom({ AsObject(widget) }, viewers);

    EXPECT_EQ(selected, viewers[ActorViewer]);
}

TEST(PropertiesWindowViewerSelection, UnregisteredTypeSelectsNoViewer)
{
    auto viewers = MakeViewers();
    auto curve = mmake<Curve>();

    auto selected = PropertiesViewerSelector::SelectFrom({ AsObject(curve) }, viewers);

    EXPECT_EQ(selected, nullptr);
}

TEST(PropertiesWindowViewerSelection, ViewerWithNullViewingTypeIsSkipped)
{
    Vector<Ref<IPropertiesViewer>> viewers = {
        MakeStubViewer(nullptr),
        MakeStubViewer(&TypeOf(Actor)),
    };
    auto actor = mmake<Actor>(ActorCreateMode::NotInScene);

    auto selected = PropertiesViewerSelector::SelectFrom({ AsObject(actor) }, viewers);

    EXPECT_EQ(selected, viewers[1]);
}

TEST(PropertiesWindowViewerSelection, FirstMatchingViewerWins)
{
    // Widget is based on both Actor and Widget; the first matching viewer in the list wins.
    Vector<Ref<IPropertiesViewer>> viewers = {
        MakeStubViewer(&TypeOf(Actor)),
        MakeStubViewer(&TypeOf(Widget)),
    };
    auto widget = mmake<Widget>(ActorCreateMode::NotInScene);

    auto selected = PropertiesViewerSelector::SelectFrom({ AsObject(widget) }, viewers);

    EXPECT_EQ(selected, viewers[0]);
}

// ----------------------------------------------------------------------------
// Passing target objects
// ----------------------------------------------------------------------------
TEST(PropertiesWindowTargets, SingleObjectUsesItsType)
{
    auto viewers = MakeViewers();
    auto asset = mmake<DataAsset>();

    EXPECT_EQ(PropertiesViewerSelector::SelectFrom({ AsObject(asset) }, viewers), viewers[AssetViewer]);
}

TEST(PropertiesWindowTargets, MultipleObjectsOfSameTypeKeepThatType)
{
    auto viewers = MakeViewers();
    auto a = mmake<Actor>(ActorCreateMode::NotInScene);
    auto b = mmake<Actor>(ActorCreateMode::NotInScene);

    auto selected = PropertiesViewerSelector::SelectFrom({ AsObject(a), AsObject(b) }, viewers);

    EXPECT_EQ(selected, viewers[ActorViewer]);
}

TEST(PropertiesWindowTargets, HeterogeneousTargetsUseFirstObjectType)
{
    auto viewers = MakeViewers();
    auto actor = mmake<Actor>(ActorCreateMode::NotInScene);
    auto asset = mmake<DataAsset>();

    EXPECT_EQ(PropertiesViewerSelector::SelectFrom({ AsObject(actor), AsObject(asset) }, viewers),
              viewers[ActorViewer]);
    EXPECT_EQ(PropertiesViewerSelector::SelectFrom({ AsObject(asset), AsObject(actor) }, viewers),
              viewers[AssetViewer]);
}

TEST(PropertiesWindowTargets, EmptyTargetsSelectNoViewer)
{
    // No targets -> no viewer matched -> PropertiesWindow falls back to its default viewer.
    EXPECT_EQ(PropertiesViewerSelector::SelectFrom({}, MakeViewers()), nullptr);
}
