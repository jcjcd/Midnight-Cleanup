#include "pch.h"
#include "PxUtils.h"

#include <bitset>

#include "MetaCtxs.h"


physx::PxTransform core::RightToLeft(const physx::PxTransform& rightHandTransform)
{
	using namespace physx;

	// 위치의 z 좌표를 반전
	PxVec3 position = rightHandTransform.p;
	position.z = -position.z;

	// 회전의 z축 반전을 위해 적절한 회전 조정
	PxQuat rotation = rightHandTransform.q;
	PxQuat zFlip(0, 0, 1, 0); // 180도 z축 회전
	rotation = rotation * zFlip;

	return PxTransform(position, rotation);
}

physx::PxTransform core::LeftToRight(const physx::PxTransform& leftHandTransform)
{
	using namespace physx;

	// 위치의 z 좌표를 반전
	PxVec3 position = leftHandTransform.p;
	position.z = -position.z;

	// 회전의 z축 반전을 위해 적절한 회전 조정
	PxQuat rotation = leftHandTransform.q;
	PxQuat zFlip(0, 0, 1, 0); // 180도 z축 회전
	rotation = rotation * zFlip;

	return PxTransform(position, rotation);
}

physx::PxFilterFlags core::CustomFilterShader(physx::PxFilterObjectAttributes attributes0,
	physx::PxFilterData filterData0, physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
{
	using namespace physx;
	using namespace physics;

	// 트리거(충돌 감지 전용) 확인
	if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		return PxFilterFlag::eDEFAULT;
	}

	// 충돌 허용 여부 확인
	if (!((filterData0.word0 & filterData1.word1) || (filterData0.word1 & filterData1.word0))) {
		return PxFilterFlag::eSUPPRESS;  // 충돌이 허용되지 않는 경우
	}

	// 콜백 설정
	pairFlags = PxPairFlag::eSOLVE_CONTACT | PxPairFlag::eDETECT_DISCRETE_CONTACT;
	pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eNOTIFY_TOUCH_LOST;

	return PxFilterFlag::eDEFAULT;
}

void core::AddLayer(const char* layerName, entt::id_type layerMask)
{
	using namespace physics;

	// 레이어 마스크로 매핑 확인 및 추가
	if (!layerMaskToNameMap.contains(layerMask))
	{
		if (std::bitset<32>(layerMask).count() == 1)  // 단일 비트 설정 확인
		{
			uint32_t layerIndex = static_cast<uint32_t>(std::log2(layerMask));
			if (layerIndex < collisionMatrix.size())
			{
				layerMaskToNameMap[layerMask] = layerName;

				// 모든 충돌을 허용하도록 초기화
				collisionMatrix[layerIndex] = 0xFFFFFFFF;
				return;
			}
		}

		// 적절한 마스크가 제공되지 않았거나 범위를 벗어난 경우 예외 처리
		throw std::runtime_error("Invalid layer mask or layer index out of range");
	}
}

void core::SetLayerCollision(uint32_t layerMask1, uint32_t layerMask2, bool canCollide)
{
	using namespace physics;

	// 마스크가 등록되어 있는지 확인
	if (!layerMaskToNameMap.contains(layerMask1) || !layerMaskToNameMap.contains(layerMask2))
	{
		throw std::runtime_error("One of the layer masks does not exist.");
	}

	// 각 마스크에 대해 모든 설정된 비트에 대해 충돌 처리
	for (int i = 0; i < 32; ++i)
	{
		if (layerMask1 & (1 << i))
		{
			for (int j = 0; j < 32; ++j)
			{
				if (layerMask2 & (1 << j))
				{
					if (canCollide)
					{
						collisionMatrix[i] |= (1 << j);
						collisionMatrix[j] |= (1 << i);
					}
					else
					{
						collisionMatrix[i] &= ~(1 << j);
						collisionMatrix[j] &= ~(1 << i);
					}
				}
			}
		}
	}
}
