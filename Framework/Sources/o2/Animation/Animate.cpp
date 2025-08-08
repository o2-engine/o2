#include "o2/stdafx.h"
#include "Animate.h"

namespace o2
{

    Animate::Animate(IObject& object):
        mTarget(&object)
    {}

    Animate::operator Ref<AnimationClip>() const
    {
        return mAnimation;
    }

    Animate& Animate::Wait(float seconds)
    {
        mTime += seconds;

        for (auto& container : mKeyContainers)
            container->Apply(mTime);

        return *this;
    }

    Animate& Animate::For(float seconds)
    {
        mTime += seconds;
        ApplyKeys();

        return *this;
    }

    void Animate::ApplyKeys()
    {
        for (auto& container : mKeyContainers)
            container->Apply(mTime);

        mKeysApplied = true;
    }

    Animate& Animate::Then()
    {
        if (!mKeysApplied)
            ApplyKeys();

        return *this;
    }

    Animate& Animate::Move(float x, float y)
    {
        CheckPositionAnimatedValue();
        CheckAppliedKeys();

        auto container = mmake<KeyContainer<Vec2F>>();
        container->animatedValue = mPositionAnimatedValue;
        container->animatedValue->spline->AppendKey(Vec2F(x, y));
        mKeyContainers.Add(container);

        return *this;
    }

    Animate& Animate::Move(const Vec2F& point)
    {
        CheckPositionAnimatedValue();
        CheckAppliedKeys();

        auto container = mmake<KeyContainer<Vec2F>>();
        container->animatedValue = mPositionAnimatedValue;
        container->animatedValue->spline->AppendKey(point);
        container->timeKey.position = mTime;
        mKeyContainers.Add(container);

        return *this;
    }

    Animate& Animate::Move(const Vector<Vec2F>& points)
    {
        return *this;
    }

    Animate& Animate::Alpha(float alpha)
    {
        CheckColorAnimatedValue();
        CheckAppliedKeys();

        auto container = mmake<KeyContainer<Color4>>();
        container->animatedValue = mColorAnimatedValue;
        container->key.value = Color4(1.0f, 1.0f, 1.0f, alpha);
        mKeyContainers.Add(container);

        return *this;
    }

    Animate& Animate::Show()
    {
        return Alpha(1.0f);
    }

    Animate& Animate::Hide()
    {
        return Alpha(0.0f);
    }

    Animate& Animate::Color(const Color4& color)
    {
        CheckColorAnimatedValue();
        CheckAppliedKeys();

        auto container = mmake<KeyContainer<Color4>>();
        container->animatedValue = mColorAnimatedValue;
        container->key.value = color;
        mKeyContainers.Add(container);

        return *this;
    }

    Animate& Animate::Color(int r, int g, int b, int a)
    {
        return Color(Color4(r, g, b, a));
    }

    Animate& Animate::Scale(float scale)
    {
        return Scale(Vec2F(scale, scale));
    }

    Animate& Animate::Scale(const Vec2F& scale)
    {
        CheckScaleAnimatedValue();
        CheckAppliedKeys();

        auto container = mmake<ScaleKeyContainer>();
        container->animatedValueX = mScaleXAnimatedValue;
        container->animatedValueY = mScaleYAnimatedValue;
        container->keyX.value = scale.x;
        container->keyY.value = scale.y;
        mKeyContainers.Add(container);

        return *this;
    }

    Animate& Animate::Rotate(float angle)
    {
        CheckRotateAnimatedValue();
        CheckAppliedKeys();

        auto container = mmake<KeyContainer<float>>();
        container->animatedValue = mRotationAnimatedValue;
        container->key.value = angle;
        mKeyContainers.Add(container);

        return *this;
    }

    Animate& Animate::Looped()
    {
        mAnimation->SetLoop(Loop::Repeat);
        return *this;
    }

    Animate& Animate::PingPong()
    {
        mAnimation->SetLoop(Loop::PingPong);
        return *this;
    }

    void Animate::CheckColorAnimatedValue()
    {
        if (mColorAnimatedValue)
            return;

        static Vector<String> nameVariants ={ "color", "mColor", "m_color", "_color" };

        auto& targetObjType = dynamic_cast<const ObjectType&>(mTarget->GetType());
        void* target = targetObjType.DynamicCastFromIObject(mTarget);
        for (auto& nameVariant : nameVariants)
        {
            const FieldInfo* fi;
            if (targetObjType.GetFieldPtr(target, nameVariant, fi))
            {
                mColorAnimatedValue = mAnimation->AddTrack<Color4>(nameVariant);
                return;
            }
        }
    }

    void Animate::CheckPositionAnimatedValue()
    {
        if (mPositionAnimatedValue)
            return;

        static Vector<String> nameVariants ={ "position", "mPosition", "m_position", "_position", "pos" };

        auto& targetObjType = dynamic_cast<const ObjectType&>(mTarget->GetType());
        void* target = targetObjType.DynamicCastFromIObject(mTarget);
        for (auto& nameVariant : nameVariants)
        {
            const FieldInfo* fi;
            if (targetObjType.GetFieldPtr(target, nameVariant, fi))
            {
                mPositionAnimatedValue = mAnimation->AddTrack<Vec2F>(nameVariant);
                return;
            }
        }
    }

    void Animate::CheckScaleAnimatedValue()
    {
        if (mScaleXAnimatedValue)
            return;

        static Vector<String> nameVariantsX = { "scaleX", "mScaleX", "m_scaleX", "_scaleX" };
        static Vector<String> nameVariantsY = { "scaleY", "mScaleY", "m_scaleY", "_scaleY" };

        auto& targetObjType = dynamic_cast<const ObjectType&>(mTarget->GetType());
        void* target = targetObjType.DynamicCastFromIObject(mTarget);
        for (auto& nameVariant : nameVariantsX)
        {
            const FieldInfo* fi;
            if (targetObjType.GetFieldPtr(target, nameVariant, fi))
            {
                mScaleXAnimatedValue = mAnimation->AddTrack<float>(nameVariant);
                break;
            }
        }

        for (auto& nameVariant : nameVariantsY)
        {
            const FieldInfo* fi;
            if (targetObjType.GetFieldPtr(target, nameVariant, fi))
            {
                mScaleYAnimatedValue = mAnimation->AddTrack<float>(nameVariant);
                break;
            }
        }
    }

    void Animate::CheckRotateAnimatedValue()
    {
        if (mRotationAnimatedValue)
            return;

        static Vector<String> nameVariants ={ "angle", "mAngle", "mRotation", "m_angle", "m_rotation", "rotation", "rot" };
        
        auto& targetObjType = dynamic_cast<const ObjectType&>(mTarget->GetType());
        void* target = targetObjType.DynamicCastFromIObject(mTarget);
        for (auto& nameVariant : nameVariants)
        {
            const FieldInfo* fi;
            if (targetObjType.GetFieldPtr(target, nameVariant, fi))
            {
                mRotationAnimatedValue = mAnimation->AddTrack<float>(nameVariant);
                return;
            }
        }
    }

    void Animate::CheckAppliedKeys()
    {
        if (!mKeysApplied)
            return;

        mKeyContainers.Clear();
        mFunction.Clear();

        mKeysApplied = false;
    }

    void KeyContainer<Vec2F>::Apply(float time)
    {
        timeKey.value = 0.0f;
        animatedValue->timeCurve->InsertKey(timeKey);

        timeKey.position = time;
        timeKey.value = 1.0f;
        animatedValue->timeCurve->InsertKey(timeKey);
    }

    void ScaleKeyContainer::Apply(float time)
    {
        keyX.position = time;
        keyX.supportsType = Curve::Key::Type::Free;
        animatedValueX->AddKey(keyX);

        keyY.position = time;
        keyY.supportsType = Curve::Key::Type::Free;
        animatedValueY->AddKey(keyY);
    }

}
