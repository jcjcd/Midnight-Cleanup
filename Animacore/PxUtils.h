#pragma once
#include <physx/PxPhysicsAPI.h>
#include "RaycastHit.h"

#define IS_VALID_CONVERT(FROM, TO) (std::is_same_v<FromType, FROM> && std::is_same_v<ToType, TO>)

namespace physics
{
	inline std::map<uint32_t, const char*> layerMaskToNameMap;  // 레이어 이름 관리
	inline std::array<uint32_t, 32> collisionMatrix; // 충돌 매트릭스 (32 * 32)

	enum class ForceMode
	{
		Force,
		Acceleration,
		Impulse,
		VelocityChange
	};

	enum class QueryTriggerInteraction
	{
		Ignore,
		Collide
	};
}

namespace core
{
	template <typename ToType, typename FromType>
	ToType Convert(const FromType& fromType)
	{
		using namespace physx;
		using namespace physics;

		if constexpr (IS_VALID_CONVERT(PxVec3, Vector3)) {
			return Vector3{ fromType.x, fromType.y, fromType.z };
		}
		else if constexpr (IS_VALID_CONVERT(PxExtendedVec3, Vector3)) {
			return Vector3{
				static_cast<float>(fromType.x),
				static_cast<float>(fromType.y),
				static_cast<float>(fromType.z)
			};
		}
		else if constexpr (IS_VALID_CONVERT(Vector3, PxExtendedVec3)) {
			return PxExtendedVec3{
				static_cast<float>(fromType.x),
				static_cast<float>(fromType.y),
				static_cast<float>(fromType.z)
			};
		}
		else if constexpr (IS_VALID_CONVERT(PxVec2, Vector2)) {
			return Vector2{ fromType.x, fromType.y };
		}
		else if constexpr (IS_VALID_CONVERT(PxQuat, Quaternion)) {
			return Quaternion{ fromType.x, fromType.y, fromType.z, fromType.w };
		}
		else if constexpr (IS_VALID_CONVERT(Vector3, PxVec3)) {
			return PxVec3(fromType.x, fromType.y, fromType.z);
		}
		else if constexpr (IS_VALID_CONVERT(Vector2, PxVec2)) {
			return PxVec2(fromType.x, fromType.y);
		}
		else if constexpr (IS_VALID_CONVERT(Quaternion, PxQuat)) {
			return PxQuat(fromType.x, fromType.y, fromType.z, fromType.w);
		}
		else if constexpr (IS_VALID_CONVERT(ForceMode, PxForceMode::Enum)) {
			switch (fromType)
			{
			case ForceMode::Force:
				return PxForceMode::eFORCE;
			case ForceMode::Impulse:
				return PxForceMode::eIMPULSE;
			case ForceMode::Acceleration:
				return PxForceMode::eACCELERATION;
			case ForceMode::VelocityChange:
				return PxForceMode::eVELOCITY_CHANGE;
			default:
				throw std::runtime_error("Invalid enum values");
			}
		}
		else {
			static_assert(IS_VALID_CONVERT(ToType, FromType), "Unsupported type conversion requested.");
			return ToType{};
		}
	}

	physx::PxTransform RightToLeft(const physx::PxTransform& rightHandTransform);
	physx::PxTransform LeftToRight(const physx::PxTransform& leftHandTransform);

	physx::PxFilterFlags CustomFilterShader(
		physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
		physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize);

	void AddLayer(const char* layerName, entt::id_type layerMask);
	void SetLayerCollision(uint32_t layerMask1, uint32_t layerMask2, bool canCollide);
}
