#pragma once
#include "ImageComponent.h"

#include "o2/Assets/Asset.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Assets/Types/AnimationAsset.h"
#include "o2/Assets/Types/DataAsset.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "o2/Scene/Physics/RigidBody.h"
#include "o2/Scene/Tags.h"
#include "o2/Utils/Editor/Attributes/DontDeleteAttribute.h"
#include "o2/Utils/Editor/Attributes/InvokeOnChangeAttribute.h"
#include "o2/Utils/Editor/Attributes/RangeAttribute.h"
#include "o2/Utils/Math/ColorGradient.h"
#include "o2/Utils/Math/Curve.h"

namespace o2
{
    class EditorTestComponent: public Component
    {
    public:
        enum class TestEnum { A, B, C, D, E, F, G, H, K, L, M, N };
         
        class TestInside: public ISerializable, public RefCounterable, public ICloneableRef
        {
        public:
            float mFloat = 1.2f;                // @SERIALIZABLE @SCRIPTABLE
            String mString = String("bla bla"); // @SERIALIZABLE @SCRIPTABLE
            WString mWString;                   // @SERIALIZABLE @SCRIPTABLE
            bool mBool = true;                  // @SERIALIZABLE @SCRIPTABLE
             
            Ref<Component> mComponent; // @SERIALIZABLE @SCRIPTABLE
            Ref<RigidBody> mRigidBody; // @SERIALIZABLE @SCRIPTABLE

            bool operator==(const TestInside& other) const { return false; }

            // @SCRIPTABLE
            TestInside() {}

            SERIALIZABLE(TestInside);
            CLONEABLE_REF(TestInside);
        };

        class TestDerivedInside : public TestInside
        {
        public:
            float mFloatDerived = 1.2f;                // @SERIALIZABLE @SCRIPTABLE
            String mStringDerived = String("bla bla"); // @SERIALIZABLE @SCRIPTABLE
            WString mWStringDerived;                   // @SERIALIZABLE @SCRIPTABLE
            bool mBoolDerived = true;                  // @SERIALIZABLE @SCRIPTABLE

            Ref<Component> mComponentDerived; // @SERIALIZABLE @SCRIPTABLE
            Ref<RigidBody> mRigidBodyDerived; // @SERIALIZABLE @SCRIPTABLE

            // @SCRIPTABLE
            TestDerivedInside() {}

            SERIALIZABLE(TestDerivedInside);
            CLONEABLE_REF(TestDerivedInside);
        };

    public:
        Ref<IRectDrawable> mDrawable;                           // @SERIALIZABLE
        AssetRef<ImageAsset> mImageAsset;                       // @SERIALIZABLE
        LinkRef<Component> mComponent;                          // @SERIALIZABLE
        LinkRef<RigidBody> mRigidBody;                          // @SERIALIZABLE
        LinkRef<ImageComponent> mImageComponent;                // @SERIALIZABLE
        LinkRef<ParticlesEmitterComponent> mParticlesComponent; // @SERIALIZABLE

        PROPERTIES(EditorTestComponent);
        PROPERTY(Ref<Sprite>, spritePropPtr, SetSpritePtr, GetSpritePtr); // @DONT_DELETE @NO_HEADER
        PROPERTY(Sprite, spriteProp, SetSprite, GetSprite); // @DONT_DELETE
        PROPERTY(Vector<Vec2I>, arr, SetArray, GetArray);
                                                               
    public:                                                    
        Vector<Vec2I> mVecs;                                   // @SERIALIZABLE @INVOKE_ON_CHANGE(Test)
        int mInteger = 0;                                      // @SERIALIZABLE @INVOKE_ON_CHANGE(Test)
        float mFloat = 0.0f;                                   // @SERIALIZABLE
        String mString;                                        // @SERIALIZABLE
        WString mWString;                                      // @SERIALIZABLE
        bool mBool = false;                                    // @SERIALIZABLE
        AssetRef<ActorAsset> mActorAsset;                      // @SERIALIZABLE
        AssetRef<DataAsset> mDataAsset;                        // @SERIALIZABLE
        AssetRef<AnimationAsset> mAnimationAsset;              // @SERIALIZABLE
        Ref<Sprite> mSprite = mmake<Sprite>();                 // @SERIALIZABLE @DONT_DELETE
        Ref<Actor> mActor;                                     // @SERIALIZABLE
        TagGroup mTags;                                        // @SERIALIZABLE
        Ref<SceneLayer> mLayer;                                // @SERIALIZABLE
        Color4 mColor;                                         // @SERIALIZABLE
        Vec2F mVec2F;                                          // @SERIALIZABLE
        Vec2I mVec2I;                                          // @SERIALIZABLE
        Vertex mVertex;                                        // @SERIALIZABLE
        RectF mRectF;                                          // @SERIALIZABLE
        RectI mRectI;                                          // @SERIALIZABLE
        BorderF mBorderF;                                      // @SERIALIZABLE
        BorderI mBorderI;                                      // @SERIALIZABLE
        Ref<Curve> mCurve = mmake<Curve>(Curve::EaseInOut());  // @SERIALIZABLE
        Ref<ColorGradient> mGradient;                          // @SERIALIZABLE
        TestInside mTestInside;                                // @SERIALIZABLE
        TestEnum mTestEnum = TestEnum::A;                      // @SERIALIZABLE
        Ref<TestInside> mTestInsideRef;                        // @SERIALIZABLE

        Vector<int> mIntVector;                         // @SERIALIZABLE
        Vector<TestInside> mTestInsideVector;           // @SERIALIZABLE @INVOKE_ON_CHANGE(Test)
        Vector<TestInside*> mTestInsideptrsVector;      // @SERIALIZABLE @INVOKE_ON_CHANGE(Test)
        Vector<Ref<Actor>> mActorVector;                // @SERIALIZABLE
        Vector<AssetRef<AnimationAsset>> mAssetsVector; // @SERIALIZABLE

        Vector<Vector<TestInside*>> mVectorOfVector; // @SERIALIZABLE

        Map<String, String> mDictionary;    // @SERIALIZABLE
        float mFloat2 = 0.0f;               // @SERIALIZABLE @RANGE(0, 10)
        float mFloat3 = 0.0f;               // @SERIALIZABLE
        float mFloat4 = 0.0f;               // @SERIALIZABLE
        float mFloat5 = 0.0f;               // @SERIALIZABLE
        float mFloat6 = 0.0f;               // @SERIALIZABLE
        float mFloat7 = 0.0f;               // @SERIALIZABLE
        float mFloat8 = 0.0f;               // @SERIALIZABLE
        float mFloat9 = 0.0f;               // @SERIALIZABLE

    public:
        static String GetCategory();

        EditorTestComponent();

        void Test();

        void OnStart() override;

        void SetSpritePtr(const Ref<Sprite>& sprite) { mSprite = sprite; }
        const Ref<Sprite>& GetSpritePtr() const { return mSprite; }

        void SetSprite(const Sprite& sprite) { *mSprite = sprite; }
        Sprite GetSprite() const { return *mSprite; }

        void SetArray(const Vector<Vec2I>& arr) { mVecs = arr; }
        const Vector<Vec2I>& GetArray() const { return mVecs; }

        SERIALIZABLE(EditorTestComponent);
        CLONEABLE_REF(EditorTestComponent);
    };
}
// --- META ---

PRE_ENUM_META(o2::EditorTestComponent::TestEnum);

CLASS_BASES_META(o2::EditorTestComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::EditorTestComponent)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mDrawable);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mImageAsset);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mComponent);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mRigidBody);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mImageComponent);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mParticlesComponent);
    FIELD().PUBLIC().DONT_DELETE_ATTRIBUTE().NO_HEADER_ATTRIBUTE().NAME(spritePropPtr);
    FIELD().PUBLIC().DONT_DELETE_ATTRIBUTE().NAME(spriteProp);
    FIELD().PUBLIC().NAME(arr);
    FIELD().PUBLIC().INVOKE_ON_CHANGE_ATTRIBUTE(Test).SERIALIZABLE_ATTRIBUTE().NAME(mVecs);
    FIELD().PUBLIC().INVOKE_ON_CHANGE_ATTRIBUTE(Test).SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(mInteger);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mFloat);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mString);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mWString);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mBool);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mActorAsset);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mDataAsset);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mAnimationAsset);
    FIELD().PUBLIC().DONT_DELETE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(mmake<Sprite>()).NAME(mSprite);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mActor);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mTags);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mLayer);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mColor);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mVec2F);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mVec2I);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mVertex);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mRectF);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mRectI);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mBorderF);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mBorderI);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(mmake<Curve>(Curve::EaseInOut())).NAME(mCurve);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mGradient);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mTestInside);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(TestEnum::A).NAME(mTestEnum);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mTestInsideRef);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mIntVector);
    FIELD().PUBLIC().INVOKE_ON_CHANGE_ATTRIBUTE(Test).SERIALIZABLE_ATTRIBUTE().NAME(mTestInsideVector);
    FIELD().PUBLIC().INVOKE_ON_CHANGE_ATTRIBUTE(Test).SERIALIZABLE_ATTRIBUTE().NAME(mTestInsideptrsVector);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mActorVector);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mAssetsVector);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mVectorOfVector);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mDictionary);
    FIELD().PUBLIC().RANGE_ATTRIBUTE(0, 10).SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mFloat2);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mFloat3);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mFloat4);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mFloat5);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mFloat6);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mFloat7);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mFloat8);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mFloat9);
}
END_META;
CLASS_METHODS_META(o2::EditorTestComponent)
{

    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, Test);
    FUNCTION().PUBLIC().SIGNATURE(void, OnStart);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSpritePtr, const Ref<Sprite>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Sprite>&, GetSpritePtr);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSprite, const Sprite&);
    FUNCTION().PUBLIC().SIGNATURE(Sprite, GetSprite);
    FUNCTION().PUBLIC().SIGNATURE(void, SetArray, const Vector<Vec2I>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Vec2I>&, GetArray);
}
END_META;

CLASS_BASES_META(o2::EditorTestComponent::TestInside)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::EditorTestComponent::TestInside)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.2f).NAME(mFloat);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(String("bla bla")).NAME(mString);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mWString);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mBool);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mComponent);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mRigidBody);
}
END_META;
CLASS_METHODS_META(o2::EditorTestComponent::TestInside)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR();
}
END_META;

CLASS_BASES_META(o2::EditorTestComponent::TestDerivedInside)
{
    BASE_CLASS(o2::EditorTestComponent::TestInside);
}
END_META;
CLASS_FIELDS_META(o2::EditorTestComponent::TestDerivedInside)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.2f).NAME(mFloatDerived);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(String("bla bla")).NAME(mStringDerived);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mWStringDerived);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mBoolDerived);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mComponentDerived);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mRigidBodyDerived);
}
END_META;
CLASS_METHODS_META(o2::EditorTestComponent::TestDerivedInside)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR();
}
END_META;
// --- END META ---
