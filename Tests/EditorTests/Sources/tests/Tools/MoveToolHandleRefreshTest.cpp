#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Actions/Transform.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    // Mimics what SceneEditScreen does for the active tool: forwards every
    // `o2Scene.onObjectsChanged` notification to its OnSceneChanged callback.
    // Used as a stand-in for MoveTool / RotateTool / ScaleTool whose
    // UpdateHandlesPosition() runs only via that callback.
    struct ToolListenerProbe
    {
        int sceneChangedCalls = 0;
        Vector<UInt64> lastChangedIds;

        ToolListenerProbe()
        {
            mHandler = Function<void(const Vector<Ref<SceneEditableObject>>&)>(
                [this](const Vector<Ref<SceneEditableObject>>& objs)
                {
                    sceneChangedCalls++;
                    lastChangedIds.Clear();
                    for (auto& o : objs)
                        if (o)
                            lastChangedIds.Add(o->GetID());
                });
            o2Scene.onObjectsChanged += mHandler;
        }

        ~ToolListenerProbe()
        {
            o2Scene.onObjectsChanged -= mHandler;
        }

    private:
        Function<void(const Vector<Ref<SceneEditableObject>>&)> mHandler;
    };
}

// Reproduces the reported MoveTool issue: pressing Ctrl+Z on a transform must
// publish a scene-change notification so the active tool can refresh handles.
//
// Editor flow:
//   ActionsList::UndoAction()             // Ctrl+Z
//     -> action->Undo()                   // mutates scene
//   SceneEditScreen::Update()             // next frame
//     -> o2Scene.CheckChangedObjects()    // fires onObjectsChanged
//       -> SceneEditScreen::OnSceneChanged(objs)
//         -> mEnabledTool->OnSceneChanged(objs)
//           -> MoveTool::UpdateHandlesPosition()
//
// If any of those links is silent, handles stay frozen at the post-drag pose
// while the actor jumps back to its pre-drag pose.
TEST(MoveToolHandleRefresh, UndoTransformActionFiresSceneChangedNotification)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    // Build, complete and register a TransformAction the same way MoveTool does.
    auto action = mmake<TransformAction>(AsEditable({ a }));
    SetActorPos(a, Vec2F(50.0f, 0.0f));
    action->Completed();

    auto list = mmake<ActionsList>();
    list->DoneAction(action);

    // Drain pending change notifications from the setup so we measure only what
    // Undo emits.
    o2Scene.CheckChangedObjects();
    a->changedFrame = -1;

    ToolListenerProbe probe;

    list->UndoAction();
    o2Scene.CheckChangedObjects();

    EXPECT_GT(probe.sceneChangedCalls, 0)
        << "onObjectsChanged was silent after Undo — the active tool's "
           "OnSceneChanged would not fire and handles would stay stale.";
    EXPECT_TRUE(probe.lastChangedIds.Contains(a->GetID()))
        << "Undone actor missing from the change-list payload.";
    EXPECT_TRUE(NearV(a->transform->GetPosition2D(), Vec2F(0.0f, 0.0f)))
        << "Sanity check: Undo should have rolled the actor back.";
}

// Same scenario for Redo: pressing Ctrl+Shift+Z must republish the change so
// handles snap to the redone pose.
TEST(MoveToolHandleRefresh, RedoTransformActionFiresSceneChangedNotification)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    auto action = mmake<TransformAction>(AsEditable({ a }));
    SetActorPos(a, Vec2F(50.0f, 0.0f));
    action->Completed();

    auto list = mmake<ActionsList>();
    list->DoneAction(action);

    o2Scene.CheckChangedObjects();
    a->changedFrame = -1;
    list->UndoAction();
    o2Scene.CheckChangedObjects();
    a->changedFrame = -1;

    ToolListenerProbe probe;

    list->RedoAction();
    o2Scene.CheckChangedObjects();

    EXPECT_GT(probe.sceneChangedCalls, 0)
        << "onObjectsChanged was silent after Redo — handles would stay stale.";
    EXPECT_TRUE(probe.lastChangedIds.Contains(a->GetID()))
        << "Redone actor missing from the change-list payload.";
    EXPECT_TRUE(NearV(a->transform->GetPosition2D(), Vec2F(50.0f, 0.0f)))
        << "Sanity check: Redo should have re-applied the actor's done pose.";
}

// Tighter, action-only variant: bypass ActionsList and call TransformAction::Undo
// directly. Mirrors the existing UndoMarksObjectAsChanged but verifies the
// downstream signal (what the tool actually subscribes to), not just the
// `mChangedObjects` list state.
TEST(MoveToolHandleRefresh, RawUndoFlushesOnObjectsChangedSignal)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    auto action = mmake<TransformAction>(AsEditable({ a }));
    SetActorPos(a, Vec2F(50.0f, 0.0f));
    action->Completed();
    o2Scene.CheckChangedObjects();
    a->changedFrame = -1;

    ToolListenerProbe probe;

    action->Undo();
    o2Scene.CheckChangedObjects();

    EXPECT_GT(probe.sceneChangedCalls, 0)
        << "TransformAction::Undo did not result in onObjectsChanged firing — "
           "MoveTool would never refresh.";
    EXPECT_TRUE(probe.lastChangedIds.Contains(a->GetID()));
}

// Regression: Undo running in the same frame as a drag-finish CheckChangedObjects
// must still publish onObjectsChanged.
//
// `Scene::OnObjectChanged` keeps a per-frame dedup guard (`actor.changedFrame ==
// currentFrame -> skip`). `Scene::CheckChangedObjects` has to clear that guard
// when it drains the list — otherwise a later change in the same frame (Undo
// right after a drag) is silently dropped, mChangedObjects stays empty, and the
// active tool's OnSceneChanged never fires (handles freeze at the post-drag
// pose).
TEST(MoveToolHandleRefresh, UndoInSameFrameAsDragStillFiresSignal)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    auto action = mmake<TransformAction>(AsEditable({ a }));
    SetActorPos(a, Vec2F(50.0f, 0.0f));   // actor.changedFrame := currentFrame
    action->Completed();

    auto list = mmake<ActionsList>();
    list->DoneAction(action);

    o2Scene.CheckChangedObjects();        // drain; actor.changedFrame is still = currentFrame

    // Intentionally NOT touching a->changedFrame — that is the production state
    // right after CheckChangedObjects within the same frame as Ctrl+Z.

    ToolListenerProbe probe;

    list->UndoAction();                   // OnObjectChanged early-returns inside SetTransform
    o2Scene.CheckChangedObjects();        // empty list -> onObjectsChanged is silent

    EXPECT_TRUE(NearV(a->transform->GetPosition2D(), Vec2F(0.0f, 0.0f)))
        << "Sanity: Undo did roll the actor back even though the signal was missed.";

    EXPECT_GT(probe.sceneChangedCalls, 0)
        << "Same-frame Undo regression: Scene::OnObjectChanged guard "
           "(changedFrame == currentFrame) must be cleared by CheckChangedObjects "
           "so a follow-up Undo in the same frame is published.";
    EXPECT_TRUE(probe.lastChangedIds.Contains(a->GetID()))
        << "Same-frame Undo regression: actor missing from change-list payload.";
}

// ---------------------------------------------------------------------------
// Even when onObjectsChanged DOES fire, MoveTool::OnSceneChanged is still not
// reached. Reproduce that downstream link.
//
// SceneEditScreen subscribes its slot like this:
//
//   o2Scene.onObjectsChanged
//     += Function<void(Vector<Ref<SceneEditableObject>>)>(
//          this, &SceneEditScreen::OnSceneChanged);
//
// Note the signal type is `Function<void(const Vector<Ref<...>>&)>` (const-ref)
// but the bound wrapper is `Function<void(Vector<Ref<...>>)>` (by value). The
// `+=` only compiles because Function has an implicit lambda-template
// constructor that wraps any callable invocable with the signal's args. This
// is the same call-shape SceneEditScreen relies on; if the wrap loses or
// silently drops the call, MoveTool never gets notified — which matches what
// is observed in the editor.
// ---------------------------------------------------------------------------
namespace
{
    // Stand-in for SceneEditScreen + active tool: same subscription shape
    // (by-value method bound onto a const-ref signal), and the same forward
    // (slot calls into a virtual OnSceneChanged that MoveTool would override).
    class FakeTool
    {
    public:
        int onSceneChangedCalls = 0;
        Vector<UInt64> lastIds;

        virtual void OnSceneChanged(const Vector<Ref<SceneEditableObject>>& objs)
        {
            onSceneChangedCalls++;
            lastIds.Clear();
            for (auto& o : objs)
                if (o)
                    lastIds.Add(o->GetID());
        }

        virtual ~FakeTool() = default;
    };

    class FakeSceneEditScreen
    {
    public:
        FakeTool* mEnabledTool = nullptr;

        // Bound to o2Scene.onObjectsChanged exactly the way the real
        // SceneEditScreen binds its slot (by-value parameter on a const-ref signal).
        void OnSceneChanged(Vector<Ref<SceneEditableObject>> actors)
        {
            slotCalls++;
            if (mEnabledTool)
                mEnabledTool->OnSceneChanged(actors);
        }

        int slotCalls = 0;
    };
}

TEST(MoveToolHandleRefresh, BoundByValueSlotForwardsToToolOnSceneChange)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    FakeSceneEditScreen screen;
    FakeTool tool;
    screen.mEnabledTool = &tool;

    // Mirror the exact subscription line in SceneEditScreen::BindSceneTree.
    auto subscription = Function<void(Vector<Ref<SceneEditableObject>>)>(
        &screen, &FakeSceneEditScreen::OnSceneChanged);
    o2Scene.onObjectsChanged += subscription;

    // Trigger a scene change the same way Undo does (raw SetTransform path).
    auto action = mmake<TransformAction>(AsEditable({ a }));
    SetActorPos(a, Vec2F(50.0f, 0.0f));
    action->Completed();
    o2Scene.CheckChangedObjects();
    a->changedFrame = -1;

    action->Undo();
    o2Scene.CheckChangedObjects();

    o2Scene.onObjectsChanged -= subscription;

    EXPECT_GT(screen.slotCalls, 0)
        << "Slot bound via Function<void(Vector<...>)>(...) on a "
           "Function<void(const Vector<...>&)> signal never fired — the +="
           " conversion silently dropped the binding.";
    EXPECT_GT(tool.onSceneChangedCalls, 0)
        << "FakeSceneEditScreen ran but did not forward to mEnabledTool->OnSceneChanged "
           "— exactly what is observed for MoveTool in the real editor.";
    EXPECT_TRUE(tool.lastIds.Contains(a->GetID()))
        << "Tool received the scene-changed callback but the changed actor was missing "
           "from the payload.";
}

// Regression, end-to-end: full chain
//   o2Scene.onObjectsChanged -> SceneEditScreen-style slot -> tool->OnSceneChanged
// runs Undo in the same frame as the drag-finish CheckChangedObjects pass
// (Ctrl+Z right after letting the handle go). The active tool's
// OnSceneChanged must be invoked so it can refresh handles.
TEST(MoveToolHandleRefresh, FullChain_ToolOnSceneChangedFiresAfterSameFrameUndo)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    FakeSceneEditScreen screen;
    FakeTool tool;
    screen.mEnabledTool = &tool;

    auto subscription = Function<void(Vector<Ref<SceneEditableObject>>)>(
        &screen, &FakeSceneEditScreen::OnSceneChanged);
    o2Scene.onObjectsChanged += subscription;

    // Drag-and-finish: pose the actor and stash a TransformAction the way
    // MoveTool::HandleReleased does.
    auto action = mmake<TransformAction>(AsEditable({ a }));
    SetActorPos(a, Vec2F(50.0f, 0.0f));         // sets a.changedFrame = currentFrame
    action->Completed();

    auto list = mmake<ActionsList>();
    list->DoneAction(action);

    o2Scene.CheckChangedObjects();              // drains; a.changedFrame stays = currentFrame

    // Reset our probes — measure only the post-Undo dispatch.
    screen.slotCalls = 0;
    tool.onSceneChangedCalls = 0;
    tool.lastIds.Clear();

    list->UndoAction();                         // Ctrl+Z in the same frame as the drag
    o2Scene.CheckChangedObjects();

    o2Scene.onObjectsChanged -= subscription;

    EXPECT_GT(tool.onSceneChangedCalls, 0)
        << "Same-frame Undo regression: tool-level OnSceneChanged must fire "
           "so handles refresh after Ctrl+Z right after a drag.";
    EXPECT_TRUE(tool.lastIds.Contains(a->GetID()));
}
