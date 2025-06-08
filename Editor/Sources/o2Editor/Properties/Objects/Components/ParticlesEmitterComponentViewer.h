#pragma once

#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Windows/SceneWindow/SceneEditorLayer.h"

using namespace o2;

namespace o2
{
    class Spoiler;
}

namespace Editor
{
    // ----------------------------------
    // Particles emitter component viewer
    // ----------------------------------
    class ParticlesEmitterComponentViewer: public TObjectPropertiesViewer<ParticlesEmitterComponent>
    {
    public:
        // Default constructor
        ParticlesEmitterComponentViewer();

        // Destructor
        ~ParticlesEmitterComponentViewer();

        // Copy operator
        ParticlesEmitterComponentViewer& operator=(const ParticlesEmitterComponentViewer& other);

        IOBJECT(ParticlesEmitterComponentViewer);

    protected:
        // ------------------------------------
        // Scene layer for draw texture overlay
        // ------------------------------------
        struct SceneLayer : public SceneEditorLayer
        {
            WeakRef<ParticlesEmitterComponentViewer> viewer; // Reference to viewer

        public:
            // Draws editor over scene
            void DrawOverScene() override;

            // Updates editor
            void Update(float dt) override;

            // Returns order of layer
            int GetOrder() const override;

            // Returns true if layer is enabled
            bool IsEnabled() const override;

            // Returns name of layer
            const String& GetName() const override;

            // Returns icon name of layer
            const String& GetIconName() const override;
        };

    protected:
        Ref<VerticalLayout> mControls;   // Play/Pause/Reset buttons, timeline, etc.
        Ref<SceneLayer>     mSceneLayer; // Scene layer for draw handles and update emitter

        Ref<HorizontalProgress> mTimeProgress;    // Time progress bar
        Ref<Toggle>             mPlayPauseToggle; // Play/Pause button
        Ref<Button>             mReplayButton;    // Replay button

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Enable viewer event function
        void OnPropertiesEnabled() override;

        // Disable viewer event function
        void OnPropertiesDisabled() override;

        // Returns target objects
        Vector<Pair<ParticlesEmitterComponent*, ParticlesEmitterComponent*>>& GetTargetObjects();

        // Called when play/pause button pressed, start or stop emitter
        void OnPlayPauseTogglePressed(bool play);

        // Called when loop button pressed, enable or disable loop
        void OnLoopTogglePressed(bool loop);

        // Called when replay button pressed, reset emitter and start it
        void OnReplayPressed();

        // Called when time progress changed, sets emitter time 
        void SetTimeProgress(float time);
    };
}
// --- META ---

CLASS_BASES_META(Editor::ParticlesEmitterComponentViewer)
{
    BASE_CLASS(Editor::TObjectPropertiesViewer<ParticlesEmitterComponent>);
}
END_META;
CLASS_FIELDS_META(Editor::ParticlesEmitterComponentViewer)
{
    FIELD().PROTECTED().NAME(mControls);
    FIELD().PROTECTED().NAME(mSceneLayer);
    FIELD().PROTECTED().NAME(mTimeProgress);
    FIELD().PROTECTED().NAME(mPlayPauseToggle);
    FIELD().PROTECTED().NAME(mReplayButton);
}
END_META;
CLASS_METHODS_META(Editor::ParticlesEmitterComponentViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;
    typedef Vector<Pair<ParticlesEmitterComponent*, ParticlesEmitterComponent*>>& _tmp3;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp2);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PROTECTED().SIGNATURE(_tmp3, GetTargetObjects);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPlayPauseTogglePressed, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLoopTogglePressed, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnReplayPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, SetTimeProgress, float);
}
END_META;
// --- END META ---
