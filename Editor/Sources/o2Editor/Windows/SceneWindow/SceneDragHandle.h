#pragma once

#include "o2/Utils/Editor/DragHandle.h"

using namespace o2;

namespace Editor
{
    // -----------------
    // Scene drag handle
    // -----------------
    class SceneDragHandle : public DragHandle
    {
    public:
        // Default constructor
        SceneDragHandle(RefCounter* refCounter);

        // Constructor with views
        SceneDragHandle(RefCounter* refCounter, const Ref<IRectDrawable>& regular, const Ref<IRectDrawable>& hover = nullptr, const Ref<IRectDrawable>& pressed = nullptr);

        // Copy-constructor
        SceneDragHandle(RefCounter* refCounter, const SceneDragHandle& other);

        // Destructor
        ~SceneDragHandle();

        // Copy-operator
        SceneDragHandle& operator=(const SceneDragHandle& other);

        // Draws handle, applies scale if mode is scene space
        void Draw() override;

        // Set handle enabled. Disabled handle don't drawn and interact
        void SetEnabled(bool enabled) override;

        // Converts point from screen to local space
        Vec2F ScreenToLocal(const Vec2F& point) override;

        // Converts point from local to screen space
        Vec2F LocalToScreen(const Vec2F& point) override;

        SERIALIZABLE(SceneDragHandle);
        CLONEABLE_REF(SceneDragHandle);

    protected:
        friend class SceneEditScreen;
    };
}
// --- META ---

CLASS_BASES_META(Editor::SceneDragHandle)
{
    BASE_CLASS(o2::DragHandle);
}
END_META;
CLASS_FIELDS_META(Editor::SceneDragHandle)
{
}
END_META;
CLASS_METHODS_META(Editor::SceneDragHandle)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SceneDragHandle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, ScreenToLocal, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, LocalToScreen, const Vec2F&);
}
END_META;
// --- END META ---
