#pragma once

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2/Assets/Types/PipelineAsset.h"
#include "o2Editor/Pipeline/PipelineEditRegion.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelinePairLayout.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePaintEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePairViews.h"

using namespace o2;
using namespace Editor;

// Fixture of the pipeline card UI tests: an editor over a test work folder, an uploaded input image, frame stepping,
// drags, captures and screenshots of single cards

namespace
{
    const Color4 inputColor(220, 60, 40, 255);
    const Color4 resultColor(40, 90, 220, 255);

    void Step(int frames = 1)
    {
        for (int i = 0; i < frames; i++)
        {
            PushEditorScopeOnStack scope;
            o2Render.Begin();
            o2Render.SetCamera(Camera());
            o2Render.Clear(Color4(30, 31, 34, 255));
            auto root = EditorUIRoot.GetRootWidget();
            root->Update(1.0f/60.0f);
            root->UpdateChildren(1.0f/60.0f);
            root->UpdateChildrenTransforms();
            root->Draw();
            // Popups draw above the rest and take input from there, as EditorApplication::DrawUIManager has them
            o2UI.DrawCurrentLayerTopWidgets();
            o2Render.End();
            AppTestDriver::PumpFrames(1);
        }
    }

    Ref<Bitmap> Capture()
    {
        Ref<Bitmap> shot;
        o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { shot = bitmap; });
        Step();
        return shot;
    }

    Vec2I ScreenToCapture(const Vec2F& screen, const Ref<Bitmap>& capture)
    {
        Vec2I resolution = o2Render.GetResolution();
        float scale = (float)capture->GetSize().x/Math::Max(1, resolution.x);
        return Vec2I((int)((screen.x + resolution.x*0.5f)*scale), (int)((resolution.y*0.5f - screen.y)*scale));
    }

    // Two presses on one widget within 0.3 s of real time make a double click instead of a press
    void Drag(const Vec2F& from, const Vec2F& to, int steps = 10)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(350));
        o2Input.OnCursorMoved(from, 0, false);
        o2Input.OnCursorPressed(from);
        Step();
        for (int i = 1; i <= steps; i++)
        {
            o2Input.OnCursorMoved(Math::Lerp(from, to, (float)i/steps), 0);
            Step();
        }
        o2Input.OnCursorReleased();
        Step(2);
    }

    bool IsNear(const Color4& a, const Color4& b, int tolerance = 40)
    {
        return Math::Abs(a.r - b.r) <= tolerance && Math::Abs(a.g - b.g) <= tolerance && Math::Abs(a.b - b.b) <= tolerance;
    }

    struct PipelinePairUiFixture : ::testing::Test
    {
        Ref<PipelineEditor> editor;
        Ref<PipelineAsset>  asset;
        String              uploadId;

        void SetUp() override
        {
            if (!UIRoot::IsSingletonInitialzed())
            {
                PushEditorScopeOnStack scope;
                mmake<UIRoot>();
            }
            PipelineUtils::SetWorkPathOverride("../../Work/Pipelines/uitest-pairs/");
            PipelinePairLayout::SetIoView(PipelineIoView::SideBySide);

            PushEditorScopeOnStack scope;
            editor = mmake<PipelineEditor>();
            *editor->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
            auto horScroll = o2UI.CreateHorScrollBar();
            *horScroll->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 5, 15, 10);
            editor->SetHorScrollbar(horScroll);
            auto verScroll = o2UI.CreateVerScrollBar();
            *verScroll->layout = WidgetLayout::VerStretch(HorAlign::Right, 5, 15, 10);
            editor->SetVerScrollbar(verScroll);

            auto root = EditorUIRoot.GetRootWidget();
            *root->layout = WidgetLayout::Based(BaseCorner::Center, (Vec2F)o2Application.GetContentSize());
            EditorUIRoot.AddWidget(editor);
            editor->SetEnabledForcible(false);
            editor->SetEnabledForcible(true);
            for (int i = 0; i < 12; i++)
            {
                root->Update(0.1f);
                root->UpdateChildren(0.1f);
                root->UpdateChildrenTransforms();
            }
            asset = mmake<PipelineAsset>();

            uploadId = "uitest-pair-input.png";
            PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath(uploadId),
                                          PipelineValue::Image(PipelineImageOps::Blank(96, 64, inputColor)).GetPngBytes());
        }

        void TearDown() override
        {
            PipelinePairLayout::SetIoView(PipelineIoView::SideBySide);
            editor = nullptr;
            asset = nullptr;
            o2FileSystem.FolderRemove(PipelineUtils::GetWorkPath(), true);
            PipelineUtils::SetWorkPathOverride("");
            if (UIRoot::IsSingletonInitialzed())
                EditorUIRoot.RemoveAllWidgets();
        }

        // Opens a graph of the node fed by an image source; returns the editor's copy of the node. A cached result is found in
        // the cache when the pipeline opens, as one computed before; an extract node gets that many parts
        Ref<PipelineNode> OpenFed(const String& type, float width = 480.0f, bool withInput = true, const Ref<Bitmap>& cached = nullptr,
                                  int parts = 1)
        {
            PipelineGraph graph;
            graph.id = "uitest-pairs";
            auto node = PipelineNodeRegistry::CreateNode(type, Vec2F(0, 0));
            node->size = Vec2F(width, 0.0f);
            if (parts > 1)
            {
                Vector<PipelineExtractRegion> regions;
                for (int i = 0; i < parts; i++)
                {
                    PipelineExtractRegion region;
                    region.id = "part" + (String)i;
                    region.name = "part " + (String)i;
                    region.x = 0.3f*i; region.w = 0.3f;
                    regions.Add(region);
                }
                PipelineRegions::Write(*node, regions);
                PipelineNodeRegistry::SyncNodeWithSchema(node);
            }
            graph.nodes.Add(node);
            if (withInput)
            {
                auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F(-400, 0));
                source->SetConfigString("uploadId", uploadId);
                graph.nodes.Add(source);

                auto edge = mmake<PipelineEdge>();
                edge->id = PipelineNode::GenerateId();
                edge->fromNodeId = source->id;
                edge->fromPortId = source->outputs[0].id;
                edge->toNodeId = node->id;
                String inputPort = PipelinePairLayout::InputPortOf(type);
                edge->toPortId = node->inputs.Find([&](const PipelinePort& p) { return p.name == inputPort; })->id;
                graph.edges.Add(edge);
            }

            if (cached)
            {
                String png = PipelineValue::Image(cached).GetPngBytes();
                PipelineUtils::WriteFileBytes(PipelineExecutor::GetPreviewPath(graph.id, node->id, "png"), png);
                if (type == "imageExtract")
                {
                    for (auto& port : node->outputs)
                        PipelineUtils::WriteFileBytes(PipelineExecutor::GetPortPreviewPath(graph.id, node->id, port.id, "png"), png);
                }
            }

            graph.SaveToAsset(*asset);
            editor->SetAsset(asset);
            Step(3);
            auto live = editor->GetGraph()->FindNode(node->id);
            auto card = editor->GetNodeWidget(live->id);
            editor->SetView(card->GetCardRect().Center(), 1.0f);
            Step(3);
            return live;
        }

        // Gives the node a result, as a run would
        void SetResult(const Ref<PipelineNode>& node, const Ref<Bitmap>& bitmap)
        {
            auto card = editor->GetNodeWidget(node->id);
            auto& runtime = card->GetRuntime();
            runtime.output = bitmap ? PipelineValue::Image(bitmap) : PipelineValue();
            runtime.portOutputs.Clear();
            if (bitmap && node->nodeType == "imageExtract")
            {
                for (auto& port : node->outputs)
                    runtime.portOutputs[port.id] = runtime.output;
            }
            card->OnOutputChanged();
            Step(2);
        }

        // Saves the card as it is drawn
        void ShotCard(const Ref<PipelineNode>& node, const String& name)
        {
            auto capture = Capture();
            ASSERT_TRUE(capture);
            RectF rect = editor->GetNodeWidget(node->id)->GetCardRect();
            Vec2I lt = ScreenToCapture(editor->LocalToScreenPoint(Vec2F(rect.left - 4, rect.top + 4)), capture);
            Vec2I rb = ScreenToCapture(editor->LocalToScreenPoint(Vec2F(rect.right + 4, rect.bottom - 4)), capture);
            lt = Vec2I(Math::Max(0, lt.x), Math::Max(0, lt.y));
            rb = Vec2I(Math::Min(capture->GetSize().x, rb.x), Math::Min(capture->GetSize().y, rb.y));
            auto crop = PipelineImageOps::CropPixels(*capture, lt.x, lt.y, rb.x - lt.x, rb.y - lt.y);
            String dir = (getenv("O2_PIPELINE_SHOTS") ? String(getenv("O2_PIPELINE_SHOTS")) : String("../../Work/Pipelines/shots")) + "/pairs";
            o2FileSystem.FolderCreate(dir, true);
            ASSERT_TRUE(crop);
            EXPECT_TRUE(crop->Save(dir + "/" + name + ".png", Bitmap::ImageType::Png));
        }

        // Resizes the card as a corner drag does and brings it into the view
        void Resize(const Ref<PipelineNode>& node, const Vec2F& size)
        {
            auto card = editor->GetNodeWidget(node->id);
            node->size = size;
            card->UpdateFromNode();
            Step(2);
            editor->SetView(card->GetCardRect().Center(), 1.0f);
            Step(3);
        }

        // Returns how much of the area the drawn image takes in its fitted dimension, 1 when it spans the area; measured on
        // lines off the middle, clear of the divider grip and the corner tags
        float FillRatio(const Ref<Bitmap>& capture, const RectF& area, const Vector<Color4>& colors)
        {
            auto shows = [&](const Vec2F& point)
            {
                Color4 c = PixelAt(capture, point);
                return colors.Any([&](const Color4& color) { return IsNear(c, color); });
            };
            auto extent = [&](const Vec2F& from, const Vec2F& step, int count)
            {
                int first = -1, last = -1;
                for (int i = 0; i < count; i++)
                {
                    if (shows(from + step*(float)i))
                    {
                        if (first < 0)
                            first = i;
                        last = i;
                    }
                }
                return first < 0 ? 0.0f : (float)(last - first + 1);
            };
            if (area.Width() < 1.0f || area.Height() < 1.0f)
                return 0.0f;

            float width = extent(Vec2F(area.left + 0.5f, area.Center().y - 20.0f), Vec2F(1.0f, 0.0f), (int)area.Width());
            float height = extent(Vec2F(area.Center().x + 20.0f, area.top - 0.5f), Vec2F(0.0f, -1.0f), (int)area.Height());
            return Math::Max(width/area.Width(), height/area.Height());
        }

        // Returns the colour drawn at a canvas point
        Color4 PixelAt(const Ref<Bitmap>& capture, const Vec2F& canvasPoint)
        {
            Vec2I p = ScreenToCapture(editor->LocalToScreenPoint(canvasPoint), capture);
            const UInt8* c = PipelineImageOps::Pixel(*capture, p.x, p.y);
            return Color4(c[0], c[1], c[2], 255);
        }
    };

    // Shade a dark overlay adds to a pixel of the red or the blue test image, whose brightest channel is 220: 0 for the image,
    // 1 for the divider's 85 % dark line
    float Darkness(const Ref<Bitmap>& capture, int x, int y)
    {
        if (x < 0 || y < 0 || x >= capture->GetSize().x || y >= capture->GetSize().y)
            return 0.0f;

        const UInt8* p = PipelineImageOps::Pixel(*capture, x, y);
        int high = Math::Max((int)p[0], Math::Max((int)p[1], (int)p[2]));
        return Math::Clamp01((220 - high)/187.0f);
    }

    const Vector<String> pairTypes = {
        "imageEdit", "imageExtract", "aiRemoveBg", "removeBackground", "imageOutline", "imageShadow", "imageGradient", "imageColor"
    };

    Ref<Toggle> FindToggle(const Ref<Widget>& root, const WString& caption)
    {
        for (auto& child : root->GetChildWidgets())
        {
            if (auto toggle = DynamicCast<Toggle>(child))
            {
                if (toggle->GetCaption() == caption)
                    return toggle;
            }
            if (auto found = FindToggle(child, caption))
                return found;
        }
        return nullptr;
    }
}
