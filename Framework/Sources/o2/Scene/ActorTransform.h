#pragma once

#include "o2/Utils/Editor/Attributes/PrototypeDeltaSearchAttribute.h"
#include "o2/Utils/Math/Transform.h"
#include "o2/Utils/Math/Vector2.h"

namespace o2
{
	class Actor;
	class ActorTransformData;

	// -------------------------------------------------------------------------------------------
	// Actor transform. Represents the position of actor relative to his parent (the local space),
	// and calculates the position relative to world (the world space).
	// Position shows the pivot point, and pivot shows where is the rectangle of actor.
	// Also this rectangle has angle, scale, size and shear.
	//
	// Note: the rectangle of transform here means the rectangle without angle, scale and shear.
	// So, the final transform is rotated, scaled and sheared rectangle.
	// -------------------------------------------------------------------------------------------
	class ActorTransform: public ISerializable
	{
	public:
		PROPERTIES(ActorTransform);
		GETTER(Actor*, actor, GetOwnerActor); // Owner actor getter 

		PROPERTY(Vec2F, position, SetPosition, GetPosition);            // Position property
		PROPERTY(Vec2F, size, SetSize, GetSize);                        // Size property
		PROPERTY(float, width, SetWidth, GetWidth);                     // Width property
		PROPERTY(float, height, SetHeight, GetHeight);                  // Width property
		PROPERTY(Vec2F, scale, SetScale, GetScale);                     // Scale property
		PROPERTY(Vec2F, pivot, SetPivot, GetPivot);                     // Pivot property, in local space
		PROPERTY(Vec2F, szPivot, SetSizePivot, GetSizePivot);           // Pivot in size space property
		PROPERTY(float, angle, SetAngle, GetAngle);                     // Rotation angle in radians
		PROPERTY(float, angleDegree, SetAngleDegrees, GetAngleDegrees); // Rotation angle in degrees
		PROPERTY(float, shear, SetShear, GetShear);                     // Shear property

		PROPERTY(Basis, basis, SetBasis, GetBasis);                         // Transformation basis property
		PROPERTY(Basis, nonSizedBasis, SetNonSizedBasis, GetNonSizedBasis); // Non sizes transformation basis property

		PROPERTY(RectF, AABB, SetAxisAlignedRect, GetAxisAlignedRect); // Axis aligned rectangle
		PROPERTY(RectF, rect, SetRect, GetRect);                       // Rectangle property. Rectangle - transform without angle, scale and shear. 
											                           // Sets the position and size

		PROPERTY(Vec2F, leftTop, SetLeftTop, GetLeftTop);             // Left top corner property
		PROPERTY(Vec2F, leftBottom, SetLeftBottom, GetLeftBottom);    // Left bottom corner property
		PROPERTY(Vec2F, rightTop, SetRightTop, GetRightTop);          // Right top corner property
		PROPERTY(Vec2F, rightBottom, SetRightBottom, GetRightBottom); // Right bottom corner property
		PROPERTY(Vec2F, center, SetCenter, GetCenter);                // Center position property
		PROPERTY(Vec2F, rightDir, SetRightDir, GetRightDir);          // X axis direction property
		PROPERTY(Vec2F, leftDir, SetLeftDir, GetLeftDir);             // Negative X axis direction property
		PROPERTY(Vec2F, upDir, SetUpDir, GetUpDir);                   // Y axis direction property
		PROPERTY(Vec2F, downDir, SetDownDir, GetDownDir);             // Negative Y axis direction property
		PROPERTY(float, right, SetRight, GetRight);                   // Right border position property
		PROPERTY(float, left, SetLeft, GetLeft);                      // Left border position property
		PROPERTY(float, top, SetTop, GetTop);                         // Top border position property
		PROPERTY(float, bottom, SetBottom, GetBottom);                // Bottom border position property

		PROPERTY(Vec2F, worldPosition, SetWorldPosition, GetWorldPosition);                // World position property
		PROPERTY(Vec2F, worldPivot, SetWorldPivot, GetWorldPivot);                         // Pivot property, in world space
		PROPERTY(float, worldAngle, SetWorldAngle, GetWorldAngle);                         // World rotation angle in radians
		PROPERTY(float, worldAngleDegree, SetWorldAngleDegree, GetWorldAngleDegree);       // World rotation angle in degree
		PROPERTY(Basis, worldBasis, SetWorldBasis, GetWorldBasis);                         // World transformation basis
		PROPERTY(Basis, worldNonSizedBasis, SetWorldNonSizedBasis, GetWorldNonSizedBasis); // World transformation basis without size

		PROPERTY(Vec2F, worldLeftTop, SetWorldLeftTop, GetWorldLeftTop);             // World Left top corner property
		PROPERTY(Vec2F, worldLeftBottom, SetWorldLeftBottom, GetWorldLeftBottom);    // World Left bottom corner property
		PROPERTY(Vec2F, worldRightTop, SetWorldRightTop, GetWorldRightTop);          // World Right top corner property
		PROPERTY(Vec2F, worldRightBottom, SetWorldRightBottom, GetWorldRightBottom); // World Right bottom corner property
		PROPERTY(Vec2F, worldCenter, SetWorldCenter, GetWorldCenter);                // World center property
		PROPERTY(Vec2F, worldRightDir, SetWorldRightDir, GetWorldRightDir);          // World X axis direction property
		PROPERTY(Vec2F, worldLeftDir, SetWorldLeftDir, GetWorldLeftDir);             // World Negative X axis direction property
		PROPERTY(Vec2F, worldUpDir, SetWorldUpDir, GetWorldUpDir);                   // World Y axis direction property
		PROPERTY(Vec2F, worldDownDir, SetWorldDownDir, GetWorldDownDir);             // World Negative Y axis direction property

		PROPERTY(float, worldRight, SetWorldRight, GetWorldRight);    // World Right border position property
		PROPERTY(float, worldLeft, SetWorldLeft, GetWorldLeft);       // World Left border position property
		PROPERTY(float, worldTop, SetWorldTop, GetWorldTop);          // World Top border position property
		PROPERTY(float, worldBottom, SetWorldBottom, GetWorldBottom); // World Bottom border position property

		PROPERTY(RectF, worldRect, SetWorldRect, GetWorldRect);                       // World rectangle property. Sets the position and size
		PROPERTY(RectF, worldAABB, SetWorldAxisAlignedRect, GetWorldAxisAlignedRect); // World direction aligned rectangle

	public:
		ActorTransform(const Vec2F& size = Vec2F(), const Vec2F& position = Vec2F(), float angle = 0.0f,
					   const Vec2F& scale = Vec2F(1.0f, 1.0f), const Vec2F& pivot = Vec2F(0.5f, 0.5f));

		// Copy-constructor
		ActorTransform(const ActorTransform& other);

		// Destructor
		~ActorTransform();

		// Assign operator
		ActorTransform& operator=(const ActorTransform& other);

		// Assign operator
		//ActorTransform& operator=(const Transform& other);

		// Check EqualSid operator
		bool operator==(const ActorTransform& other) const;

		// Returns owner actor
		Actor* GetOwnerActor() const;

		// Sets transform dirty and needed to update
		virtual void SetDirty(bool fromParent = false);

		// Returns is transform dirty
		bool IsDirty() const;

		// Updates transformation
		virtual void Update();

		// Sets position
		virtual void SetPosition(const Vec2F& position);

		// Returns position
		Vec2F GetPosition() const;

		// Sets size
		virtual void SetSize(const Vec2F& size);

		// Return size
		virtual Vec2F GetSize() const;

		// Sets width
		virtual void SetWidth(float value);

		// Return width
		virtual float GetWidth() const;

		// Sets height
		virtual void SetHeight(float value);

		// Return height
		virtual float GetHeight() const;

		// Sets pivot, in local space, where (0, 0) - left down corner, (1; 1) - right top
		virtual void SetPivot(const Vec2F& pivot);

		// Return pivot, in local space, where (0, 0) - left down corner, (1; 1) - right top
		Vec2F GetPivot() const;

		// Sets size pivot, in local space, where (0, 0) - left down corner, (size.x, size.y) - right top
		void SetSizePivot(const Vec2F& relPivot);

		// Returns size pivot, in local space, where (0, 0) - left down corner, (size.x, size.y) - right top
		Vec2F GetSizePivot() const;

		// Sets scale
		void SetScale(const Vec2F& scale);

		// Returns scale
		Vec2F GetScale() const;

		// Sets rotation angle, in radians
		void SetAngle(float rad);

		// Returns rotation angle in radians
		float GetAngle() const;

		// Sets rotation angle, in degrees
		void SetAngleDegrees(float deg);

		// Returns rotation angle in degrees
		float GetAngleDegrees() const;

		// Sets shear
		void SetShear(float shear);

		// Returns shear
		float GetShear() const;

		// Sets basis
		virtual void SetBasis(const Basis& basis);

		// Returns basis
		Basis GetBasis() const;

		// Sets basis without size
		virtual void SetNonSizedBasis(const Basis& basis);

		// Returns basis without size
		Basis GetNonSizedBasis() const;

		// Sets rect
		virtual void SetRect(const RectF& rect);

		// Returns rect
		virtual RectF GetRect() const;

		// Sets direction aligned rectangle transformation
		virtual void SetAxisAlignedRect(const RectF& rect);

		// Returns direction aligned rectangle transformation
		RectF GetAxisAlignedRect() const;

		// Sets left top corner position
		void SetLeftTop(const Vec2F& position);

		// Returns left top corner position
		Vec2F GetLeftTop() const;

		// Sets right top corner position
		void SetRightTop(const Vec2F& position);

		// Returns right top corner position
		Vec2F GetRightTop() const;

		// Sets left down corner position
		void SetLeftBottom(const Vec2F& position);

		// Returns left down corner position
		Vec2F GetLeftBottom() const;

		// Sets left right bottom position
		void SetRightBottom(const Vec2F& position);

		// Returns right bottom corner position
		Vec2F GetRightBottom() const;

		// Sets center position
		void SetCenter(const Vec2F& position);

		// Returns center position
		Vec2F GetCenter() const;

		// Set local right direction
		void SetRightDir(const Vec2F& dir);

		// Returns local right direction
		Vec2F GetRightDir() const;

		// Set local left direction
		void SetLeftDir(const Vec2F& dir);

		// Returns local left direction
		Vec2F GetLeftDir() const;

		// Set local up direction
		void SetUpDir(const Vec2F& dir);

		// Returns local up direction
		Vec2F GetUpDir() const;

		// Set local down direction
		void SetDownDir(const Vec2F& dir);

		// Returns local down direction
		Vec2F GetDownDir() const;

		// Set local right border position
		void SetRight(float value);

		// Returns local right border position
		float GetRight() const;

		// Set local left border position
		void SetLeft(float value);

		// Returns local left border position
		float GetLeft() const;

		// Set local top border position
		void SetTop(float value);

		// Returns local top border position
		float GetTop() const;

		// Set local down border position
		void SetBottom(float value);

		// Returns local down border position
		float GetBottom() const;

		// Transforms point from world space into local
		Vec2F World2LocalPoint(const Vec2F& worldPoint) const;

		// Transforms point from local space into world
		Vec2F Local2WorldPoint(const Vec2F& localPoint) const;

		// Transforms direction from world space into local
		Vec2F World2LocalDir(const Vec2F& worldDir) const;

		// Transforms direction from local space into world
		Vec2F Local2WorldDir(const Vec2F& localDir) const;

		// Returns true when point inside this
		bool IsPointInside(const Vec2F& point) const;

		// Sets world position
		void SetWorldPosition(const Vec2F& position);

		// Returns world position
		Vec2F GetWorldPosition() const;

		// Sets pivot by world coordinates
		void SetWorldPivot(const Vec2F& pivot);

		// Returns pivot position in world coordinates
		Vec2F GetWorldPivot() const;

		// Sets world rotation angle, in radians
		void SetWorldAngle(float rad);

		// Returns world rotation angle in radians
		float GetWorldAngle() const;

		// Sets world rotation angle, in degrees
		void SetWorldAngleDegree(float deg);

		// Returns world rotation angle in degrees
		float GetWorldAngleDegree() const;

		// Sets world basis
		void SetWorldBasis(const Basis& basis);

		// Returns world basis
		Basis GetWorldBasis() const;

		// Sets world basis without size
		void SetWorldNonSizedBasis(const Basis& basis);

		// Returns world basis without size
		Basis GetWorldNonSizedBasis() const;

		// Sets world rect
		void SetWorldRect(const RectF& rect);

		// Returns world rect
		RectF GetWorldRect() const;

		// Sets world direction aligned rectangle transformation
		void SetWorldAxisAlignedRect(const RectF& rect);

		// Returns world direction aligned rectangle transformation
		RectF GetWorldAxisAlignedRect() const;

		// Sets world left top corner position
		void SetWorldLeftTop(const Vec2F& position);

		// Returns world left top corner position
		Vec2F GetWorldLeftTop() const;

		// Sets world right top corner position
		void SetWorldRightTop(const Vec2F& position);

		// Returns world right top corner position
		Vec2F GetWorldRightTop() const;

		// Sets world left down corner position
		void SetWorldLeftBottom(const Vec2F& position);

		// Returns world left down corner position
		Vec2F GetWorldLeftBottom() const;

		// Sets world left right bottom position
		void SetWorldRightBottom(const Vec2F& position);

		// Returns world right bottom corner position
		Vec2F GetWorldRightBottom() const;

		// Sets world center position
		void SetWorldCenter(const Vec2F& position);

		// Returns world center position
		Vec2F GetWorldCenter() const;

		// Set World right direction
		void SetWorldRightDir(const Vec2F& dir);

		// Returns World right direction
		Vec2F GetWorldRightDir() const;

		// Set World left direction
		void SetWorldLeftDir(const Vec2F& dir);

		// Returns World left direction
		Vec2F GetWorldLeftDir() const;

		// Set World up direction
		void SetWorldUpDir(const Vec2F& dir);

		// Returns World up direction
		Vec2F GetWorldUpDir() const;

		// Set World down direction
		void SetWorldDownDir(const Vec2F& dir);

		// Returns World down direction
		Vec2F GetWorldDownDir() const;

		// Set World right border position
		void SetWorldRight(float value);

		// Returns World right border position
		float GetWorldRight() const;

		// Set World left border position
		void SetWorldLeft(float value);

		// Returns World left border position
		float GetWorldLeft() const;

		// Set World top border position
		void SetWorldTop(float value);

		// Returns World top border position
		float GetWorldTop() const;

		// Set World down border position
		void SetWorldBottom(float value);

		// Returns World down border position
		float GetWorldBottom() const;

		SERIALIZABLE(ActorTransform);

	protected:
		ActorTransformData* mData; // Data container. Will be stored in optimized storage @DELTA_SEARCH

	protected:
		// Actor transform constructor with specified data
		ActorTransform(ActorTransformData* data);

		// Copies data parameters from other transform
		virtual void CopyFrom(const ActorTransform& other);

		// Sets owner and updates transform
		virtual void SetOwner(Actor* actor);

		// Returns parent rectange, or zero when no parent
		virtual RectF GetParentRectangle() const;

		// Updates world rectangle and transform relative to parent or origin
		void UpdateWorldRectangleAndTransform();

		// Updates local transformation
		void UpdateTransform();

		// Updates local rectangle
		void UpdateRectangle();

		// Check parentInvertedTransform for actual
		void CheckParentInvTransform();

		// Beginning serialization callback, writes data
		void OnSerialize(DataValue& node) const override;

		// It is called when object was deserialized, reads data
		void OnDeserialized(const DataValue& node) override;

		// Beginning serialization delta callback
		void OnSerializeDelta(DataValue& node, const IObject& origin) const override;

		// Completion deserialization delta callback
		void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

		// Returns parent world rect position - left bottom corner
		Vec2F GetParentPosition() const;

		friend class Actor;
		friend class WidgetLayout;
	};

	class ActorTransformData : public ISerializable
	{
	public:
		int dirtyFrame = 1;  // Frame index, when layout was marked as dirty
		int updateFrame = 1; // Frame index, when layout was updated

		Vec2F position;            // Position @SERIALIZABLE @SERIALIZE_IF(IsSerializeEnabled)
		Vec2F size;                // Size @SERIALIZABLE @SERIALIZE_IF(IsSerializeEnabled)
		Vec2F scale = Vec2F(1, 1); // Scale, (1, 1) is default @SERIALIZABLE @SERIALIZE_IF(IsSerializeEnabled)
		Vec2F pivot;               // Pivot: (0, 0) is left bottom corner - (1, 1) is right top corner @SERIALIZABLE @SERIALIZE_IF(IsSerializeEnabled)
		float angle = 0.0f;        // Rotation angle in radians @SERIALIZABLE @SERIALIZE_IF(IsSerializeEnabled)
		float shear = 0.0f;        // Shear @SERIALIZABLE @SERIALIZE_IF(IsSerializeEnabled)

		RectF rectangle;              // The rectangle in local space
		RectF parentRectangle;        // The parent rectangle
		Vec2F parentRectangePosition; // The parent rectangle pivot position
		RectF worldRectangle;         // The rectangle in world space

		Basis transform;         // Final transform basis
		Basis nonSizedTransform; // Final transform basis without size

		Basis worldNonSizedTransform; // World transform without size
		Basis worldTransform;         // Result world basis

		Basis parentInvertedTransform;       // Parent world transform inverted
		Basis parentTransform;               // Parent world transform
		int   parentInvTransformActualFrame; // last mParentInvertedTransform actual frame index

		Actor* owner = nullptr; // Owner actor 

		SERIALIZABLE(ActorTransformData);

		// Returns is serialize enabled; used to turn off fields serialization
		virtual bool IsSerializeEnabled() const;
	};
}

CLASS_BASES_META(o2::ActorTransform)
{
	BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::ActorTransform)
{
	FIELD().NAME(actor).PUBLIC();
	FIELD().NAME(position).PUBLIC();
	FIELD().NAME(size).PUBLIC();
	FIELD().NAME(width).PUBLIC();
	FIELD().NAME(height).PUBLIC();
	FIELD().NAME(scale).PUBLIC();
	FIELD().NAME(pivot).PUBLIC();
	FIELD().NAME(szPivot).PUBLIC();
	FIELD().NAME(angle).PUBLIC();
	FIELD().NAME(angleDegree).PUBLIC();
	FIELD().NAME(shear).PUBLIC();
	FIELD().NAME(basis).PUBLIC();
	FIELD().NAME(nonSizedBasis).PUBLIC();
	FIELD().NAME(AABB).PUBLIC();
	FIELD().NAME(rect).PUBLIC();
	FIELD().NAME(leftTop).PUBLIC();
	FIELD().NAME(leftBottom).PUBLIC();
	FIELD().NAME(rightTop).PUBLIC();
	FIELD().NAME(rightBottom).PUBLIC();
	FIELD().NAME(center).PUBLIC();
	FIELD().NAME(rightDir).PUBLIC();
	FIELD().NAME(leftDir).PUBLIC();
	FIELD().NAME(upDir).PUBLIC();
	FIELD().NAME(downDir).PUBLIC();
	FIELD().NAME(right).PUBLIC();
	FIELD().NAME(left).PUBLIC();
	FIELD().NAME(top).PUBLIC();
	FIELD().NAME(bottom).PUBLIC();
	FIELD().NAME(worldPosition).PUBLIC();
	FIELD().NAME(worldPivot).PUBLIC();
	FIELD().NAME(worldAngle).PUBLIC();
	FIELD().NAME(worldAngleDegree).PUBLIC();
	FIELD().NAME(worldBasis).PUBLIC();
	FIELD().NAME(worldNonSizedBasis).PUBLIC();
	FIELD().NAME(worldLeftTop).PUBLIC();
	FIELD().NAME(worldLeftBottom).PUBLIC();
	FIELD().NAME(worldRightTop).PUBLIC();
	FIELD().NAME(worldRightBottom).PUBLIC();
	FIELD().NAME(worldCenter).PUBLIC();
	FIELD().NAME(worldRightDir).PUBLIC();
	FIELD().NAME(worldLeftDir).PUBLIC();
	FIELD().NAME(worldUpDir).PUBLIC();
	FIELD().NAME(worldDownDir).PUBLIC();
	FIELD().NAME(worldRight).PUBLIC();
	FIELD().NAME(worldLeft).PUBLIC();
	FIELD().NAME(worldTop).PUBLIC();
	FIELD().NAME(worldBottom).PUBLIC();
	FIELD().NAME(worldRect).PUBLIC();
	FIELD().NAME(worldAABB).PUBLIC();
	FIELD().DELTA_SEARCH_ATTRIBUTE().NAME(mData).PROTECTED();
}
END_META;
CLASS_METHODS_META(o2::ActorTransform)
{

	PUBLIC_FUNCTION(Actor*, GetOwnerActor);
	PUBLIC_FUNCTION(void, SetDirty, bool);
	PUBLIC_FUNCTION(bool, IsDirty);
	PUBLIC_FUNCTION(void, Update);
	PUBLIC_FUNCTION(void, SetPosition, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetPosition);
	PUBLIC_FUNCTION(void, SetSize, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetSize);
	PUBLIC_FUNCTION(void, SetWidth, float);
	PUBLIC_FUNCTION(float, GetWidth);
	PUBLIC_FUNCTION(void, SetHeight, float);
	PUBLIC_FUNCTION(float, GetHeight);
	PUBLIC_FUNCTION(void, SetPivot, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetPivot);
	PUBLIC_FUNCTION(void, SetSizePivot, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetSizePivot);
	PUBLIC_FUNCTION(void, SetScale, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetScale);
	PUBLIC_FUNCTION(void, SetAngle, float);
	PUBLIC_FUNCTION(float, GetAngle);
	PUBLIC_FUNCTION(void, SetAngleDegrees, float);
	PUBLIC_FUNCTION(float, GetAngleDegrees);
	PUBLIC_FUNCTION(void, SetShear, float);
	PUBLIC_FUNCTION(float, GetShear);
	PUBLIC_FUNCTION(void, SetBasis, const Basis&);
	PUBLIC_FUNCTION(Basis, GetBasis);
	PUBLIC_FUNCTION(void, SetNonSizedBasis, const Basis&);
	PUBLIC_FUNCTION(Basis, GetNonSizedBasis);
	PUBLIC_FUNCTION(void, SetRect, const RectF&);
	PUBLIC_FUNCTION(RectF, GetRect);
	PUBLIC_FUNCTION(void, SetAxisAlignedRect, const RectF&);
	PUBLIC_FUNCTION(RectF, GetAxisAlignedRect);
	PUBLIC_FUNCTION(void, SetLeftTop, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetLeftTop);
	PUBLIC_FUNCTION(void, SetRightTop, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetRightTop);
	PUBLIC_FUNCTION(void, SetLeftBottom, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetLeftBottom);
	PUBLIC_FUNCTION(void, SetRightBottom, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetRightBottom);
	PUBLIC_FUNCTION(void, SetCenter, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetCenter);
	PUBLIC_FUNCTION(void, SetRightDir, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetRightDir);
	PUBLIC_FUNCTION(void, SetLeftDir, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetLeftDir);
	PUBLIC_FUNCTION(void, SetUpDir, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetUpDir);
	PUBLIC_FUNCTION(void, SetDownDir, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetDownDir);
	PUBLIC_FUNCTION(void, SetRight, float);
	PUBLIC_FUNCTION(float, GetRight);
	PUBLIC_FUNCTION(void, SetLeft, float);
	PUBLIC_FUNCTION(float, GetLeft);
	PUBLIC_FUNCTION(void, SetTop, float);
	PUBLIC_FUNCTION(float, GetTop);
	PUBLIC_FUNCTION(void, SetBottom, float);
	PUBLIC_FUNCTION(float, GetBottom);
	PUBLIC_FUNCTION(Vec2F, World2LocalPoint, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, Local2WorldPoint, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, World2LocalDir, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, Local2WorldDir, const Vec2F&);
	PUBLIC_FUNCTION(bool, IsPointInside, const Vec2F&);
	PUBLIC_FUNCTION(void, SetWorldPosition, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetWorldPosition);
	PUBLIC_FUNCTION(void, SetWorldPivot, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetWorldPivot);
	PUBLIC_FUNCTION(void, SetWorldAngle, float);
	PUBLIC_FUNCTION(float, GetWorldAngle);
	PUBLIC_FUNCTION(void, SetWorldAngleDegree, float);
	PUBLIC_FUNCTION(float, GetWorldAngleDegree);
	PUBLIC_FUNCTION(void, SetWorldBasis, const Basis&);
	PUBLIC_FUNCTION(Basis, GetWorldBasis);
	PUBLIC_FUNCTION(void, SetWorldNonSizedBasis, const Basis&);
	PUBLIC_FUNCTION(Basis, GetWorldNonSizedBasis);
	PUBLIC_FUNCTION(void, SetWorldRect, const RectF&);
	PUBLIC_FUNCTION(RectF, GetWorldRect);
	PUBLIC_FUNCTION(void, SetWorldAxisAlignedRect, const RectF&);
	PUBLIC_FUNCTION(RectF, GetWorldAxisAlignedRect);
	PUBLIC_FUNCTION(void, SetWorldLeftTop, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetWorldLeftTop);
	PUBLIC_FUNCTION(void, SetWorldRightTop, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetWorldRightTop);
	PUBLIC_FUNCTION(void, SetWorldLeftBottom, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetWorldLeftBottom);
	PUBLIC_FUNCTION(void, SetWorldRightBottom, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetWorldRightBottom);
	PUBLIC_FUNCTION(void, SetWorldCenter, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetWorldCenter);
	PUBLIC_FUNCTION(void, SetWorldRightDir, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetWorldRightDir);
	PUBLIC_FUNCTION(void, SetWorldLeftDir, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetWorldLeftDir);
	PUBLIC_FUNCTION(void, SetWorldUpDir, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetWorldUpDir);
	PUBLIC_FUNCTION(void, SetWorldDownDir, const Vec2F&);
	PUBLIC_FUNCTION(Vec2F, GetWorldDownDir);
	PUBLIC_FUNCTION(void, SetWorldRight, float);
	PUBLIC_FUNCTION(float, GetWorldRight);
	PUBLIC_FUNCTION(void, SetWorldLeft, float);
	PUBLIC_FUNCTION(float, GetWorldLeft);
	PUBLIC_FUNCTION(void, SetWorldTop, float);
	PUBLIC_FUNCTION(float, GetWorldTop);
	PUBLIC_FUNCTION(void, SetWorldBottom, float);
	PUBLIC_FUNCTION(float, GetWorldBottom);
	PROTECTED_FUNCTION(void, CopyFrom, const ActorTransform&);
	PROTECTED_FUNCTION(void, SetOwner, Actor*);
	PROTECTED_FUNCTION(RectF, GetParentRectangle);
	PROTECTED_FUNCTION(void, UpdateWorldRectangleAndTransform);
	PROTECTED_FUNCTION(void, UpdateTransform);
	PROTECTED_FUNCTION(void, UpdateRectangle);
	PROTECTED_FUNCTION(void, CheckParentInvTransform);
	PROTECTED_FUNCTION(void, OnSerialize, DataValue&);
	PROTECTED_FUNCTION(void, OnDeserialized, const DataValue&);
	PROTECTED_FUNCTION(void, OnSerializeDelta, DataValue&, const IObject&);
	PROTECTED_FUNCTION(void, OnDeserializedDelta, const DataValue&, const IObject&);
	PROTECTED_FUNCTION(Vec2F, GetParentPosition);
}
END_META;

CLASS_BASES_META(o2::ActorTransformData)
{
	BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::ActorTransformData)
{
	FIELD().DEFAULT_VALUE(1).NAME(dirtyFrame).PUBLIC();
	FIELD().DEFAULT_VALUE(1).NAME(updateFrame).PUBLIC();
	FIELD().SERIALIZABLE_ATTRIBUTE().SERIALIZE_IF_ATTRIBUTE(IsSerializeEnabled).NAME(position).PUBLIC();
	FIELD().SERIALIZABLE_ATTRIBUTE().SERIALIZE_IF_ATTRIBUTE(IsSerializeEnabled).NAME(size).PUBLIC();
	FIELD().SERIALIZABLE_ATTRIBUTE().SERIALIZE_IF_ATTRIBUTE(IsSerializeEnabled).DEFAULT_VALUE(Vec2F(1, 1)).NAME(scale).PUBLIC();
	FIELD().SERIALIZABLE_ATTRIBUTE().SERIALIZE_IF_ATTRIBUTE(IsSerializeEnabled).NAME(pivot).PUBLIC();
	FIELD().SERIALIZABLE_ATTRIBUTE().SERIALIZE_IF_ATTRIBUTE(IsSerializeEnabled).DEFAULT_VALUE(0.0f).NAME(angle).PUBLIC();
	FIELD().SERIALIZABLE_ATTRIBUTE().SERIALIZE_IF_ATTRIBUTE(IsSerializeEnabled).DEFAULT_VALUE(0.0f).NAME(shear).PUBLIC();
	FIELD().NAME(rectangle).PUBLIC();
	FIELD().NAME(parentRectangle).PUBLIC();
	FIELD().NAME(parentRectangePosition).PUBLIC();
	FIELD().NAME(worldRectangle).PUBLIC();
	FIELD().NAME(transform).PUBLIC();
	FIELD().NAME(nonSizedTransform).PUBLIC();
	FIELD().NAME(worldNonSizedTransform).PUBLIC();
	FIELD().NAME(worldTransform).PUBLIC();
	FIELD().NAME(parentInvertedTransform).PUBLIC();
	FIELD().NAME(parentTransform).PUBLIC();
	FIELD().NAME(parentInvTransformActualFrame).PUBLIC();
	FIELD().DEFAULT_VALUE(nullptr).NAME(owner).PUBLIC();
}
END_META;
CLASS_METHODS_META(o2::ActorTransformData)
{

	PUBLIC_FUNCTION(bool, IsSerializeEnabled);
}
END_META;
