## Sound
The sound subsystem is built on miniaudio. It is managed by `o2::SoundSystem`, with quick access via the `o2Sounds` macro. It initializes the audio engine (a null backend without an output device in headless mode), stores the master volume, registers players and listeners, and is updated every frame from the application loop.

### Sound asset, o2::SoundAsset
Stores the encoded audio file data. Supported formats: wav, ogg, mp3, flac. Provides access to the data, duration, channel count and sample rate. Decoding happens in the player.

### Playback, o2::SoundPlayer
The sound player. Inherits from `o2::IAnimation`, so it is controlled like any animation (`Play`/`Stop`, looping, setting the time) and can be sequenced and scrubbed in the animation editor.

Parameters: sound asset, volume, pitch. There is a spatial mode (`spatial`): the source has a position, minimum/maximum audibility distances and an attenuation rolloff factor.

### Listener, o2::SoundListener
The spatial listener point. Registers itself in the sound system; the first active one (`IsListening()`) becomes the listener. If there are no active listeners, the listener follows the current render camera.

### Scene components
- `o2::SoundComponent` - descendant of `Component` and `SoundPlayer`, plays a sound at the actor position, animatable in the animation editor
- `o2::SoundListenerComponent` - descendant of `Component` and `SoundListener`, places the listener at the actor position and orientation

<details><summary>Example</summary>

```cpp
auto actor = mmake<Actor>();
auto sound = actor->AddComponent<SoundComponent>();
sound->SetSound(AssetRef<SoundAsset>("Sounds/shot.wav"));
sound->SetVolume(0.8f);
sound->SetSpatial(true);
sound->Play();
```
</details>
