#include "o2Editor/stdafx.h"
#include "ParticlesEmitterComponentViewer.h"

#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    ParticlesEmitterComponentViewer::ParticlesEmitterComponentViewer() = default;

    ParticlesEmitterComponentViewer::~ParticlesEmitterComponentViewer()
    {
        o2EditorSceneScreen.RemoveEditorLayer(mSceneLayer);
    }

    ParticlesEmitterComponentViewer& ParticlesEmitterComponentViewer::operator=(const ParticlesEmitterComponentViewer& other)
    {
        TObjectPropertiesViewer<ParticlesEmitterComponent>::operator=(other);
        return *this;
    }

    void ParticlesEmitterComponentViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        mControls = mmake<VerticalLayout>();
        mControls->expandHeight = false;
        mControls->expandWidth = true;
        mControls->fitByChildren = true;
        mSpoiler->AddChild(mControls);

        auto playControls = mmake<HorizontalLayout>();
        playControls->layout->minHeight = 20;
        playControls->expandWidth = true;
        playControls->expandHeight = false;
        playControls->borderLeft = 5;
        playControls->spacing = 5;
        mControls->AddChild(playControls);

        mPlayPauseToggle = o2UI.CreateWidget<Toggle>("green play-stop");
        mPlayPauseToggle->name = "play-stop";
        mPlayPauseToggle->layout->minSize = Vec2F(20, 20);
        mPlayPauseToggle->layout->maxSize = Vec2F(20, 20);
        mPlayPauseToggle->onToggleByUser = [&](bool play) { OnPlayPauseTogglePressed(play); };
        playControls->AddChild(mPlayPauseToggle);

        mReplayButton = o2UI.CreateWidget<Button>("revert prototype");
        mReplayButton->name = "revert";
        mReplayButton->layout->minSize = Vec2F(20, 20);
        mReplayButton->layout->maxSize = Vec2F(20, 20);
        mReplayButton->onClick = [&]() { OnReplayPressed(); };
        playControls->AddChild(mReplayButton);

        mTimeProgress = o2UI.CreateWidget<HorizontalProgress>("animation state bar");
        mTimeProgress->name = "bar";
        mTimeProgress->layout->maxHeight = 3;
        mTimeProgress->onChangeByUser = [&](float value) { SetTimeProgress(value); };
        playControls->AddChild(mTimeProgress);

        auto loopedToggle = o2UI.CreateWidget<Toggle>("animation state loop");
        loopedToggle->name = "loop";
        loopedToggle->layout->minSize = Vec2F(20, 20);
        loopedToggle->layout->maxSize = Vec2F(20, 20);
        loopedToggle->onToggleByUser = [&](bool loop) { OnLoopTogglePressed(loop); };
        playControls->AddChild(loopedToggle);

        auto horSeparatorContainer = mmake<Widget>();
        horSeparatorContainer->layout->minHeight = 3;
        mControls->AddChild(horSeparatorContainer);

        auto horSeparator = o2UI.CreateWidget<Widget>("hor separator");
        *horSeparator->layout = WidgetLayout::BothStretch(-10, 0, -10, 0);
        horSeparatorContainer->AddChild(horSeparator);

        o2EditorProperties.BuildObjectProperties(mSpoiler, &TypeOf(ParticlesEmitterComponent), mPropertiesContext, "",
                                                 mOnChildFieldChangeCompleted, onChanged);

        mSceneLayer = mmake<SceneLayer>();
        mSceneLayer->viewer = Ref(this);
    }

    void ParticlesEmitterComponentViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        auto prevTargetObjects = mTypeTargetObjects;

        TObjectPropertiesViewer<ParticlesEmitterComponent>::OnRefreshed(targetObjets);
    }

    void ParticlesEmitterComponentViewer::OnPropertiesEnabled()
    {
        o2EditorSceneScreen.AddEditorLayer(mSceneLayer);

        TObjectPropertiesViewer<ParticlesEmitterComponent>::OnPropertiesEnabled();
    }

    void ParticlesEmitterComponentViewer::OnPropertiesDisabled()
    {
        o2EditorSceneScreen.RemoveEditorLayer(mSceneLayer);

        TObjectPropertiesViewer<ParticlesEmitterComponent>::OnPropertiesDisabled();
    }

    Vector<Pair<ParticlesEmitterComponent*, ParticlesEmitterComponent*>>& ParticlesEmitterComponentViewer::GetTargetObjects()
    {
        return mTypeTargetObjects;
    }

    void ParticlesEmitterComponentViewer::OnPlayPauseTogglePressed(bool play)
    {
        for (auto& pair : GetTargetObjects())
        {
            auto emitter = pair.first;
            if (emitter)
            {
                if (play && Math::Equals(emitter->GetRelativeTime(), 1.0f))
                    emitter->GoToBegin();

                emitter->SetPlaying(play);
                emitter->SetParticlesPause(!play);
            }
        }
    }

    void ParticlesEmitterComponentViewer::OnLoopTogglePressed(bool loop)
    {
        for (auto& pair : GetTargetObjects())
        {
            auto emitter = pair.first;
            if (emitter)
                emitter->SetLoop(loop ? Loop::Repeat : Loop::None);
        }
    }

    void ParticlesEmitterComponentViewer::OnReplayPressed()
    {
        for (auto& pair : GetTargetObjects())
        {
            auto emitter = pair.first;
            if (emitter)
            {
                emitter->SetParticlesPause(false);
                emitter->RewindAndPlay();
            }
        }
    }

    void ParticlesEmitterComponentViewer::SetTimeProgress(float time)
    {
        for (auto& pair : GetTargetObjects())
        {
            auto emitter = pair.first;
            if (emitter)
            {
                emitter->SetPlaying(false);
                emitter->SetParticlesPause(true);
                emitter->SetTime(time);
            }
        }
    }

    void ParticlesEmitterComponentViewer::SceneLayer::DrawOverScene()
    {}

    void ParticlesEmitterComponentViewer::SceneLayer::Update(float dt)
    {
        bool first = true;
        auto viewer = this->viewer.Lock();
        for (auto& pair : viewer->GetTargetObjects())
        {
            auto emitter = pair.first;
            if (emitter)
            {
                emitter->Update(dt);

                if (first)
                {
                    viewer->mTimeProgress->SetValue(emitter->GetTime());

                    bool isPlaying = emitter->IsPlaying();
                    viewer->mPlayPauseToggle->SetValue(isPlaying);
                    viewer->mTimeProgress->maxValue = emitter->GetDuration();

                    first = false;
                }
            }
        }
    }

    int ParticlesEmitterComponentViewer::SceneLayer::GetOrder() const
    {
        return 0;
    }

    bool ParticlesEmitterComponentViewer::SceneLayer::IsEnabled() const
    {
        return true;
    }

    const String& ParticlesEmitterComponentViewer::SceneLayer::GetName() const
    {
        static String name = "Particles Emitter";
        return name;
    }

    const String& ParticlesEmitterComponentViewer::SceneLayer::GetIconName() const
    {
        return String::empty;
    }

}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<ParticlesEmitterComponent>);
// --- META ---

DECLARE_CLASS(Editor::ParticlesEmitterComponentViewer, Editor__ParticlesEmitterComponentViewer);
// --- END META ---
