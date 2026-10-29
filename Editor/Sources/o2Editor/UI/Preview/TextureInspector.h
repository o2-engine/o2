#pragma once

#include "o2/Scene/UI/Widget.h"
#include "o2Editor/UI/Preview/TextureView.h"

using namespace o2;

namespace o2
{
    class HorizontalLayout;
    class Label;
    class Toggle;
}

namespace Editor
{
    // --------------------------------------------------------------------------------------------------
    // Texture view with its controls: fit, one to one, the channel and the background toggles and reread;
    // under it the size, the format, the memory of the texture and the pixel under the cursor. The controls
    // stand in a toolbar of their own or move into the toolbar of the owner
    // --------------------------------------------------------------------------------------------------
    class TextureInspector: public Widget
    {
    public:
        // Default constructor
        TextureInspector(RefCounter* refCounter);

        // Copy-constructor
        TextureInspector(RefCounter* refCounter, const TextureInspector& other);

        // Copy-operator
        TextureInspector& operator=(const TextureInspector& other);

        // Shows the rectangle of the texture, the whole one when it is empty
        void SetTexture(const TextureRef& texture, const RectI& sourceRect = RectI());

        // Sets the rectangles drawn by frames over the texture, in its pixels
        void SetHighlights(const Vector<RectI>& rects);

        // Sets the line of text shown before the texture info
        void SetNote(const WString& note);

        // Returns the text shown before the texture info
        const WString& GetNote() const;

        // Sets the shown channel
        void SetChannel(TextureView::Channel channel);

        // Sets the background under the transparent pixels
        void SetBackground(TextureView::Background background);

        // Moves the controls into the toolbar of the owner at the position; the own toolbar goes away
        void PlaceControlsInto(const Ref<HorizontalLayout>& toolbar, int position);

        // Returns the texture view
        const Ref<TextureView>& GetView() const;

        // Returns the own toolbar
        const Ref<HorizontalLayout>& GetToolbar() const;

        // Returns the group of the controls
        const Ref<HorizontalLayout>& GetControls() const;

        // Returns the channel toggles, in the order of the channels
        const Vector<Ref<Toggle>>& GetChannelToggles() const;

        // Returns the background toggles, in the order of the backgrounds
        const Vector<Ref<Toggle>>& GetBackgroundToggles() const;

        // Returns the text under the view: the texture info and the pixel under the cursor
        WString GetStatusText() const;

        SERIALIZABLE(TextureInspector);
        CLONEABLE_REF(TextureInspector);

    protected:
        Ref<HorizontalLayout> mToolbar;     // Own toolbar
        Ref<HorizontalLayout> mControls;    // Controls, in the own toolbar or in the one of the owner
        Vector<Ref<Toggle>>   mChannels;    // Channel toggles
        Vector<Ref<Toggle>>   mBackgrounds; // Background toggles
        Ref<TextureView>      mView;        // Texture view
        Ref<Label>            mStatus;      // Info and pixel under the cursor
        WString               mNote;        // Text before the info

    protected:
        // Builds the controls
        void Build();

        // Updates the info and the pixel under the cursor
        void UpdateStatus();
    };
}
// --- META ---

CLASS_BASES_META(Editor::TextureInspector)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::TextureInspector)
{
    FIELD().PROTECTED().NAME(mToolbar);
    FIELD().PROTECTED().NAME(mControls);
    FIELD().PROTECTED().NAME(mChannels);
    FIELD().PROTECTED().NAME(mBackgrounds);
    FIELD().PROTECTED().NAME(mView);
    FIELD().PROTECTED().NAME(mStatus);
    FIELD().PROTECTED().NAME(mNote);
}
END_META;
CLASS_METHODS_META(Editor::TextureInspector)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const TextureInspector&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTexture, const TextureRef&, const RectI&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHighlights, const Vector<RectI>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetNote, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(const WString&, GetNote);
    FUNCTION().PUBLIC().SIGNATURE(void, SetChannel, TextureView::Channel);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBackground, TextureView::Background);
    FUNCTION().PUBLIC().SIGNATURE(void, PlaceControlsInto, const Ref<HorizontalLayout>&, int);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<TextureView>&, GetView);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<HorizontalLayout>&, GetToolbar);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<HorizontalLayout>&, GetControls);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<Toggle>>&, GetChannelToggles);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<Toggle>>&, GetBackgroundToggles);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetStatusText);
    FUNCTION().PROTECTED().SIGNATURE(void, Build);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateStatus);
}
END_META;
// --- END META ---
