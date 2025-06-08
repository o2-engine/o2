#pragma once
#include "o2/Animation/Tracks/AnimationTrack.h"
#include "o2/Animation/Tracks/AnimationVec2FTrack.h"

using namespace o2;

namespace Editor
{
    template<typename TrackType>
    struct AnimationTrackWrapper
    {
        typedef typename TrackType::Key KeyType;
        typedef typename TrackType::ValueType ValueType;
        typedef typename TrackType::Player TrackPlayerType;

        static auto GetValue(TrackPlayerType& player) { return player.GetValue(); }
        static auto GetValue(TrackType& track, float pos) { return track.GetValue(pos); }
        static const Vector<KeyType>& GetKeys(TrackType& track) { return track.GetKeys(); }
        static KeyType GetKey(TrackType& track, int idx) { return track.GetKeyAt(idx); }
        static KeyType FindKey(TrackType& track, UInt64 uid) { return track.FindKey(uid); }
        static int FindKeyIdx(TrackType& track, UInt64 uid) { return track.FindKeyIdx(uid); }
        static void RemoveKeyAt(TrackType& track, int idx) { track.RemoveKeyAt(idx); }
        static int AddKey(TrackType& track, const KeyType& key) { return track.AddKey(key); }
    };

    template<>
    struct AnimationTrackWrapper<AnimationTrack<Vec2F>>
    {
        typedef Curve::Key KeyType;
        typedef AnimationTrack<Vec2F> TrackType;
        typedef float ValueType;
        typedef typename AnimationTrack<Vec2F>::Player TrackPlayerType;

        static auto GetValue(TrackPlayerType& player) 
        { 
            return DynamicCast<TrackType>(player.GetTrack())->timeCurve->Evaluate(player.GetTime());
        }

        static auto GetValue(TrackType& track, float pos) { return track.timeCurve->Evaluate(pos); }
        static const Vector<KeyType>& GetKeys(TrackType& track) { return track.timeCurve->GetKeys(); }
        static KeyType GetKey(TrackType& track, int idx) { return track.timeCurve->GetKeyAt(idx); }
        static KeyType FindKey(TrackType& track, UInt64 uid) { return track.timeCurve->FindKey(uid); }
        static int FindKeyIdx(TrackType& track, UInt64 uid) { return track.timeCurve->FindKeyIdx(uid); }
        static void RemoveKeyAt(TrackType& track, int idx) { track.timeCurve->RemoveKeyAt(idx); }
        static int AddKey(TrackType& track, const KeyType& key) { return track.timeCurve->InsertKey(key); }
    };
}
