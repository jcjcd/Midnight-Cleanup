
#include "pch.h"
#include "CollisionCallback.h"

#include "Scene.h"
#include "CoreTagsAndLayers.h"
#include "SystemInterface.h"
#include "CoreSystemEvents.h"

#include <physx/PxActor.h>

#include "CorePhysicsComponents.h"


#pragma region CollisionCallback
core::CollisionCallback::CollisionCallback(Scene& scene)
	: _scene(&scene)
{
	const auto dispatcher = _scene->GetDispatcher();

	// 콜리전 핸들러 시스템 등록/해제 이벤트 연결
	dispatcher->sink<OnRegisterCollisionHandler>()
		.connect<&CollisionCallback::registerCollisionHandler>(this);
	dispatcher->sink<OnRemoveCollisionHandler>()
		.connect<&CollisionCallback::removeCollisionHandler>(this);
}

void core::CollisionCallback::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
	for (physx::PxU32 i = 0; i < nbPairs; i++)
	{
		const physx::PxContactPair& cp = pairs[i];

		// 충돌 시작 이벤트 처리
		processCollisionEvent(cp, pairHeader, physx::PxPairFlag::eNOTIFY_TOUCH_FOUND, &ICollisionHandler::OnCollisionEnter);

		// 충돌 지속 이벤트 처리
		processCollisionEvent(cp, pairHeader, physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS, &ICollisionHandler::OnCollisionStay);

		// 충돌 종료 이벤트 처리
		processCollisionEvent(cp, pairHeader, physx::PxPairFlag::eNOTIFY_TOUCH_LOST, &ICollisionHandler::OnCollisionExit);
	}
}

void core::CollisionCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
{
	for (physx::PxU32 i = 0; i < count; i++)
	{
		const physx::PxTriggerPair& pair = pairs[i];

		if (!pair.triggerActor->userData || !pair.otherActor->userData)
			continue;

		auto registry = _scene->GetRegistry();

		// 모든 액터는 userData로 entt::entity를 가지고 있다고 가정
		entt::entity entityA = { *static_cast<entt::entity*>(pair.triggerActor->userData) };
		entt::entity entityB = { *static_cast<entt::entity*>(pair.otherActor->userData) };

		auto processEntity = [&](entt::entity self, entt::entity other, auto callback)
			{
				if (auto tag = registry->try_get<Tag>(self))
				{
					auto range = _handlers.equal_range(tag->id);
					for (auto it = range.first; it != range.second; ++it)
					{
						(it->second->*callback)(self, other, *registry);
					}
				}
			};

		// 트리거 엔터 이벤트 처리
		if (pair.status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
			processEntity(entityA, entityB, &ICollisionHandler::OnCollisionEnter);

		// 트리거 스테이 이벤트 처리
		if (pair.status == physx::PxPairFlag::eDETECT_DISCRETE_CONTACT)
			processEntity(entityA, entityB, &ICollisionHandler::OnCollisionStay);

		// 트리거 엑싯 이벤트 처리
		if (pair.status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
			processEntity(entityA, entityB, &ICollisionHandler::OnCollisionExit);
	}
}

void core::CollisionCallback::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
{
}

void core::CollisionCallback::onWake(physx::PxActor** actors, physx::PxU32 count)
{
}

void core::CollisionCallback::onSleep(physx::PxActor** actors, physx::PxU32 count)
{
}

void core::CollisionCallback::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
{
}

void core::CollisionCallback::processCollisionEvent(const physx::PxContactPair& cp,
	const physx::PxContactPairHeader& pairHeader, physx::PxPairFlag::Enum eventFlag,
	void (ICollisionHandler::* callback)(entt::entity, entt::entity, entt::registry&))
{
	if (cp.events & eventFlag)
	{
		if (!pairHeader.actors[0]->userData or !pairHeader.actors[1]->userData)
			return;

		auto registry = _scene->GetRegistry();

		// 모든 액터는 userData 로 entt::entity 를 가지고 있다고 가정
		entt::entity entityA = { *static_cast<entt::entity*>(pairHeader.actors[0]->userData) };
		entt::entity entityB = { *static_cast<entt::entity*>(pairHeader.actors[1]->userData) };

		auto processEntity = [&](entt::entity self, entt::entity other)
			{
				if (auto tag = registry->try_get<Tag>(self))
				{
					auto range = _handlers.equal_range(tag->id);
					for (auto it = range.first; it != range.second; ++it)
					{
						(it->second->*callback)(self, other, *registry);
					}
				}
			};

		processEntity(entityA, entityB);
	}
}

void core::CollisionCallback::registerCollisionHandler(const OnRegisterCollisionHandler& event)
{
	// 특정 태그의 충돌을 관리하는 핸들러 등록 (중첩 가능)
	_handlers.insert({ event.id, event.handler });
}

void core::CollisionCallback::removeCollisionHandler(const OnRemoveCollisionHandler& event)
{
	// 핸들러 등록 취소
	const auto range = _handlers.equal_range(event.id);

	for (auto it = range.first; it != range.second; )
	{
		if (it->second == event.handler)
		{
			it = _handlers.erase(it);
		}
		else
		{
			++it;
		}
	}
}
#pragma endregion


#pragma region DefaultCctHitReport
core::DefaultCctHitReport::DefaultCctHitReport(const CharacterController& controller)
	: _controller(&controller)
{
}

void core::DefaultCctHitReport::onShapeHit(const shapeHit& hit)
{
	if (auto* dynamicActor = hit.actor->is<physx::PxRigidDynamic>())
	{
		if (dynamicActor->getActorFlags() & physx::PxActorFlag::eDISABLE_SIMULATION)
			return;

		if (dynamicActor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC)
			return;

		if (*static_cast<entt::entity*>(dynamicActor->userData) == entt::null)
			return;

		// 충돌 방향과 충돌 지점 위치를 가져옵니다.
		auto hitDirection = hit.worldNormal;
		hitDirection.x = -hitDirection.x; // X축 방향을 반전시킵니다.
		hitDirection.z = -hitDirection.z; // Z축 방향을 반전시킵니다.
		hitDirection.y = 0.f;
		hitDirection.normalizeFast();

		// 최종 힘 벡터 계산: 충돌 방향에 위쪽 힘을 추가
		physx::PxVec3 force = hitDirection * _controller->mass;

		// 충돌 지점 위치를 PxVec3로 변환
		physx::PxVec3 hitPosition = physx::PxVec3(
			static_cast<float>(hit.worldPos.x),
			static_cast<float>(hit.worldPos.y),
			static_cast<float>(hit.worldPos.z)
		);

		// PxRigidDynamic에 충돌 지점에서 힘을 적용합니다.
		physx::PxRigidBodyExt::addForceAtPos(*dynamicActor, force, hitPosition, physx::PxForceMode::eIMPULSE);
	}
}

void core::DefaultCctHitReport::onControllerHit(const controllerHit& hit)
{
}

void core::DefaultCctHitReport::onObstacleHit(const obstacleHit& hit)
{
}

#pragma endregion
