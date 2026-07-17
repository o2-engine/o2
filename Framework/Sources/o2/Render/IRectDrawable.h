#pragma once

#include "o2/Assets/Types/MaterialAsset.h"
#include "o2/Render/IDrawable.h"
#include "o2/Render/Material.h"
#include "o2/Utils/Editor/Attributes/RangeAttribute.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Transform.h"
#include "o2/Utils/Property.h"

namespace o2
{
    // -----------------------------
    // Basic rect drawable interface
    // -----------------------------
    class IRectDrawable: public Transform, public virtual IDrawable, virtual public RefCounterable, virtual public ICloneableRef
    {
    public:
		PROPERTIES(IRectDrawable);
		PROPERTY(bool, enabled, SetEnabled, IsEnabled); // Enable property @SCRIPTABLE

		PROPERTY(Color4, color, SetColor, GetColor);                         // Color property @SCRIPTABLE
		PROPERTY(Color4, overrideColor, SetOverrideColor, GetOverrideColor); // Override color property, used to modify color from outside @SCRIPTABLE
		PROPERTY(float, transparency, SetTransparency, GetTransparency);     // Transparency property, changing alpha in color @SCRIPTABLE @RANGE(0, 1)

		PROPERTY(AssetRef<MaterialAsset>, material, SetMaterialAsset, GetMaterialAsset);         // Material (asset) for rendering @SCRIPTABLE
		ACCESSOR(Ref<IShaderParam>, shaderParam, String, GetShaderParam, GetAllShaderParamsMap); // Shader parameter accessor by name

    public:
        // Constructor
        IRectDrawable(const Vec2F& size = Vec2F(), const Vec2F& position = Vec2F(), float angle = 0.0f, 
                      const Vec2F& scale = Vec2F(1.0f, 1.0f), const Color4& color = Color4::White(), 
                      const Vec2F& pivot = Vec2F(0.5f, 0.5f));

        // Copy-constructor
        IRectDrawable(const IRectDrawable& other);

        // Virtual destructor
        ~IRectDrawable() override {}

        // Assign operator
        IRectDrawable& operator=(const IRectDrawable& other);

        // Equals operator
        bool operator==(const IRectDrawable& other) const;

        // Not equals operator
        bool operator!=(const IRectDrawable& other) const;

        // Drawing
        virtual void Draw() override {}

        // Sets color @SCRIPTABLE
        virtual void SetColor(const Color4& color);

        // Returns color @SCRIPTABLE
        virtual Color4 GetColor() const;

        // Sets color override, used to modify color from outside
        virtual void SetOverrideColor(const Color4& color);

        // Returns color override, used to modify color from outside
        virtual Color4 GetOverrideColor() const;

        // Sets transparency. Changing color alpha @SCRIPTABLE
        virtual void SetTransparency(float transparency);

        // Returns transparency(color alpha) @SCRIPTABLE
        virtual float GetTransparency() const;

        // Sets enabled @SCRIPTABLE
        virtual void SetEnabled(bool enabled);

        // Returns enabled @SCRIPTABLE
        virtual bool IsEnabled() const;

        // Returns true if point is under drawable
        bool IsUnderPoint(const Vec2F& point) override;

        // Sets material by asset reference. GetMaterial() will return asset's material.
        void SetMaterialAsset(const AssetRef<MaterialAsset>& asset);

        // Returns material asset reference, or null if not set. GetMaterial() will return asset's material if asset is set.
        const AssetRef<MaterialAsset>& GetMaterialAsset() const;

        // Sets material for rendering. Pass nullptr for default material. Clears material asset. @SCRIPTABLE
        void SetMaterial(const Ref<Material>& material) override;

        // Returns current material (from material asset if set, else direct override; may be null)
        Ref<Material> GetMaterial() const override;

        // Returns a shader parameter by uniform name from the current material, or nullptr if not found
        Ref<IShaderParam> GetShaderParam(const String& name) const;

        // Returns all shader parameters from the current material as a name-to-parameter map
        Map<String, Ref<IShaderParam>> GetAllShaderParamsMap() const;

        SERIALIZABLE(IRectDrawable);
        CLONEABLE_REF(IRectDrawable);

    protected:
        Color4 mColor = Color4::White();          // Color @SERIALIZABLE
        Color4 mOverrideColor = Color4::White();  // Override color, used to modify color from outside
        Color4 mResultColor;                      // Result color, calculated from color and override color

        bool mEnabled = true; // True, when drawable enabled and needs to draw @SERIALIZABLE

        AssetRef<MaterialAsset> mMaterialAsset;   // Material asset (when set, GetMaterial uses its material) @SERIALIZABLE

    protected:
        // Updates result color from color and override color
        void UpdateColor();

        // Called when color was changed
        virtual void OnColorChanged() {}

        // Called when enabling changed
        virtual void OnEnableChanged() {}

		// Called when material or material asset was changed (by user or code)
        void OnMaterialChanged() override;
    };

    // -----------------------------
    // Functional rectangle drawable
    // -----------------------------
    class FunctionalRectDrawable: public IRectDrawable
    {
    public:
        Function<void(const Basis& transform, const Color4& color)> draw; // Draw function

    public:
        // Default constructor
        FunctionalRectDrawable();

        // Default constructor woth parameters
        FunctionalRectDrawable(const Function<void(const Basis& transform, const Color4& color)>& draw,
                               const Vec2F& size = Vec2F(), const Vec2F& position = Vec2F(), float angle = 0.0f,
                               const Vec2F& scale = Vec2F(1.0f, 1.0f), const Color4& color = Color4::White(),
                               const Vec2F& pivot = Vec2F(0.5f, 0.5f));

        // Copy constructor
        FunctionalRectDrawable(const FunctionalRectDrawable& other);

        // Calls draw function to draw
        void Draw() override;

        SERIALIZABLE(FunctionalRectDrawable);
    };
}
// --- META ---

CLASS_BASES_META(o2::IRectDrawable)
{
    BASE_CLASS(o2::Transform);
    BASE_CLASS(o2::IDrawable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::IRectDrawable)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(enabled);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(color);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(overrideColor);
    FIELD().PUBLIC().RANGE_ATTRIBUTE(0, 1).SCRIPTABLE_ATTRIBUTE().NAME(transparency);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(material);
    FIELD().PUBLIC().NAME(shaderParam);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color4::White()).NAME(mColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4::White()).NAME(mOverrideColor);
    FIELD().PROTECTED().NAME(mResultColor);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mEnabled);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mMaterialAsset);
}
END_META;
CLASS_METHODS_META(o2::IRectDrawable)
{

    typedef Map<String, Ref<IShaderParam>> _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR(const Vec2F&, const Vec2F&, float, const Vec2F&, const Color4&, const Vec2F&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const IRectDrawable&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetColor, const Color4&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Color4, GetColor);
    FUNCTION().PUBLIC().SIGNATURE(void, SetOverrideColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(Color4, GetOverrideColor);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetTransparency, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetTransparency);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetEnabled, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsEnabled);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaterialAsset, const AssetRef<MaterialAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<MaterialAsset>&, GetMaterialAsset);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetMaterial, const Ref<Material>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Material>, GetMaterial);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IShaderParam>, GetShaderParam, const String&);
    FUNCTION().PUBLIC().SIGNATURE(_tmp1, GetAllShaderParamsMap);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateColor);
    FUNCTION().PROTECTED().SIGNATURE(void, OnColorChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnableChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMaterialChanged);
}
END_META;

CLASS_BASES_META(o2::FunctionalRectDrawable)
{
    BASE_CLASS(o2::IRectDrawable);
}
END_META;
CLASS_FIELDS_META(o2::FunctionalRectDrawable)
{
    FIELD().PUBLIC().NAME(draw);
}
END_META;
CLASS_METHODS_META(o2::FunctionalRectDrawable)
{

    typedef const Function<void(const Basis& transform, const Color4& color)>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(_tmp1, const Vec2F&, const Vec2F&, float, const Vec2F&, const Color4&, const Vec2F&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const FunctionalRectDrawable&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
}
END_META;
// --- END META ---
