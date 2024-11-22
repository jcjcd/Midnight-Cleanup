#pragma once

namespace core
{
	/*------------------------------
		Rigidbody
	------------------------------*/
	struct Rigidbody
	{
		enum class Interpolation
		{
			None,
			Interpolate,
			Extrapolate
		};
		enum class Constraints
		{
			None = 0,
			FreezePositionX = (1 << 0),
			FreezePositionY = (1 << 1),
			FreezePositionZ = (1 << 2),
			FreezeRotationX = (1 << 3),
			FreezeRotationY = (1 << 4),
			FreezeRotationZ = (1 << 5),
			FreezePosition = FreezePositionX | FreezePositionY | FreezePositionZ,
			FreezeRotation = FreezeRotationX | FreezeRotationY | FreezeRotationZ,
			FreezeAll = FreezePosition | FreezeRotation,
		};

		float mass = 0.f;
		float drag = 0.f;
		float angularDrag = 0.05f;
		bool useGravity = false;
		bool isKinematic = false;
		bool isDisabled = false;
		Interpolation interpolation = Interpolation::None;
		Constraints constraints = Constraints::None;
		Vector3 velocity;
		Vector3 angularVelocity;
	};

	/*------------------------------
		Joint
	------------------------------*/
	struct JointCommon
	{
		Vector3 anchor;
		Vector3 axis;
		Rigidbody* connectedBody = nullptr;
	};

	struct Joint
	{

	};

	/*------------------------------
		Collider
	------------------------------*/
	struct ColliderCommon
	{
		struct PhysicMaterial
		{
			float dynamicFriction = 0.6f;
			float staticFriction = 0.6f;
			float bounciness = 0.f;
		};

		bool isTrigger = false;
		std::string materialName = "./Resources/Physic Material/default.pmaterial";
		Vector3 center;
		float contactOffset = 0.1f;
	};

	struct BoxCollider
	{
		Vector3 size = Vector3::One;
	};

	struct SphereCollider
	{
		float radius = 1.f;
	};

	struct CapsuleCollider
	{
		enum Axis
		{
			X,
			Y,
			Z,
		};

		float radius = 1.f;
		float height = 1.f;
		Axis direction = Y;
	};

	struct MeshCollider
	{
		enum MeshColliderCookingOptions
		{
			None = 0,
			CookForFasterSimulation = (1 << 0),	// 빠른 콜라이더 생성, 런타임 충돌 처리 성능 저하
			DisableMeshCleaning = (1 << 1),		// 임의 메시 삭제 기능 제거, 빠른 콜라이더 생성
			WeldColocatedVertices = (1 << 2),	// 겹쳐지는 버텍스 병합 (DisableMeshCleaning 과 함께 사용 불가)
			UseLegacyMidphase = (1 << 3),		// 기존 eBVH33 메시 구조로 사용 (호환성)
			BuildGPUData = (1 << 4),			// GPU 가속 활성화, 런타임 성능 향상, 쿠킹 속도 저하
		};

		bool convex = false;
		MeshColliderCookingOptions cookingOptions = MeshColliderCookingOptions::None;
	};

	/*------------------------------
		CharacterController
	------------------------------*/
	struct CharacterController
	{
		enum CollisionFlags
		{
			None = 0,			// 충돌 없음
			Sides = (1 << 0),	// 측면 충돌
			Above = (1 << 1),	// 상단 충돌
			Below = (1 << 2)	// 하단 충돌
		};

		float slopeLimit = 45.0f;
		float stepOffset = 0.5f;
		float skinWidth = 0.1f;
		float radius = 0.f;
		float height = 0.f;
		float density = 10.0f;

		// 스크립트에서 사용
		float mass = 0.f;
		bool isGrounded = false;
		CollisionFlags collisionFlags = CollisionFlags::None;
	};
}
