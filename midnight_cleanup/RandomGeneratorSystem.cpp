#include "pch.h"
#include "RandomGeneratorSystem.h"

#include "McComponents.h"
#include "McTagsAndLayers.h"
#include "Animacore/Scene.h"
#include "Animacore/CoreComponents.h"
#include "Animacore/CoreSystemEvents.h"
#include "Animacore/CoreTagsAndLayers.h"

#include <random>

#include "Animacore/PhysicsScene.h"

mc::RandomGeneratorSystem::RandomGeneratorSystem(core::Scene& scene)
	: ISystem(scene), _dispatcher(scene.GetDispatcher())
{
	_dispatcher->sink<core::OnPostStartSystem>()
		.connect<&RandomGeneratorSystem::postStartSystem>(this);
	_dispatcher->sink<core::OnFinishSystem>()
		.connect<&RandomGeneratorSystem::finishSystem>(this);
}

mc::RandomGeneratorSystem::~RandomGeneratorSystem()
{
	_dispatcher->disconnect(this);
}

void mc::RandomGeneratorSystem::postStartSystem(const core::OnPostStartSystem& event)
{
	auto* registry = event.scene->GetRegistry();
	auto* randomGen = event.scene->GetGenerator();
	auto& quest = registry->ctx().get<Quest>();

	std::uniform_int_distribution<int> trashDist(0, TrashGenerator::End - 1);

	for (const auto&& [entity, trashGen, world] : registry->view<mc::TrashGenerator, core::WorldTransform>().each())
	{
		_trashGenerators.push_back(entity);

		const Vector3& position = world.position;
		const Vector3 halfScale = world.scale * 0.5f;

		std::uniform_real_distribution posXDist(position.x - halfScale.x, position.x + halfScale.x);
		std::uniform_real_distribution posZDist(position.z - halfScale.z, position.z + halfScale.z);

		for (auto i = 0; i < trashGen.trashCount; ++i)
		{
			int trashIndex = trashDist(*randomGen);
			auto prefab = event.scene->LoadPrefab(TrashGenerator::trashPrefabs[trashIndex]);

			Vector3 randomPosition(
				posXDist(*randomGen),
				position.y,
				posZDist(*randomGen)
			);

			core::WorldTransform newTransform;
			newTransform.position = randomPosition;
			registry->replace<core::WorldTransform>(prefab, newTransform);
		}
	}

	constexpr uint32_t targetLayerMask = layer::Wall::mask;

	std::uniform_int_distribution<int> stainDist(0, MopStainGenerator::End - 1);

	for (auto&& [entity, mopStainGen, world, room] : registry->view<mc::MopStainGenerator, core::WorldTransform, mc::Room>().each())
	{
		//_trashGenerators.push_back(entity);

		const Vector3& position = world.position;
		const Vector3 halfScale = world.scale * 0.5f;

		std::uniform_real_distribution posXDist(position.x - halfScale.x, position.x + halfScale.x);
		std::uniform_real_distribution posZDist(position.z - halfScale.z, position.z + halfScale.z);
		std::uniform_real_distribution stainRotation = std::uniform_real_distribution(0.f, 360.f);
		std::uniform_real_distribution stainScale = std::uniform_real_distribution(1.f, 2.0f);

		// -y방향으로 싹 다 레이캐스팅
		for (auto i = 0; i < mopStainGen.stainCount; ++i)
		{
			Vector3 rayOrigin(
				posXDist(*randomGen),
				position.y,
				posZDist(*randomGen)
			);

			Vector3 rayDirection(0.f, -1.f, 0.f);

			std::vector<physics::RaycastHit> hits;

			event.scene->GetPhysicsScene()->Raycast(rayOrigin, rayDirection, hits, 100.f, targetLayerMask);

			if (hits.empty())
			{
				continue;
			}

			// 가장 가까운 레이캐스트 결과를 사용

			// 레이캐스트 결과를 정렬
			std::ranges::sort(hits, [](const physics::RaycastHit& a, const physics::RaycastHit& b)
			{
				return a.distance < b.distance;
			});

			physics::RaycastHit hit;

			for (const auto& h : hits)
			{
				if (auto* hitLayer = registry->try_get<core::Layer>(h.entity))
				{
					if (hitLayer->mask & layer::Wall::mask)
					{
						hit = h;
						break;
					}
				}
			}

			if (hit.entity == entt::null)
			{
				continue;
			}

			int stainIndex = stainDist(*randomGen);
			auto prefab = event.scene->LoadPrefab(MopStainGenerator::stainPrefabs[stainIndex]);

			auto& worldTransform = registry->get<core::WorldTransform>(prefab);
			worldTransform.position = hit.point;
			float rotY = stainRotation(*randomGen);
			Vector3 euler = Vector3(0.f, rotY / 180.f * DirectX::XM_PI, 0.f);
			worldTransform.rotation = Quaternion::CreateFromYawPitchRoll(euler);
			float scale = stainScale(*randomGen);
			Vector3 scaleVec = Vector3(scale, 1.f, scale);
			worldTransform.scale *= scaleVec;

			registry->patch<core::WorldTransform>(prefab);

			// 레이캐스트 결과를 이용해 룸 정보를 업데이트

			if (auto* prefabRoom = registry->try_get<mc::Room>(prefab))
			{
				prefabRoom->type = room.type;
			}
		}


	}
}

void mc::RandomGeneratorSystem::finishSystem(const core::OnFinishSystem& event)
{
}
