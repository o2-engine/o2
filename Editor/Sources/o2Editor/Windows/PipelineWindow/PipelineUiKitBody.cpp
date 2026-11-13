#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"
#include "PipelineKitCard.h"

#include <map>

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/Nodes/UiNodes.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocDevice.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiDoc/UiStyleAi.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineImageTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineUiAi.h"

namespace Editor
{
    using namespace PipelineControls;
    using UiDoc::Json;
    namespace Kit = PipelineKitLayout;

    namespace
    {
        // The element whose settings are open, per kit node: view state of the card, kept while the card is built again
        Map<String, String>& OpenCells()
        {
            static Map<String, String> open;
            return open;
        }

        // "Aa" in a style's look, drawn once per look
        Ref<Bitmap> StyleLetters(const UiDoc::TextStyle& style)
        {
            static std::map<std::string, Ref<Bitmap>> cache;
            std::string key = UiDoc::TextStyleToJson(style).Dump().Data();
            auto it = cache.find(key);
            if (it != cache.end())
                return it->second;

            if (cache.size() > 64)
                cache.clear();

            // The sample's screen without its width for a line of text: the letters fill a square cell
            auto doc = UiDoc::SampleDoc(style, "Aa", "#20232B");
            auto reach = UiDoc::EffectsReach(style);
            doc.width = Math::Max(doc.height, std::ceil(UiDoc::EstimateTextWidth("Aa", style)*1.03 + 36 + reach.left + reach.right + 8));
            UiDoc::UiKit kit;
            kit.name = "k";
            kit.textStyles = { style };
            UiDoc::UiRenderOptions options;
            options.mode = "kit";
            options.kit = &kit;
            options.pixelLong = 192;
            auto bitmap = UiDoc::RenderLayoutBitmap(doc, options);
            cache[key] = bitmap;
            return bitmap;
        }

        bool StyleOf(const PipelineValue& value, UiDoc::TextStyle& style)
        {
            if (value.type != PipelinePortType::TextStyle || value.data.IsEmpty())
                return false;

            Json raw;
            Json::Parse(value.data, raw);
            style = UiDoc::NormalizeTextStyle(raw);
            return true;
        }
    }

    // ---------------------------------------------------------------------------------------------------------------
    // The UI kit card (AssetsLine UiKitBody.tsx): the device the kit is made for, a head - what the kit holds, AI and its
    // models - and a grid of cells, one per input. The AI fills the meta of every picture (or one) and removes complex
    // backgrounds when asked; the rest (cutting a flat backdrop, trimming, nine-slice detection, shrinking a uniform middle)
    // runs locally on every change
    // ---------------------------------------------------------------------------------------------------------------

    void PipelineUiKitBody::Build()
    {
        auto& theme = PipelineTheme::Get();
        WeakRef<PipelineUiKitBody> weakThis(this);
        mSpacing = Kit::gap;
        ReadKit();

        // The device comes first: it is what the kit is made for
        mDevice = mmake<PipelineKitDeviceChip>();
        mDevice->name = "kit device";
        mDevice->Bind(Ref(this));
        mDevice->layout->minWidth = mDevice->layout->maxWidth = mDevice->GetPreferredWidth();
        auto deviceRow = MakeRow("Device", mDevice, 52.0f);
        deviceRow->expandWidth = false;
        AddRow(deviceRow, Kit::deviceH);

        // What the kit holds, the AI for every picture
        auto head = mmake<HorizontalLayout>();
        head->spacing = 10;
        head->expandWidth = true;
        head->expandHeight = true;
        head->baseCorner = BaseCorner::Left;
        mSummary = MakeLabel("", true);
        mSummary->name = "kit summary";
        StyleLabel(mSummary, 11.5f, 400, theme.fgDim);
        head->AddChild(mSummary);
        mAiAll = MakeGhostButton("AI: describe all");
        mAiAll->name = "ai all";
        PipelineImageTheme::MakeAccentGhost(mAiAll);
        mAiAll->layout->minWidth = mAiAll->layout->maxWidth = 140;
        mAiAll->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->Analyze(""); };
        head->AddChild(mAiAll);
        AddRow(head, Kit::headH);

        // The AI's models: the vision model, and the image model that removes complex backgrounds when that is on
        AddModelRow(PipelineTextModelPresets(), UiDoc::UI_KIT_AI_DEFAULT_MODEL, PipelineModelKind::Text, "kitModel", "Models", 52.0f);
        auto removeBg = AddCheckbox("remove backgrounds (one image generation each)", "kitRemoveBg", false);
        removeBg->name = "kit remove bg";
        removeBg->onToggleByUser = [weakThis](bool value)
        {
            if (auto self = weakThis.Lock())
            {
                self->SetBool("kitRemoveBg", value, true);
                self->RebuildBodyKeepingArea();
            }
        };
        if (GetBool("kitRemoveBg", false))
            AddModelRow(PipelineImageModelPresets(), UiDoc::UI_KIT_AI_DEFAULT_IMAGE_MODEL, PipelineModelKind::Image, "kitImageModel", "Image model", 80.0f);

        // The cells: a link dropped anywhere on the grid becomes a new input (on a cell's port: that input's link)
        mGrid = mmake<PipelineKitGrid>();
        mGrid->name = "kit grid";
        mGrid->onOpen = [weakThis](const String& id) { if (auto self = weakThis.Lock()) self->OpenCell(id); };
        mGrid->onRemove = [weakThis](const String& id) { if (auto self = weakThis.Lock()) self->RemoveInput(id); };
        mGrid->onAdd = [weakThis]()
        {
            auto self = weakThis.Lock();
            auto editor = self ? self->mEditor.Lock() : nullptr;
            if (editor)
                editor->AddCustomInput(self->mOwner.Lock());
        };
        AddRow(mGrid, [weakThis](float width)
        {
            auto self = weakThis.Lock();
            return self ? Kit::Of(width + 2.0f*Kit::padX, self->mNode->inputs.Count()).h : 120.0f;
        });
        MarkContent(mGrid);

        RefreshCells();
        SyncPopup();
    }

    void PipelineUiKitBody::OnOutputChanged()
    {
        PipelineNodeBody::OnOutputChanged();
        String before = mKitText;
        ReadKit();
        RefreshCells();
        if (before != mKitText && mPopup)
            mPopup->Rebuild();
    }

    void PipelineUiKitBody::OnConfigChanged()
    {
        // The card's height follows the inputs only: the rest changes in place
        RefreshCells();
        mPopupDirty = true;
        if (mDevice)
            mDevice->layout->minWidth = mDevice->layout->maxWidth = mDevice->GetPreferredWidth();
    }

    void PipelineUiKitBody::Update(float dt)
    {
        PipelineNodeBody::Update(dt);
        bool busy = PipelineUiAi::IsBusy("kit", mNode->id);
        if (mAiAll)
        {
            bool pictures = mNode->inputs.Any([](const PipelinePort& p) { return p.portType == PipelinePortType::Image; });
            mAiAll->interactable = !busy && pictures;
            mAiAll->transparency = busy || !pictures ? 0.4f : 1.0f;
        }

        // Pictures decode, the AI runs: the cells follow
        mPoll += dt;
        if (mPoll > 0.25f)
        {
            mPoll = 0.0f;
            RefreshCells();
        }

        if (mPopupDirty)
        {
            mPopupDirty = false;
            SyncPopup();
        }
        PlacePopup();
        WatchOutside();
    }

    Ref<PipelineEditor> PipelineUiKitBody::GetEditor() const
    {
        return mEditor.Lock();
    }

    PipelineKitLayout::Grid PipelineUiKitBody::Grid() const
    {
        auto owner = mOwner.Lock();
        return Kit::Of(owner ? owner->GetCardWidth() : Kit::defaultWidth, mNode->inputs.Count());
    }

    bool PipelineUiKitBody::HasBodyPort(const String& portId) const
    {
        return mNode->inputs.Any([&](const PipelinePort& port) { return port.id == portId; });
    }

    bool PipelineUiKitBody::GetBodyPortOffset(const String& portId, Vec2F& offset) const
    {
        auto owner = mOwner.Lock();
        if (!owner || !mGrid)
            return false;

        int index = mNode->inputs.IndexOf([&](const PipelinePort& p) { return p.id == portId; });
        if (index < 0)
            return false;

        float top = 0.0f, height = 0.0f;
        if (!GetRowPlacement(mGrid, owner->GetCardWidth(), owner->GetBodyAreaHeight(), top, height))
            return false;

        // The top-left corner of the input's cell
        auto b = Kit::CellInGrid(Grid(), index);
        offset = Vec2F(Kit::padX + b.x, top + b.y);
        return true;
    }

    bool PipelineUiKitBody::IsAddInputAt(const Vec2F& point) const
    {
        return mGrid && UiWsDraw::Inside(mGrid->layout->GetWorldRect(), point);
    }

    void PipelineUiKitBody::DrawOverlay()
    {
        if (!mPopup || !mPopup->IsEnabled())
            return;

        mPopup->overlay = true;
        mPopup->Draw();
        mPopup->overlay = false;
    }

    void PipelineUiKitBody::ReadKit()
    {
        auto output = GetOutput();
        mKitText = output.IsValid() && output.type == PipelinePortType::UiKit ? output.data : String();
        mKit = mKitText.IsEmpty() ? UiDoc::UiKit() : UiDoc::ParseKit(mKitText);
    }

    Json PipelineUiKitBody::ItemOf(const String& portId) const
    {
        auto raw = mNode->GetConfigValue("items");
        if (!raw || !raw->IsObject())
            return Json::Object();

        auto items = UiDoc::FromDataValue(*raw);
        auto item = items.Get(portId);
        return item && item->IsObject() ? *item : Json::Object();
    }

    void PipelineUiKitBody::SetItemFields(const String& portId, const Vector<Pair<String, Json>>& fields, const Vector<String>& removed)
    {
        auto raw = mNode->GetConfigValue("items");
        Json items = raw && raw->IsObject() ? UiDoc::FromDataValue(*raw) : Json::Object();
        Json item = items.Get(portId) && items.Get(portId)->IsObject() ? *items.Get(portId) : Json::Object();
        for (auto& field : fields)
            item[field.first] = field.second;
        for (auto& key : removed)
            item.Remove(key);
        items[portId] = item;
        UiDoc::SetConfigJson(mNode->config, "items", items);
        Notify("items", true);
        OnConfigChanged();
    }

    const UiDoc::UiKitElement* PipelineUiKitBody::ElementOf(const String& portId) const
    {
        for (auto& el : mKit.elements)
        {
            if (el.port && *el.port == portId)
                return &el;
        }
        return nullptr;
    }

    String PipelineUiKitBody::NameOf(const PipelinePort& port, int index) const
    {
        auto item = ItemOf(port.id);
        String own = item.Get("name") && !item.Get("name")->IsNull() ? item.Get("name")->ToJsString() : String();
        Json raw(!own.IsEmpty() ? own : port.name);
        return UiDoc::CleanKitName(&raw, port.portType == PipelinePortType::TextStyle ? "style_" + (String)(index + 1) :
                                   "element_" + (String)(index + 1));
    }

    UiDoc::UiTarget PipelineUiKitBody::Device() const
    {
        if (mKit.device)
            return *mKit.device;

        return mDevice ? mDevice->Target() : UiDoc::DefaultTarget();
    }

    PipelineValue PipelineUiKitBody::InputValue(const String& portId) const
    {
        auto editor = mEditor.Lock();
        auto graph = editor ? editor->GetGraph() : nullptr;
        auto node = graph ? graph->FindNode(mNode->id) : nullptr;
        return node ? editor->GetInputValueById(node, portId) : PipelineValue();
    }

    void PipelineUiKitBody::RefreshCells()
    {
        if (!mGrid)
            return;

        String busyPort = PipelineUiAi::KitBusyPort(mNode->id);
        auto raw = mNode->GetConfigValue("items");
        Json items = raw && raw->IsObject() ? UiDoc::FromDataValue(*raw) : Json::Object();
        Vector<PipelineKitCell> cells;
        int pictures = 0, styles = 0;
        for (int i = 0; i < mNode->inputs.Count(); i++)
        {
            auto& port = mNode->inputs[i];
            PipelineKitCell cell;
            cell.id = port.id;
            cell.style = port.portType == PipelinePortType::TextStyle;
            auto value = InputValue(port.id);
            cell.linked = value.IsValid();
            Json item = items.Get(port.id) && items.Get(port.id)->IsObject() ? *items.Get(port.id) : Json::Object();
            cell.ai = item.Get("ai") && item.Get("ai")->IsBool() && item.Get("ai")->Bool();
            if (cell.style)
            {
                styles++;
                UiDoc::TextStyle style;
                bool hasStyle = StyleOf(value, style);
                String own = item.Get("name") && !item.Get("name")->IsNull() ? item.Get("name")->ToJsString() : String();
                Json fallback(port.name);
                cell.name = !own.IsEmpty() ? own : hasStyle ? style.name : UiDoc::CleanKitName(&fallback, "style_" + (String)(i + 1));
                cell.meta = !cell.linked ? String("not connected") : hasStyle ? style.font + " " + UiDoc::JsNum(style.size) : String("text style");
                cell.purpose = hasStyle ? style.description.value_or("") : String();
                if (hasStyle)
                    cell.picture = StyleLetters(style);
            }
            else
            {
                pictures++;
                String own = item.Get("name") && !item.Get("name")->IsNull() ? item.Get("name")->ToJsString() : String();
                Json name(!own.IsEmpty() ? own : port.name);
                cell.name = UiDoc::CleanKitName(&name, "element_" + (String)(i + 1));
                String type = item.Get("type") && !item.Get("type")->IsNull() ? item.Get("type")->ToJsString() : UiDoc::GuessElementType(cell.name);
                String asset = item.Get("assetType") && !item.Get("assetType")->IsNull() ? item.Get("assetType")->ToJsString() : UiDoc::DefaultAssetType(type);
                // The asset is named when it is not the plain one: the line is short, a cell is narrow
                cell.meta = !cell.linked ? String("not connected") :
                    PipelineKitCards::TypeName(type) + (asset == "simple" ? String() : " \xC2\xB7 " + PipelineKitCards::AssetName(asset));
                cell.purpose = UiDoc::CleanPurpose(item.Get("purpose"));
                if (auto el = ElementOf(port.id))
                {
                    cell.element = *el;
                    cell.soft = el->soft;
                }
                else if (value.IsImage())
                    cell.picture = Picture(value);
                cell.busy = busyPort == "*" || (!busyPort.IsEmpty() && busyPort == port.id);
            }
            cells.Add(cell);
        }

        auto owner = mOwner.Lock();
        bool canAdd = owner && owner->GetSchema() && !owner->GetSchema()->addableInputs.IsEmpty();
        mGrid->SetCells(cells, GetOpenCell(), canAdd);
        if (mSummary)
            mSummary->text = (String)pictures + " graphics \xC2\xB7 " + (String)styles + " text styles";
    }

    String PipelineUiKitBody::GetOpenCell() const
    {
        String open;
        if (!OpenCells().TryGetValue(mNode->id, open) || !mNode->FindInput(open))
            return "";
        return open;
    }

    void PipelineUiKitBody::OpenCell(const String& portId)
    {
        // A second click on the open cell closes it; another cell moves the popup
        String open = GetOpenCell();
        if (portId.IsEmpty() || portId == open)
            OpenCells().Remove(mNode->id);
        else
            OpenCells()[mNode->id] = portId;

        RefreshCells();
        SyncPopup();
    }

    Ref<PipelineKitPopup> PipelineUiKitBody::GetPopup() const
    {
        return mPopup;
    }

    void PipelineUiKitBody::SyncPopup()
    {
        PushEditorScopeOnStack scope;
        String open = GetOpenCell();
        if (open.IsEmpty())
        {
            if (mPopup)
            {
                RemoveChild(mPopup);
                mPopup = nullptr;
            }
            return;
        }

        if (!mPopup || mPopup->portId != open)
        {
            if (mPopup)
                RemoveChild(mPopup);
            mPopup = mmake<PipelineKitPopup>();
            mPopup->name = "kit popup";
            mPopup->body = Ref(this);
            mPopup->portId = open;
            AddChild(mPopup);
        }
        mPopup->Rebuild();
        PlacePopup();
    }

    void PipelineUiKitBody::PlacePopup()
    {
        auto editor = mEditor.Lock();
        if (!mPopup || !mGrid || !editor)
            return;

        RectF cell = mGrid->GetCellRect(mPopup->portId);
        if (cell.Width() <= 0.0f)
            return;

        mPopup->Place(cell, mGrid->layout->GetWorldRect(), editor->GetVisibleCanvasRect());
    }

    void PipelineUiKitBody::WatchOutside()
    {
        auto editor = mEditor.Lock();
        if (!mPopup || !mGrid || !editor)
            return;

        // A left press outside the popup and Escape close it (PipelineKitPopup); the right button drags the canvas, so it
        // closes only when the canvas has not moved by the time it is let go (a right click)
        Vec2F p = editor->ScreenToLocalPoint(o2Input.GetCursorPos());
        bool inside = UiWsDraw::Inside(mPopup->layout->GetWorldRect(), p) || UiWsDraw::Inside(mGrid->GetCellArea(mPopup->portId), p);
        Vec3F position = editor->GetCamera().GetPosition();
        Vec2F camera(position.x, position.y);
        if (!inside && o2Input.IsRightMousePressed())
        {
            mRightPressed = true;
            mRightPress = camera;
        }
        if (mRightPressed && o2Input.IsRightMouseReleased())
        {
            mRightPressed = false;
            if ((mRightPress - camera).Length() < 0.01f)
                OpenCell("");
        }
    }

    void PipelineUiKitBody::RemoveInput(const String& portId)
    {
        auto customs = mNode->GetCustomInputs();
        customs.RemoveAll([&](const PipelinePort& p) { return p.id == portId; });
        mNode->SetCustomInputs(customs);
        auto raw = mNode->GetConfigValue("items");
        Json items = raw && raw->IsObject() ? UiDoc::FromDataValue(*raw) : Json::Object();
        items.Remove(portId);
        UiDoc::SetConfigJson(mNode->config, "items", items);
        if (GetOpenCell() == portId)
            OpenCells().Remove(mNode->id);

        PipelineNodeRegistry::SyncNodeWithSchema(mNode);
        if (auto editor = mEditor.Lock())
        {
            if (auto graph = editor->GetGraph())
                graph->RemoveDanglingEdges();
        }
        Notify("customInputs", true);
        RebuildBodyKeepingArea();
    }

    void PipelineUiKitBody::Analyze(const String& portId)
    {
        PipelineUiAi::AnalyzeKit(mEditor.Lock(), mNode->id, portId);
        RefreshCells();
    }

    Ref<PipelineNodeBody> CreateUiKitNodeBody(const String& type)
    {
        return type == "uiKit" ? mmake<PipelineUiKitBody>() : nullptr;
    }
}
