#pragma once
#include <physx/PxSimulationEventCallback.h>

namespace core
{
	class Scene;
	class Entity;
	class ICollisionHandler;
	struct CharacterController;
	struct OnRemoveCollisionHandler;
	struct OnRegisterCollisionHandler;

	class CollisionCallback : public physx::PxSimulationEventCallback
	{
	public:
		CollisionCallback(Scene& scene);
		~CollisionCallback() override = default;

		void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
		void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
		void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;
		void onWake(physx::PxActor** actors, physx::PxU32 count) override;
		void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
		void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override;

	private:
		void processCollisionEvent(const physx::PxContactPair& cp,
		                           const physx::PxContactPairHeader& pairHeader,
		                           physx::PxPairFlag::Enum eventFlag,
		                           void (ICollisionHandler::*callback)(entt::entity, entt::entity, entt::registry&));

		void registerCollisionHandler(const OnRegisterCollisionHandler& event);
		void removeCollisionHandler(const OnRemoveCollisionHandler& event);

		Scene* _scene = nullptr;
		std::multimap<entt::id_type, ICollisionHandler*> _handlers;
	};

	class DefaultCctHitReport : public physx::PxUserControllerHitReport
	{
		using shapeHit = physx::PxControllerShapeHit;
		using controllerHit = physx::PxControllersHit;
		using obstacleHit = physx::PxControllerObstacleHit;

	public:
		DefaultCctHitReport(const CharacterController& controller);

		void onShapeHit(const shapeHit& hit) override;
		void onControllerHit(const controllerHit& hit) override;
		void onObstacleHit(const obstacleHit& hit) override;

	private:
		const CharacterController* _controller = nullptr;
	};
}

