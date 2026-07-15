#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/ValueProxy.h"
#include "o2Editor/Properties/Basic/EnumProperty.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"

using namespace o2;
using namespace Editor;

// Asset metas are not scene objects: their property edits bypass the actor undo action and
// must be applied directly by path — this is what the properties window does for them.
// Guards the "picked compression resets back to None" bug.
TEST(AssetMetaPropertyApply, CompressionEnumAppliesByPath)
{
    auto meta = mmake<AtlasAsset::Meta>();
    ASSERT_EQ(meta->common.compression, TextureCompression::None);

    Vector<DataDocument> values;
    values.Add(DataDocument());
    values[0].Set(TextureCompression::DXT5);

    PropertiesWindow::ApplyPropertyToObjects({ dynamic_cast<IObject*>(meta.Get()) },
                                             "common/compression", values);

    EXPECT_EQ(meta->common.compression, TextureCompression::DXT5);
}

TEST(AssetMetaPropertyApply, QualityIntAppliesByPath)
{
    auto meta = mmake<AtlasAsset::Meta>();

    Vector<DataDocument> values;
    values.Add(DataDocument());
    values[0].Set(60);

    PropertiesWindow::ApplyPropertyToObjects({ dynamic_cast<IObject*>(meta.Get()) },
                                             "common/quality", values);

    EXPECT_EQ(meta->common.quality, 60);
}

// The apply must not crash when a change document holds the raw int (older int-based
// property fields stored numbers, and reading a number as an enum-name string crashed)
TEST(AssetMetaPropertyApply, NumericEnumDocumentAppliesWithoutCrash)
{
    auto meta = mmake<AtlasAsset::Meta>();

    Vector<DataDocument> values;
    values.Add(DataDocument());
    values[0].Set((int)TextureCompression::DXT5);

    PropertiesWindow::ApplyPropertyToObjects({ dynamic_cast<IObject*>(meta.Get()) },
                                             "common/compression", values);

    EXPECT_EQ(meta->common.compression, TextureCompression::DXT5);
}

namespace
{
    // Exposes the protected user-change entry point; no-ops the view so the field works headless
    class EnumPropertyProbe: public EnumProperty
    {
    public:
        EnumPropertyProbe(RefCounter* refCounter): EnumProperty(refCounter) {}

        void Init() { InitializeControls(); }
        void Commit(int value) { SetValueByUserAndComplete(value); }
        void UpdateValueView() override {}
    };
}

// Enum fields serialize by name: the property's change documents must hold the name, not the
// raw int, so the change action can deserialize them into the target field
TEST(EnumPropertyStore, ChangeDocumentsHoldEnumNames)
{
    TextureCompression backing = TextureCompression::None;

    auto field = mmake<EnumPropertyProbe>();
    field->Init();
    field->SpecializeType(&TypeOf(TextureCompression));
    field->SetValueProxy({ mmake<PointerValueProxy<TextureCompression>>(&backing) });

    Vector<DataDocument> after;
    field->onChangeCompleted = [&](const String&, const Vector<DataDocument>&,
                                   const Vector<DataDocument>& afterValues) { after = afterValues; };

    field->Commit((int)TextureCompression::BC7);

    ASSERT_EQ(after.Count(), 1);
    ASSERT_TRUE(after[0].IsString());

    TextureCompression applied = TextureCompression::None;
    after[0].Get(applied);
    EXPECT_EQ(applied, TextureCompression::BC7);
}

TEST(AssetMetaPropertyApply, CompressionEnumSurvivesMetaSerialization)
{
    auto meta = mmake<AtlasAsset::Meta>();
    meta->common.compression = TextureCompression::ASTC4x4;
    meta->common.quality = 75;

    DataDocument doc;
    meta->Serialize(doc);

    auto restored = mmake<AtlasAsset::Meta>();
    restored->Deserialize(doc);

    EXPECT_EQ(restored->common.compression, TextureCompression::ASTC4x4);
    EXPECT_EQ(restored->common.quality, 75);
}
