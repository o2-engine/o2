#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Utils/Singleton.h"

using namespace o2;

namespace o2
{
    class Button;
    class DropDown;
    class HorizontalLayout;
    class Label;
    class Widget;
    class Window;
}

namespace Editor
{
    FORWARD_CLASS_REF(FontSampleView);
    FORWARD_CLASS_REF(ModelView);
    FORWARD_CLASS_REF(SoundPlayerView);
    FORWARD_CLASS_REF(TextureInspector);
    FORWARD_CLASS_REF(VectorImageView);
    FORWARD_CLASS_REF(VideoView);

    // ------------------------------------------------------------------------------------------------------
    // Window to look at an asset closer than the Properties window shows it. The modes depend on the asset:
    // an image - Image, Atlas page; an atlas - its pages; a vector image - Image, Mesh, Rasters; a font - Text,
    // Atlas; a model - Shaded, Wireframe, Shaded wireframe, and the animation clips of a skinned one; a video
    // and a sound - their players
    // ------------------------------------------------------------------------------------------------------
    class AssetPreviewWindow: public Singleton<AssetPreviewWindow>
    {
    public:
        // Constructor
        AssetPreviewWindow(RefCounter* refCounter);

        // Destructor
        ~AssetPreviewWindow();

        // Shows the window with the asset, in the mode when it is given
        static void Show(const AssetRef<Asset>& asset, const String& mode = "");

        // Creates the button showing the asset the function returns in the window
        static Ref<Button> CreateOpenButton(const Function<AssetRef<Asset>()>& getAsset, const WString& caption = "Open preview",
                                            const String& mode = "");

        // Shows the asset in its first mode
        void SetAsset(const AssetRef<Asset>& asset);

        // Returns the shown asset
        const AssetRef<Asset>& GetAsset() const;

        // Returns the modes of the shown asset
        const Vector<String>& GetModes() const;

        // Sets the mode by name; an unknown one is ignored
        void SetMode(const String& mode);

        // Returns the current mode
        const String& GetMode() const;

        // Returns the widget showing the asset in the current mode
        const Ref<Widget>& GetContent() const;

        // Returns the info line of the toolbar
        WString GetInfoText() const;

        // Returns the window
        const Ref<o2::Window>& GetWindow() const;

        // Returns the texture inspector of the texture modes
        const Ref<TextureInspector>& GetTextureInspector() const;

        // Returns the vector image view
        const Ref<VectorImageView>& GetVectorImageView() const;

        // Returns the font sample
        const Ref<FontSampleView>& GetFontSampleView() const;

        // Returns the model view
        const Ref<ModelView>& GetModelView() const;

        // Returns the video player
        const Ref<VideoView>& GetVideoView() const;

        // Returns the sound player
        const Ref<SoundPlayerView>& GetSoundPlayerView() const;

    protected:
        Ref<o2::Window>       mWindow;      // Window
        Ref<HorizontalLayout> mToolbar;       // Mode, animation, the controls of the content and info
        Ref<DropDown>         mModesList;     // Modes of the asset
        Ref<DropDown>         mAnimations;    // Animation clips of a skinned model
        Ref<Button>           mAnimationPlay; // Plays the animation clip
        Ref<Button>           mFit;           // Fits a vector image or a model; a texture has its own
        Ref<Label>            mInfo;          // Info about the asset in the mode
        Ref<Widget>           mContentArea; // Area of the content
        Ref<Widget>           mContent;     // Current content
        Ref<Label>            mNoPreview;   // Content of an asset without a preview

        Ref<TextureInspector> mTextureInspector; // Textures, atlas pages, glyph atlases and vector rasters
        Ref<VectorImageView>  mVectorImageView;  // Vector images
        Ref<FontSampleView>   mFontSampleView;   // Fonts
        Ref<ModelView>        mModelView;        // Meshes and skinned models
        Ref<VideoView>        mVideoView;        // Videos
        Ref<SoundPlayerView>  mSoundPlayerView;  // Sounds

        AssetRef<Asset> mAsset; // Shown asset
        Vector<String>  mModes; // Modes of the asset
        String          mMode;  // Current mode

    protected:
        // Builds the window
        void InitializeWindow();

        // Shows the content in the area, hides the others
        void ShowContent(const Ref<Widget>& content);

        // Shows the asset in the current mode
        void ApplyMode();

        // Stops the playback of the players
        void StopPlayback();

        // Shows the image or its atlas page
        void ApplyImageMode();

        // Shows a page of the atlas
        void ApplyAtlasMode();

        // Shows the vector image, its mesh or its rasters
        void ApplyVectorImageMode();

        // Shows the font sample or its atlas
        void ApplyFontMode();

        // Shows the model
        void ApplyModelMode();
    };
}
