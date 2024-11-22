#pragma once
#include "SystemTraits.h"
#include "CoreSystemEvents.h"
#include "SystemInterface.h"

namespace core
{
	class PhysicsScene;

	class PhysicsSystem : public ISystem, public IUpdateSystem
	{
	public:
		PhysicsSystem(Scene& scene);
		~PhysicsSystem() override { _dispatcher->disconnect(this); }

		void operator()(Scene& scene, float tick) override;

	private:
		void preStartSystem(const OnStartSystem& event);
		void finishSystem(const OnFinishSystem& event);

		void createEntity(const OnCreateEntity& event);
		void destroyEntity(const OnDestroyEntity& event);

		void updateLocal(entt::registry& registry, entt::entity entity);
		void updateWorld(entt::registry& registry, entt::entity entity);
		void updateRigidbody(entt::registry& registry, entt::entity entity);
		void updateColliderCommon(entt::registry& registry, entt::entity entity);

		entt::dispatcher* _dispatcher = nullptr;
		std::shared_ptr<PhysicsScene> _physicsScene;
	};
}
DEFINE_SYSTEM_TRAITS(core::PhysicsSystem)