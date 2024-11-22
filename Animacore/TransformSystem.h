#pragma once
#include "CoreComponents.h"
#include "SystemInterface.h"
#include "SystemTraits.h"

namespace core
{
	struct OnStartSystem;
	struct OnCreateEntity;
	class Scene;

	class TransformSystem : public ISystem, public IUpdateSystem
	{
	public:
		TransformSystem(Scene& scene);
		~TransformSystem();

		void operator()(Scene& scene, float tick) override;

#ifdef _EDITOR
		void Update(Scene& scene, float tick) { (*this)(scene, tick); }
#endif

	private:
		void createEntity(const OnCreateEntity& event);
		void updateWorldByPhysics(const OnUpdateTransform& event);

		void updateTransform(entt::registry& registry, entt::entity entity);
		void updateLocal(entt::registry& registry, entt::entity entity);
		void updateWorld(entt::registry& registry, entt::entity entity);

		entt::dispatcher* _dispatcher = nullptr;
		entt::registry* _registry = nullptr;
	};
}
DEFINE_SYSTEM_TRAITS(core::TransformSystem)
