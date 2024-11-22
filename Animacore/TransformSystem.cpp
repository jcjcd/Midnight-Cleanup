#include "pch.h"
#include "TransformSystem.h"

#include "Scene.h"
#include "CoreComponents.h"
#include "CorePhysicsComponents.h"
#include "CoreSystemEvents.h"

core::TransformSystem::TransformSystem(Scene& scene)
	: ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<OnCreateEntity>().connect<&TransformSystem::createEntity>(this);
	_dispatcher->sink<OnUpdateTransform>().connect<&TransformSystem::updateWorldByPhysics>(this);

	_registry = scene.GetRegistry();
	_registry->on_update<LocalTransform>().connect<&TransformSystem::updateLocal>(this);
	_registry->on_update<WorldTransform>().connect<&TransformSystem::updateWorld>(this);
}

core::TransformSystem::~TransformSystem()
{
	_dispatcher->disconnect(this);
	_registry->on_update<LocalTransform>().disconnect(this);
	_registry->on_update<WorldTransform>().disconnect(this);
}

void core::TransformSystem::operator()(Scene& scene, float tick)
{
	// Transform이 있는 모든 엔티티를 조회
	auto& registry = *scene.GetRegistry();
	auto group = registry.group<LocalTransform, WorldTransform>();

	// 최상위 엔티티의 matrix 업데이트
	group.each([this, &registry, &group](entt::entity entity, LocalTransform& local, WorldTransform& world)
		{
			// 부모가 없거나 최상위 엔티티인 경우에만 처리
			auto parent = registry.try_get<Relationship>(entity);

			if (!parent || parent->parent == entt::null || !registry.any_of<LocalTransform>(parent->parent))
			{
				std::queue<entt::entity> queue;
				queue.push(entity);

				while (!queue.empty())
				{
					entt::entity current = queue.front();
					queue.pop();

					// LocalTransform과 WorldTransform을 가진 현재 엔티티의 트랜스폼 업데이트
					updateTransform(registry, current);

					// 자식 엔티티들에 대해 처리
					if (registry.any_of<Relationship>(current))
					{
						const auto& relationship = registry.get<Relationship>(current);
						for (auto child : relationship.children)
						{
							if (group.contains(child))
								queue.push(child);
						}
					}
				}
			}
		});
}

void core::TransformSystem::createEntity(const OnCreateEntity& event)
{
	auto& registry = *event.scene->GetRegistry();

	for (auto entity : event.entities)
	{
		if (!registry.all_of<LocalTransform, WorldTransform>(entity))
			continue;

		// 트랜스폼 초기화
		updateTransform(registry, entity);

		// 최상위 엔티티의 matrix 업데이트
		std::queue<entt::entity> queue;
		queue.push(entity);

		while (!queue.empty())
		{
			entt::entity current = queue.front();
			queue.pop();

			if (!registry.all_of<LocalTransform, WorldTransform>(current))
				continue;

			updateTransform(registry, current);

			if (registry.any_of<Relationship>(current))
			{
				const auto& relationship = registry.get<Relationship>(current);
				for (auto child : relationship.children)
				{
					queue.push(child);
				}
			}
		}
	}
}

void core::TransformSystem::updateLocal(entt::registry& registry, entt::entity entity)
{
	updateTransform(registry, entity);
}

void core::TransformSystem::updateTransform(entt::registry& registry, entt::entity entity)
{
	auto& local = registry.get<LocalTransform>(entity);

	local.matrix = Matrix::CreateScale(local.scale) *
		Matrix::CreateFromQuaternion(local.rotation) *
		Matrix::CreateTranslation(local.position);

	auto& world = registry.get<WorldTransform>(entity);
	bool hasParent = false;

	if (auto relationship = registry.try_get<Relationship>(entity))
	{
		if (relationship->parent != entt::null)
		{
			if (auto parentWorld = registry.try_get<WorldTransform>(relationship->parent))
			{
				hasParent = true;
				world.matrix = local.matrix * parentWorld->matrix;
				world.scale = local.scale * parentWorld->scale;
				world.rotation = local.rotation * parentWorld->rotation;
				world.position = Vector3::Transform(local.position, parentWorld->matrix);
			}
		}
	}

	if (!hasParent)
	{
		world.matrix = local.matrix;
		world.scale = local.scale;
		world.rotation = local.rotation;
		world.position = local.position;
	}
}

void core::TransformSystem::updateWorldByPhysics(const OnUpdateTransform& event)
{
	updateWorld(*event.registry, event.entity); 
}

void core::TransformSystem::updateWorld(entt::registry& registry, entt::entity entity)
{
	// 월드 트랜스폼 업데이트
	auto& world = registry.get<WorldTransform>(entity);

	// 사용자가 설정한 worldMatrix를 반영 (월드 트랜스폼 갱신)
	world.matrix = Matrix::CreateScale(world.scale) *
		Matrix::CreateFromQuaternion(world.rotation) *
		Matrix::CreateTranslation(world.position);

	bool hasParent = false;

	// 부모가 있을 경우 로컬 트랜스폼을 역으로 계산
	if (auto relationship = registry.try_get<Relationship>(entity))
	{
		if (relationship->parent != entt::null)
		{
			// 부모의 월드 트랜스폼을 가져옴
			if (auto parentWorld = registry.try_get<WorldTransform>(relationship->parent))
			{
				hasParent = true;
				// 부모의 worldMatrix를 역으로 계산하여 자식의 localMatrix를 계산
				Matrix parentWorldInverse = parentWorld->matrix.Invert();
				Matrix localMatrix = world.matrix * parentWorldInverse;

				// LocalTransform에 로컬 트랜스폼을 적용
				auto& local = registry.get<LocalTransform>(entity);
				local.matrix = localMatrix;

				local.scale = world.scale / parentWorld->scale;
				Quaternion inverseRotation; parentWorld->rotation.Inverse(inverseRotation);
				local.rotation = world.rotation * inverseRotation;
				local.position = Vector3::Transform(world.position - parentWorld->position, inverseRotation);
			}
		}
	}

	if (!hasParent)
	{
		// 부모가 없는 경우, 월드 트랜스폼이 로컬 트랜스폼이 됨
		auto& local = registry.get<LocalTransform>(entity);
		local.matrix = world.matrix;
		local.scale = world.scale;
		local.rotation = world.rotation;
		local.position = world.position;
	}
}

