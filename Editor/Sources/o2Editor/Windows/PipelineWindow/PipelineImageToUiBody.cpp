#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2/Assets/Types/FontAsset.h"
#include "o2Editor/Windows/PipelineWindow/PipelineImageTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

#include "o2/Assets/Assets.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/Nodes/ImageToUiNode.h"
#include "o2Editor/Pipeline/PipelineImageToUi.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRunSteps.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Windows/PipelineWindow/ImageToUiDebugDlg.h"
#include "o2Editor/Windows/PipelineWindow/ImageToUiEditsDlg.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"

namespace Editor
{
    using namespace PipelineControls;

    // The sprites grid of the card, AssetsLine's uiGrid: as many columns of at least 52 as fit, square cells up to 96, a
    // caption under each
    struct PipelineUiSpriteGeometry
    {
        static constexpr float cellMin = 52.0f, cellMax = 96.0f, caption = 18.0f, gap = PipelinePairLayout::gridGap;
        static constexpr float emptyHeight = 64.0f;

        int   count = 0;
        int   cols = 0;
        int   rows = 0;
        float cell = 0.0f;
        float height = emptyHeight;

        PipelineUiSpriteGeometry(float rowWidth, int n): count(n)
        {
            if (n <= 0)
                return;

            cols = Math::Max(1, Math::Min(n, (int)std::floor((rowWidth + gap)/(cellMin + gap))));
            cell = Math::Min(cellMax, std::floor((rowWidth - gap*(cols - 1))/cols));
            rows = (n + cols - 1)/cols;
            height = rows*(cell + 2.0f + caption) + (rows - 1)*gap;
        }

        // Top-right corner of a cell's image from the grid's top-left, y down: where its output port sits
        Vec2F Corner(int index) const
        {
            int col = index % Math::Max(1, cols), row = index/Math::Max(1, cols);
            return Vec2F(col*(cell + gap) + cell, row*(cell + 2.0f + caption + gap));
        }
    };

    // ------------------------------------------------------------------------------------------
    // Progress over a sprite cell while the node runs: the cell dimmed and a turning quarter ring
    // ------------------------------------------------------------------------------------------
    class PipelineUiCellSpinner : public Widget
    {
    public:
        explicit PipelineUiCellSpinner(RefCounter* refCounter): Widget(refCounter) {}

        void Update(float dt) override
        {
            Widget::Update(dt);
            mTime = Math::Mod(mTime + dt, 0.8f);
        }

        void Draw() override
        {
            if (!mResEnabledInHierarchy || mIsClipped)
                return;

            RectF rect = layout->GetWorldRect();
            o2Render.DrawFilledPolygon({ rect.LeftBottom(), Vec2F(rect.left, rect.top), rect.RightTop(), Vec2F(rect.right, rect.bottom) },
                                       Color4(0, 0, 0, 70));

            float unitsPerPixel = Math::Max(0.0001f, o2Render.GetCamera().GetScale2D().x);
            float width = Math::Max(1.0f, 2.0f/unitsPerPixel), radius = 7.0f;
            Vec2F center = rect.Center();
            float start = Math::PI()*0.25f - mTime/0.8f*Math::PI()*2.0f;
            Vector<Vec2F> arc;
            for (int i = 0; i <= 8; i++)
            {
                float angle = start + Math::PI()*0.5f*(float)i/8.0f;
                arc.Add(center + Vec2F(Math::Cos(angle), Math::Sin(angle))*radius);
            }
            o2Render.DrawAALine(arc, Color4(255, 255, 255, 220), width);
        }

    private:
        float mTime = 0.0f;
    };

    // --------------------------------------------------------------------------------------------------------------
    // The sprites a run cut: a square cell each with the sprite and its key under it; the sprite's output port sits
    // on the top-right corner of the cell's image. Before the first run a hint stands in for the cells
    // --------------------------------------------------------------------------------------------------------------
    class PipelineUiSpriteGrid : public Widget
    {
    public:
        explicit PipelineUiSpriteGrid(RefCounter* refCounter): Widget(refCounter)
        {
            mHint = MakeLabel("Sprites appear after the run", true);
            mHint->name = "sprites hint";
            mHint->horAlign = HorAlign::Middle;
            mHint->verAlign = VerAlign::Middle;
            *mHint->layout = WidgetLayout::BothStretch();
            AddChild(mHint);
        }

        // Rebuilds the cells for the sprite outputs
        void SetPorts(const Vector<PipelinePort>& ports)
        {
            Vector<String> ids;
            for (auto& port : ports)
                ids.Add(port.id + "|" + port.name);
            if (ids == mIds)
                return;

            mIds = ids;
            for (auto& cell : mCells)
                RemoveChild(cell.root);
            mCells.Clear();

            for (auto& port : ports)
            {
                Cell cell;
                cell.id = port.id;
                cell.root = mmake<Widget>();
                cell.root->name = "sprite cell";
                cell.image = mmake<PipelineCellImage>();
                cell.image->SetHint("\xC2\xB7");
                cell.root->AddChild(cell.image);
                cell.spinner = mmake<PipelineUiCellSpinner>();
                cell.root->AddChild(cell.spinner);
                cell.spinner->SetEnabledForcible(mBusy);
                cell.label = MakeLabel(port.name, true);
                cell.label->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::MonoFont()));
                cell.label->height = PipelineTheme::Size(9.5f);
                cell.label->color = PipelineTheme::Get().fgDim;
                cell.label->horOverflow = Label::HorOverflow::Dots;
                cell.label->horAlign = HorAlign::Middle;
                cell.label->verAlign = VerAlign::Middle;
                cell.root->AddChild(cell.label);
                AddChild(cell.root);
                mCells.Add(cell);
            }

            mHint->SetEnabledForcible(mCells.IsEmpty());
            LayoutCells();
        }

        // Shows a sprite in its cell
        void SetImage(const String& portId, const Ref<Bitmap>& bitmap)
        {
            for (auto& cell : mCells)
            {
                if (cell.id == portId)
                    cell.image->SetBitmap(bitmap);
            }
        }

        // Shows the progress over the cells, or in the hint before the first run
        void SetBusy(bool busy)
        {
            mBusy = busy;
            for (auto& cell : mCells)
                cell.spinner->SetEnabledForcible(busy);
            mHint->text = busy ? "Cutting sprites, running the Image-To-UI skill..." : "Sprites appear after the run";
        }

        // Returns the number of cells
        int GetCellCount() const { return mCells.Count(); }

        // Returns the index of the port's cell, -1 when there is none
        int GetCellIndex(const String& id) const { return mCells.IndexOf([&](const Cell& c) { return c.id == id; }); }

        // Returns true and the drawn top-right corner of a cell's image in world space, once the cell is laid out
        bool GetImageCorner(int index, Vec2F& corner) const
        {
            if (index < 0 || index >= mCells.Count() || mCells[index].image->layout->GetWidth() <= 0.0f)
                return false;

            RectF rect = mCells[index].image->layout->GetWorldRect();
            corner = Vec2F(rect.right, rect.top);
            return true;
        }

        void UpdateSelfTransform() override
        {
            Widget::UpdateSelfTransform();
            LayoutCells();
        }

    private:
        struct Cell
        {
            String                       id;
            Ref<Widget>                  root;
            Ref<PipelineCellImage>       image;
            Ref<PipelineUiCellSpinner>   spinner;
            Ref<Label>                   label;

            bool operator==(const Cell& other) const { return id == other.id; }
        };

        Vector<Cell>   mCells;
        Vector<String> mIds;
        Ref<Label>     mHint;
        bool           mBusy = false;

    private:
        void LayoutCells()
        {
            float width = layout->GetWidth();
            if (width <= 0.0f || mCells.IsEmpty())
                return;

            PipelineUiSpriteGeometry g(width, mCells.Count());
            for (int i = 0; i < mCells.Count(); i++)
            {
                auto& cell = mCells[i];
                Vec2F corner = g.Corner(i);
                float x = corner.x - g.cell, y = corner.y;
                *cell.root->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(x, -(y + g.cell + 2.0f + g.caption)), Vec2F(x + g.cell, -y));
                *cell.image->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, g.cell, 0);
                *cell.spinner->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, g.cell, 0);
                *cell.label->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, g.caption, 0);
            }
        }
    };

    // -----------------------------------------------------------------------------------------------------------------
    // AI image to UI: design | reconstruction, the summary (whether the skill completed, the counts) with the debug view and
    // the export, the sprites the layout names (each with its output port), where the prototype goes, the notes for the model
    // and the parameters
    // -----------------------------------------------------------------------------------------------------------------
    class ImageToUiBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;

        void Build() override
        {
            WeakRef<ImageToUiBody> weakThis(this);
            AddPairRow(MakeResultView("no result - press play to compute the branch"));

            // .ui-body .head-row: the summary in mono, then the head-row buttons; o2 saves the prototype into the project where
            // AssetsLine downloads the sprites and the layout
            auto head = mmake<HorizontalLayout>();
            head->name = "summary row";
            head->spacing = 6;
            head->expandWidth = true;
            head->expandHeight = true;
            head->baseCorner = BaseCorner::Left;
            mSummary = MakeLabel("Not run yet", true);
            mSummary->name = "summary";
            mSummary->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::MonoFont()));
            mSummary->height = PipelineTheme::Size(11.0f);
            mSummary->color = PipelineTheme::Get().fgDim;
            mSummary->horOverflow = Label::HorOverflow::Dots;
            head->AddChild(mSummary);
            mDebug = PipelineImageTheme::MakeSmallToggle("Debug", GetBool("debugView", false));
            mDebug->name = "debug view";
            mDebug->onToggleByUser = [weakThis](bool value) { if (auto self = weakThis.Lock()) self->SetDebugView(value); };
            head->AddChild(mDebug);
            mEdits = PipelineImageTheme::MakeSmallToggle(EditsCaption(), GetBool("editsView", false));
            mEdits->name = "edits view";
            mEdits->onToggleByUser = [weakThis](bool value) { if (auto self = weakThis.Lock()) self->SetEditsView(value); };
            head->AddChild(mEdits);
            mSave = MakeSmallButton("Save to Assets");
            mSave->name = "save to assets";
            PipelineImageTheme::KeepWholeCaption(*mSave);
            head->AddChild(mSave);
            mSave->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->SaveToAssets(); };
            auto headBlock = mmake<Widget>();
            headBlock->name = "head row";
            *head->layout = WidgetLayout::BothStretch(0, 0, 0, 1);
            headBlock->AddChild(head);
            AddRow(headBlock, 1.0f + smallButtonHeight + 0.04f);

            mGrid = mmake<PipelineUiSpriteGrid>();
            mGrid->name = "sprites";
            AddRow(mGrid, [weakThis](float width)
            {
                auto self = weakThis.Lock();
                return self ? PipelineUiSpriteGeometry(width, PipelineImageToUi::SpritePorts(*self->mNode).Count()).height : 0.0f;
            });
            MarkContent(mGrid);

            AddPrimaryField("hint", "What the screen is, notes for the model (optional)");

            if (BeginParams({ "Vision model", "Image model", "Seed", "Agent turns", "Sprite cut", "Background", "Export" }))
            {
                AddExportRows();
                AddModelRow(PipelineTextModelPresets(), PipelineImageToUi::defaultModel, PipelineModelKind::Text, "model", "Vision model", 100);
                AddModelRow(PipelineImageModelPresets(), PipelineImageToUi::defaultImageModel, PipelineModelKind::Image, "imageModel",
                            "Image model", 100);
                AddSeedRow();
                int turns = PipelineImageToUi::ReadConfig(*mNode).maxTurns;
                Vector<Pair<String, String>> turnOptions;
                for (int option : PipelineImageToUi::TurnOptions())
                    turnOptions.Add(Pair<String, String>((String)option, (String)option));
                String current = PipelineImageToUi::TurnOptions().Contains(turns) ? (String)turns : (String)PipelineImageToUi::defaultMaxTurns;
                AddChoiceRow("Agent turns", "maxTurns", turnOptions, current, true);
                AddChoiceRow("Sprite cut", "sprites", { { "auto", "Auto" }, { "ai", "AI" } }, PipelineImageToUi::ReadConfig(*mNode).sprites, false);
                AddChoiceRow("Background", "background", { { "auto", "Auto" }, { "image", "Image" }, { "color", "Colour" }, { "none", "None" } },
                             PipelineImageToUi::ReadConfig(*mNode).background, false);
            }
            EndParams();

            OnOutputChanged();
            OnRunStateChanged();
            ApplyDebugView();
            ApplyEditsView();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            RefreshSprites();
            RefreshSummary();
            UpdateAssetInfo();
            ImageToUiDebug::Refresh(mNode->id);
            ImageToUiEdits::Refresh(mNode->id);
        }

        void OnConfigChanged() override
        {
            if (mDebug)
                mDebug->SetValue(GetBool("debugView", false));
            if (mFolderEdit)
                mFolderEdit->SetText(PipelineImageToUi::ReadConfig(*mNode).assetPath);
            if (mNameEdit)
                mNameEdit->SetText(GetString("screenName", ""));
            ApplyDebugView();
            UpdateEditsToggle();
            ApplyEditsView();
            ImageToUiEdits::Refresh(mNode->id);
            ImageToUiDebug::Refresh(mNode->id);
            UpdateAssetInfo();
        }

        void OnRunStateChanged() override
        {
            bool running = IsRunning();
            if (mGrid)
                mGrid->SetBusy(running);
            ImageToUiDebug::SetRunning(mNode->id, running);
            ImageToUiEdits::SetRunning(mNode->id, running);
            RefreshSummary();
        }

        // The sprites' output ports sit on the image corners of their cells; preview and layout keep their port rows
        bool HasBodyPort(const String& portId) const override
        {
            return mGrid && mGrid->GetCellIndex(portId) >= 0;
        }

        bool GetBodyPortOffset(const String& portId, Vec2F& offset) const override
        {
            auto owner = mOwner.Lock();
            if (!owner || !HasBodyPort(portId))
                return false;

            float width = owner->GetCardWidth(), top = 0.0f, height = 0.0f;
            if (!GetRowPlacement(mGrid, width, owner->GetBodyAreaHeight(), top, height))
                return false;

            int index = mGrid->GetCellIndex(portId);
            PipelineUiSpriteGeometry g(width - mPadding*2.0f, mGrid->GetCellCount());
            Vec2F corner = g.Corner(index);
            offset = Vec2F(mPadding + corner.x, top + corner.y);

            Vec2F drawn;
            if (mGrid->GetImageCorner(index, drawn))
            {
                RectF body = layout->GetWorldRect();
                Vec2F fromBody(drawn.x - body.left, body.top - drawn.y);
                if (Math::Abs(fromBody.x - offset.x) < 2.0f && Math::Abs(fromBody.y - offset.y) < 2.0f)
                    offset = fromBody;
            }
            return true;
        }

        // Writes the prototype of the current outputs into the assets: Assets/<assetPath>/<screenName>.proto
        bool SaveToAssets() override
        {
            String layoutJson = LayoutText();
            if (layoutJson.IsEmpty() || mSaving)
                return false;

            UiLayout::Json structure;
            if (!UiLayout::Json::Parse(layoutJson, structure))
                return false;

            auto owner = mOwner.Lock();
            if (!owner)
                return false;

            // The nine-slice borders the run wrote for the skill, when its work folder is still there
            auto editor = mEditor.Lock();
            String assetsDir = editor ? PipelineExecutor::GetWorkPath(editor->GetPipelineId(), mNode->id) + "sprites/" : String();
            Vector<UiLayout::ImageToUiSprite> sprites;
            for (auto& file : UiLayout::AssetRefs(structure))
            {
                if (!UiLayout::HasSuffix(file, ".png"))
                    continue;

                String key = file.SubStr(0, file.Length() - 4);
                String name = PipelineImageToUi::SpriteName(key);
                auto port = mNode->outputs.FindOrDefault([&](const PipelinePort& p) { return p.name == name; });
                PipelineValue value;
                if (port.id.IsEmpty() || !owner->GetRuntime().portOutputs.TryGetValue(port.id, value) || !value.IsImage())
                    continue;

                UiLayout::ImageToUiSprite sprite;
                sprite.key = key;
                sprite.png = value.GetPngBytes();
                sprite.nine = ReadNineMeta(assetsDir.IsEmpty() ? String() : assetsDir + file + ".meta");
                sprites.Add(sprite);
            }

            auto ctx = mmake<PipelineExecContext>();
            ctx->assetsPath = o2Assets.GetAssetsPath();
            ctx->nodeId = mNode->id;
            if (editor)
            {
                WeakRef<PipelineEditor> weakEditor(editor);
                ctx->log = [weakEditor](const String& message)
                {
                    auto e = weakEditor.Lock();
                    if (e && e->onLog)
                        e->onLog(message);
                };
            }

            mSaving = true;
            UpdateAssetInfo();
            mSaveJob = SaveJob(WeakRef<ImageToUiBody>(this), ctx, PipelineImageToUi::ReadConfig(*mNode), layoutJson, sprites);
            mSaveJob.Start(JobThread::Main);
            return true;
        }

    private:
        Ref<Label>                mSummary;    // Whether the skill completed, elements and sprites of the last run
        Ref<Toggle>               mDebug;      // Shows the debug view
        Ref<Toggle>               mEdits;      // Shows the edits dialog, "Edits · n" once rounds were applied
        Ref<Button>               mSave;       // Writes the prototype
        Ref<PipelineUiSpriteGrid> mGrid;       // Sprites of the run with their output ports
        Ref<EditBox>              mFolderEdit; // Folder of the prototype inside Assets
        Ref<EditBox>              mNameEdit;   // Screen name: the canvas and the prototype file
        Ref<Button>               mShowButton; // Shows the written prototype in the assets
        Ref<Label>                mPathLabel;  // Where the prototype goes
        Coroutine<void>           mSaveJob;    // Export started by Save to Assets
        bool                      mSaving = false;

        String              mSummaryOf;    // Layout text and trace stamp the summary was made from
        String              mSummaryText;  // Its summary line
        Map<String, String> mSpriteData;   // Sprite bytes each cell's bitmap was decoded from

    private:
        // The borders of a .png.meta the run wrote: "spriteBorder: {x: L, y: B, z: R, w: T}"
        static std::optional<UiLayout::NineSlice> ReadNineMeta(const String& path)
        {
            if (path.IsEmpty() || !o2FileSystem.IsFileExist(path))
                return std::nullopt;

            return UiLayout::ParseNineSliceMeta(PipelineUtils::ReadFileBytes(path));
        }

        static Coroutine<void> SaveJob(WeakRef<ImageToUiBody> body, Ref<PipelineExecContext> ctx, PipelineImageToUiConfig cfg, String layout,
                                       Vector<UiLayout::ImageToUiSprite> sprites)
        {
            bool ok = co_await ImageToUiExport(ctx, cfg, layout, sprites);
            if (auto self = body.Lock())
            {
                PushEditorScopeOnStack scope;
                self->mSaving = false;
                self->UpdateAssetInfo();
                if (auto editor = self->mEditor.Lock())
                {
                    if (editor->onLog)
                        editor->onLog(ok ? "Prototype saved: Assets/" + self->ProtoPath() : String("Prototype export failed, see the log"));
                }
            }
        }

        bool IsRunning() const
        {
            auto owner = mOwner.Lock();
            if (!owner)
                return false;

            const String& state = owner->GetRuntime().state;
            return state == "running" || state == "queued";
        }

        String LayoutText() const
        {
            auto owner = mOwner.Lock();
            auto port = mNode->outputs.Find([](const PipelinePort& p) { return p.name == "layout"; });
            PipelineValue value;
            if (owner && port && owner->GetRuntime().portOutputs.TryGetValue(port->id, value) && value.IsText())
                return value.data;
            return String();
        }

        void RefreshSummary()
        {
            if (!mSummary)
                return;

            String layoutJson = LayoutText();
            String tracePath;
            if (auto editor = mEditor.Lock())
                tracePath = PipelineImageToUi::StateTracePath(PipelineExecutor::GetWorkPath(editor->GetPipelineId(), mNode->id),
                                                              PipelineImageToUi::Edits(*mNode));
            String stamp = layoutJson + "|" + (String)PipelineImageToUi::SpritePorts(*mNode).Count() + "|" + tracePath;
            if (!tracePath.IsEmpty() && o2FileSystem.IsFileExist(tracePath))
                stamp += "|" + (String)(int)o2FileSystem.GetFileInfo(tracePath).size;
            if (stamp != mSummaryOf)
            {
                mSummaryOf = stamp;
                auto summary = PipelineImageToUi::SummarizeLayout(layoutJson);
                auto run = !summary || tracePath.IsEmpty() ? std::nullopt : PipelineImageToUi::ReadRunSummary(PipelineUtils::ReadFileBytes(tracePath));
                mSummaryText = summary ? PipelineImageToUi::SummaryLine(*summary, run ? &*run : nullptr,
                                                                        PipelineImageToUi::SpritePorts(*mNode).Count()) : String();
            }

            if (!mSummaryText.IsEmpty())
                mSummary->text = mSummaryText;
            else
                mSummary->text = IsRunning() ? "Cutting sprites, running the Image-To-UI skill..." : "Not run yet";

            if (mSave)
            {
                bool can = !mSummaryText.IsEmpty() && !mSaving;
                mSave->interactable = can;
                mSave->transparency = can ? 1.0f : 0.5f;
            }
            UpdateEditsToggle();
        }

        void RefreshSprites()
        {
            auto owner = mOwner.Lock();
            if (!mGrid || !owner)
                return;

            auto ports = PipelineImageToUi::SpritePorts(*mNode);
            mGrid->SetPorts(ports);
            for (auto& port : ports)
            {
                PipelineValue value;
                bool has = owner->GetRuntime().portOutputs.TryGetValue(port.id, value) && value.IsImage();
                String data = has ? value.data : String();
                String shown;
                if (mSpriteData.TryGetValue(port.id, shown) && shown == data)
                    continue;

                // A sprite still decoding is looked at again when it arrives
                auto bitmap = has ? Picture(value) : nullptr;
                if (bitmap || !has)
                    mSpriteData[port.id] = data;
                mGrid->SetImage(port.id, bitmap);
            }
        }

        void AddExportRows()
        {
            WeakRef<ImageToUiBody> weakThis(this);
            auto folderRow = MakeRow("Folder", nullptr, 56);
            mFolderEdit = MakeEditBox(PipelineImageToUi::ReadConfig(*mNode).assetPath, false, PipelineImageToUi::defaultAssetPath);
            mFolderEdit->name = "asset folder";
            mFolderEdit->onChangeCompleted = [weakThis](const WString& text) { if (auto self = weakThis.Lock()) self->SetFolder((String)text); };
            folderRow->AddChild(mFolderEdit);
            auto browse = MakeButton("...");
            browse->name = "browse";
            browse->layout->minWidth = 28;
            browse->layout->maxWidth = 28;
            browse->onClick = [weakThis]()
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;

                self->ShowAssetFolderMenu([weakThis](const String& folder)
                {
                    if (auto self = weakThis.Lock())
                    {
                        self->mFolderEdit->SetText(folder);
                        self->SetFolder(folder);
                    }
                });
            };
            folderRow->AddChild(browse);
            AddRow(folderRow, boxRowHeight);

            auto nameRow = MakeRow("Name", nullptr, 56);
            mNameEdit = MakeEditBox(GetString("screenName", ""), false, PipelineImageToUi::defaultScreenName);
            mNameEdit->name = "screen name";
            mNameEdit->onChangeCompleted = [weakThis](const WString& text)
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;

                String clean, raw = (String)text;
                for (int i = 0; i < raw.Length(); i++)
                {
                    char c = raw[i];
                    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
                        clean += c;
                }
                if (clean != raw)
                    self->mNameEdit->SetText(clean);
                if (clean != self->GetString("screenName", ""))
                    self->SetString("screenName", clean, true);
                self->UpdateAssetInfo();
            };
            nameRow->AddChild(mNameEdit);
            auto ext = MakeLabel(".proto", true);
            ext->horOverflow = Label::HorOverflow::None;
            ext->layout->minWidth = 40;
            ext->layout->maxWidth = 40;
            nameRow->AddChild(ext);
            mShowButton = MakeIconButton("ui/pipeline/btn_locate.svg", AccentColor(), Color4(0, 0, 0, 0));
            mShowButton->name = "show in assets";
            mShowButton->layout->minWidth = 22;
            mShowButton->layout->maxWidth = 22;
            mShowButton->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->ShowInAssets(); };
            nameRow->AddChild(mShowButton);
            AddRow(nameRow, boxRowHeight);
            UpdateAssetInfo();
        }

        // A labelled row of exclusive segments bound to a config key; numbers are stored as numbers
        void AddChoiceRow(const String& label, const String& key, const Vector<Pair<String, String>>& options, const String& current, bool number)
        {
            auto segments = mmake<HorizontalLayout>();
            segments->name = key + " options";
            segments->spacing = 4;
            segments->expandWidth = true;
            segments->expandHeight = true;
            segments->baseCorner = BaseCorner::Left;
            WeakRef<ImageToUiBody> weakThis(this);
            for (auto& option : options)
            {
                auto toggle = MakeSegment(option.second, option.first == current);
                String value = option.first, k = key;
                toggle->onToggleByUser = [weakThis, k, value, number](bool)
                {
                    auto self = weakThis.Lock();
                    if (!self)
                        return;

                    if (number)
                        self->SetNumber(k, (float)atoi(value.Data()), true);
                    else
                        self->SetString(k, value, true);
                    self->RebuildBodyKeepingArea();
                };
                segments->AddChild(toggle);
            }
            AddRow(MakeRow(label, segments, 100), 26.85f);
        }

        void SetFolder(const String& text)
        {
            String folder = text.Trimed(" \t\r\n");
            folder.ReplaceAll("\\", "/");
            while (folder.StartsWith("/"))
                folder = folder.SubStr(1);
            while (UiLayout::HasSuffix(folder, "/"))
                folder = folder.SubStr(0, folder.Length() - 1);
            if (folder != GetString("assetPath", ""))
                SetString("assetPath", folder, true);
            UpdateAssetInfo();
        }

        String ProtoPath() const
        {
            auto cfg = PipelineImageToUi::ReadConfig(*mNode);
            return cfg.assetPath + "/" + cfg.screenName + ".proto";
        }

        void UpdateAssetInfo()
        {
            if (!mShowButton)
                return;

            bool exists = o2FileSystem.IsFileExist(o2Assets.GetAssetsPath() + ProtoPath());
            mShowButton->interactable = exists && !mSaving;
            mShowButton->transparency = exists ? 1.0f : 0.4f;
            RefreshSummary();
        }

        void ShowInAssets()
        {
            if (!AssetsWindow::IsSingletonInitialzed())
                return;

            String path = ProtoPath();
            if (!o2Assets.IsAssetExist(path))
                return;

            o2EditorAssets.SelectAsset(path);
            o2EditorAssets.ShowAssetIcon(path);
        }

        void SetDebugView(bool on)
        {
            if (GetBool("debugView", false) != on)
                SetBool("debugView", on, true);
            if (mDebug)
                mDebug->SetValue(on);
            ApplyDebugView();
        }

        String EditsCaption() const
        {
            int rounds = PipelineImageToUi::Edits(*mNode).Count();
            return rounds ? "Edits \xC2\xB7 " + (String)rounds : String("Edits");
        }

        // The caption follows the rounds; without a result and without rounds there is nothing to change
        void UpdateEditsToggle()
        {
            if (!mEdits)
                return;

            String caption = EditsCaption();
            if ((String)mEdits->GetCaption() != caption)
            {
                mEdits->caption = caption;
                float width = Math::Ceil(TextWidth(caption, smallFont)) + 18.0f;
                mEdits->layout->minWidth = width;
                mEdits->layout->maxWidth = width;
            }
            mEdits->SetValue(GetBool("editsView", false));
            bool can = !mSummaryText.IsEmpty() || !PipelineImageToUi::Edits(*mNode).IsEmpty();
            mEdits->interactable = can;
            mEdits->transparency = can ? 1.0f : 0.5f;
        }

        void SetEditsView(bool on)
        {
            if (GetBool("editsView", false) != on)
                SetBool("editsView", on, true);
            if (mEdits)
                mEdits->SetValue(on);
            ApplyEditsView();
        }

        void ApplyEditsView()
        {
            if (!GetBool("editsView", false))
            {
                ImageToUiEdits::Hide(mNode->id);
                return;
            }

            auto editor = mEditor.Lock();
            if (!editor)
                return;

            WeakRef<ImageToUiBody> weakThis(this);
            String nodeId = mNode->id;
            WeakRef<PipelineEditor> weakEditor(editor);
            ImageToUiEdits::Show(editor, mNode, [weakThis, weakEditor, nodeId]()
            {
                if (auto self = weakThis.Lock())
                {
                    self->SetEditsView(false);
                    return;
                }

                // The card was rebuilt meanwhile: its current body takes the change
                auto editor = weakEditor.Lock();
                auto widget = editor ? editor->GetNodeWidget(nodeId) : nullptr;
                if (widget && widget->GetBody())
                {
                    widget->GetNode()->SetConfigBool("editsView", false);
                    widget->GetBody()->OnConfigChanged();
                }
            });
            ImageToUiEdits::SetRunning(mNode->id, IsRunning());
        }

        void ApplyDebugView()
        {
            if (!GetBool("debugView", false))
            {
                ImageToUiDebug::Hide(mNode->id);
                return;
            }

            auto editor = mEditor.Lock();
            if (!editor)
                return;

            WeakRef<ImageToUiBody> weakThis(this);
            String nodeId = mNode->id;
            WeakRef<PipelineEditor> weakEditor(editor);
            ImageToUiDebug::Show(editor, mNode, [weakThis, weakEditor, nodeId]()
            {
                if (auto self = weakThis.Lock())
                {
                    self->SetDebugView(false);
                    return;
                }

                // The card was rebuilt meanwhile: its current body takes the change
                auto editor = weakEditor.Lock();
                auto widget = editor ? editor->GetNodeWidget(nodeId) : nullptr;
                if (widget && widget->GetBody())
                {
                    widget->GetNode()->SetConfigBool("debugView", false);
                    widget->GetBody()->OnConfigChanged();
                }
            });
            ImageToUiDebug::SetRunning(mNode->id, IsRunning());
        }
    };

    Ref<PipelineNodeBody> CreateImageToUiNodeBody(const String& type)
    {
        if (type == PipelineImageToUi::type)
            return mmake<ImageToUiBody>();
        return nullptr;
    }
}
