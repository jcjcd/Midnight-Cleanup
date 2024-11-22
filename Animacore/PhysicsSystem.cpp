#include "pch.h"
#include "PhysicsSystem.h"

#include "Scene.h"
#include "PhysicsScene.h"
#include "CoreComponents.h"
#include "CorePhysicsComponents.h"

core::PhysicsSystem::PhysicsSystem(Scene& scene)
	: ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();

	_dispatcher->sink<OnStartSystem>().connect<&PhysicsSystem::preStartSystem>(this);
	_dispatcher->sink<OnFinishSystem>().connect<&PhysicsSystem::finishSystem>(this);

	_dispatcher->sink<OnCreateEntity>().connect<&PhysicsSystem::createEntity>(this);
	_dispatcher->sink<OnDestroyEntity>().connect<&PhysicsSystem::destroyEntity>(this);

}

void core::PhysicsSystem::preStartSystem(const OnStartSystem& event)
{
	auto registry = event.scene->GetRegistry();
	_physicsScene = event.scene->GetPhysicsScene();

	// 씬 시작시 액터 생성
	for (auto&& [entity, collider] : registry->view<ColliderCommon>().each())
	{
		if (_physicsScene->LoadMaterial(collider.materialName))
		{
			_physicsScene->CreatePhysicsActor(entity, *registry);
		}
		else
		{
			LOG_ERROR(*event.scene, "{} : Invalid Physic Material", entity);
		}
	}

	registry->on_update<LocalTransform>().connect<&PhysicsSystem::updateLocal>(this);
	registry->on_update<WorldTransform>().connect<&PhysicsSystem::updateWorld>(this);
	registry->on_update<Rigidbody>().connect<&PhysicsSystem::updateRigidbody>(this);
	registry->on_update<ColliderCommon>().connect<&PhysicsSystem::updateColliderCommon>(this);
}

void core::PhysicsSystem::finishSystem(const core::OnFinishSystem& event)
{
	auto registry = event.scene->GetRegistry();

	// 씬 종료시 액터 삭제
	for (auto&& [entity, collider] : registry->view<ColliderCommon>().each())
	{
		_physicsScene->DestroyPhysicsActor(entity, *registry);
	}

	_physicsScene->Clear();
	_physicsScene = nullptr;

	registry->on_update<LocalTransform>().disconnect(this);
	registry->on_update<WorldTransform>().disconnect(this);
	registry->on_update<Rigidbody>().disconnect(this);
	registry->on_update<ColliderCommon>().disconnect(this);
}

void core::PhysicsSystem::createEntity(const core::OnCreateEntity& event)
{
	// 런타임 중 액터 생성
	for (auto entity : event.entities)
		_physicsScene->CreatePhysicsActor(entity, *event.scene->GetRegistry());
}

void core::PhysicsSystem::destroyEntity(const core::OnDestroyEntity& event)
{
	// 런타임 중 액터 삭제
	for (auto entity : event.entities)
		_physicsScene->DestroyPhysicsActor(entity, *event.scene->GetRegistry());
}

void core::PhysicsSystem::updateLocal(entt::registry& registry, entt::entity entity)
{
	_physicsScene->UpdateTransform(entity, registry.get<WorldTransform>(entity));
}

void core::PhysicsSystem::updateWorld(entt::registry& registry, entt::entity entity)
{
	_physicsScene->UpdateTransform(entity, registry.get<WorldTransform>(entity));
}

void core::PhysicsSystem::updateRigidbody(entt::registry& registry, entt::entity entity)
{
	_physicsScene->UpdateRigidbody(entity, registry.get<Rigidbody>(entity), true);
}

void core::PhysicsSystem::updateColliderCommon(entt::registry& registry, entt::entity entity)
{
	_physicsScene->UpdateCollider(entity, registry.get<ColliderCommon>(entity));
}

void core::PhysicsSystem::operator()(Scene& scene, float tick)
{
	if(!scene.IsPlaying())
		return;

	_physicsScene->Update(tick);
}
