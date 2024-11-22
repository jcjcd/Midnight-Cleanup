#include "pch.h"
#include "FurnitureDiscardingSystem.h"

#include "McComponents.h"
#include "McTagsAndLayers.h"

#include <Animacore/Scene.h>
#include <Animacore/CoreComponents.h>
#include <Animacore/RenderComponents.h>
#include <Animacore/PhysicsScene.h>
#include <Animacore/RaycastHit.h>
#include <Animacore/CoreSystemEvents.h>

#include "../Animavision/Renderer.h"
#include "McEvents.h"

mc::FurnitureDiscardingSystem::FurnitureDiscardingSystem(core::Scene& scene)
	: ISystem(scene), _generator(scene.GetGenerator()), _dispatcher(scene.GetDispatcher())
{
	_dispatcher->sink<core::OnStartSystem>().connect<&FurnitureDiscardingSystem::startSystem>(this);
	_dispatcher->sink<mc::OnProcessFurniture>().connect<&FurnitureDiscardingSystem::processFurniture>(this);
}

mc::FurnitureDiscardingSystem::~FurnitureDiscardingSystem()
{
	_dispatcher->disconnect(this);
}

void mc::FurnitureDiscardingSystem::operator()(core::Scene& scene, Renderer& renderer, float tick)
{
	if (!scene.IsPlaying())
		return;

	auto registry = scene.GetRegistry();

	// ī�޶� ��鸲 �ִϸ��̼� ����
	if (_playerCamera != entt::null && _isHit)
	{
		core::LocalTransform& cameraLocal = registry->get<core::LocalTransform>(_playerCamera);

		if (_isOrigin)
		{
			_localCamPos = cameraLocal.position;
			_localCamRot = cameraLocal.rotation;
			_localCamScale = cameraLocal.scale;
			_isOrigin = false;
		}

		applyCameraShake(scene, cameraLocal, tick, 0.1f, .08f);
	}

	processRissolveEffect(scene, tick);

	if (_curParticle != entt::null)
	{
		_particleLifeTime += tick;
		if (_particleLifeTime > 1.0f)
		{
			scene.DestroyEntity(_curParticle);
			_curParticle = entt::null;
		}
	}
}

void mc::FurnitureDiscardingSystem::startSystem(const core::OnStartSystem& event)
{
	// ���� �� ��ŷ
	auto registry = event.scene->GetRegistry();

	if (auto&& storage = registry->storage<mc::RayCastingInfo>(); !storage.empty())
		_rayCastingInfo = &(*storage.begin());

	for (auto&& [entity, durability] : registry->view<mc::Durability>().each())
		durability.currentDurability = durability.maxDurability;

	for (auto&& [entity, camera] : registry->view<core::Camera>().each())
	{
		if (camera.isActive)
		{
			_playerCamera = entity;
			break;
		}
	}

	if (event.scene->GetRegistry()->ctx().contains<mc::Quest>())
		_quest = &event.scene->GetRegistry()->ctx().get<mc::Quest>();
	else
		_quest = &event.scene->GetRegistry()->ctx().emplace<mc::Quest>();

	// Player matching
	for (auto&& [entity, controller] : registry->view<core::CharacterController>().each())
	{
		if (registry->all_of<mc::Picking, mc::Inventory, mc::RayCastingInfo>(entity))
		{
			_player = entity;
			break;
		}
	}
	if (_player == entt::null)
		LOG_ERROR(*event.scene, "FurnitureDiscardingSystem needs essential components(Player)");
}

void mc::FurnitureDiscardingSystem::finishSystem(const core::OnFinishSystem& event)
{
	_rayCastingInfo = nullptr;
	_quest = nullptr;
	_axe = entt::null;
	_playerCamera = entt::null;
}

void mc::FurnitureDiscardingSystem::applyCameraShake(core::Scene& scene, core::LocalTransform& cameraLocal, float tick, float amplitude, float duration)
{
	if (_timeElapsed > duration)
	{
		_timeElapsed = 0.0f;
		cameraLocal.position = _localCamPos;
		cameraLocal.rotation = _localCamRot;
		cameraLocal.scale = _localCamScale;
		scene.GetRegistry()->patch<core::LocalTransform>(_playerCamera);
		_isHit = false;
		_isOrigin = true;
		return;
	}

	_timeElapsed += tick;
	amplitude *= (1.0f - _timeElapsed / duration);

	cameraLocal.position.x = _localCamPos.x + std::uniform_real_distribution<float>(-.1f, .1f)(*_generator);
	cameraLocal.position.y = _localCamPos.y + std::uniform_real_distribution<float>(-.1f, .1f)(*_generator);
	scene.GetRegistry()->patch<core::LocalTransform>(_playerCamera);
}

void mc::FurnitureDiscardingSystem::processRissolveEffect(core::Scene& scene, float tick)
{
	auto&& registry = scene.GetRegistry();
	auto&& view = registry->view<core::WorldTransform, mc::DissolveRenderer>();

	for (auto&& [entity, transform, dissolveRenderer] : view.each())
	{
		if (!dissolveRenderer.isPlaying)
			continue;

		dissolveRenderer.dissolveFactor += tick * dissolveRenderer.dissolveSpeed;

		if (dissolveRenderer.dissolveFactor >= 1.0f)
		{
			dissolveRenderer.dissolveFactor = 1.0f;
			dissolveRenderer.isPlaying = false;

			// ��ƼƼ ����
			scene.DestroyEntity(entity);

			auto* room = registry->try_get<mc::Room>(entity);

			auto woodenBoardNum = std::uniform_int_distribution(0, 9)(*_generator);
			woodenBoardNum <= 6 ? woodenBoardNum = 1 : woodenBoardNum = 2;

			for (int i = 0; i < woodenBoardNum; ++i)
			{
				// ������ ������ ����
				int prefabIndex = std::uniform_int_distribution(1, 4)(*_generator);
				std::string prefabPath = "./Resources/Prefabs/Trash_WoodenBoard_"
					+ std::to_string(prefabIndex) + ".prefab";
				auto newEntity = scene.LoadPrefab(prefabPath);

				// ������ Ʈ������ �Ҵ�
				auto& world = registry->replace<core::WorldTransform>
					(newEntity, registry->get<core::WorldTransform>(entity));

				world.position.x += std::uniform_real_distribution(-.01f, .01f)(*_generator);
				world.position.y += std::uniform_real_distribution(-.01f, .01f)(*_generator);
				world.position.z += std::uniform_real_distribution(-.01f, .01f)(*_generator);
				world.position.y += 0.5f;

				float yaw = std::uniform_real_distribution(-3.f, 3.f)(*_generator);
				float pitch = std::uniform_real_distribution(-3.f, 3.0f)(*_generator);
				float roll = std::uniform_real_distribution(-3.0f, 3.0f)(*_generator);

				world.rotation *= Quaternion::CreateFromYawPitchRoll(yaw, pitch, roll);

				// ���� ��ġ ����(���� �÷��̾� ��ġ)
				if (room)
				{
					registry->emplace_or_replace<mc::Room>(newEntity, *room);
					registry->patch<core::WorldTransform>(newEntity);
				}
				else
				{
					LOG_ERROR(scene, "{} : Furniture needs mc::Room component", newEntity);
				}

				_quest->totalTrashes += 1;
			}

			if (room)
				_quest->cleanedFurnitureInRoom[room->type] += 1;
		}
	}

}

void mc::FurnitureDiscardingSystem::processFurniture(const mc::OnProcessFurniture& event)
{
	auto registry = event.scene->GetRegistry();

	// ���� ������ ��� �ִ��� Ȯ��
	bool isAxe = false;
	entt::entity axe = entt::null;
	for (auto&& [entity, inventory] : registry->view<mc::Inventory>().each())
	{
		if (inventory.toolSlot.tools[inventory.currentTool] != entt::null)
		{
			mc::Tool* tool = registry->try_get<mc::Tool>(inventory.toolSlot.tools[inventory.currentTool]);
			if (tool && *tool == mc::Tool::Type::AXE)
			{
				isAxe = true;
				axe = inventory.toolSlot.tools[inventory.currentTool];
				break;
			}
		}
	}

	if (!isAxe)
		return;

	// ����ĳ��Ʈ Ȯ�� �� �������� ��ȣ�ۿ� ó��
	if (_rayCastingInfo)
	{
		const auto& hitInfo = _rayCastingInfo->hits;
		for (const auto& hit : hitInfo)
		{
			core::Layer* targetLayer = registry->try_get<core::Layer>(hit.entity);
			if (targetLayer && (targetLayer->mask & layer::Furniture::mask))
			{
				core::UICommon* ui = registry->try_get<core::UICommon>(hit.entity);
				core::Relationship* relationship = registry->try_get<core::Relationship>(hit.entity);

				if (auto* durability = registry->try_get<mc::Durability>(hit.entity))
				{
					auto& sound = registry->get<core::Sound>(axe);
					sound.isPlaying = true;
					registry->patch<core::Sound>(axe);

					if (durability->currentDurability > 0)
					{
						durability->currentDurability--;
						_isHit = true;


						// ���� ������ ����
						if (durability->currentDurability == 0)
						{
							_curParticle = event.scene->LoadPrefab("./Resources/Prefabs/woodParticle.prefab");
							auto& particleWorld = registry->replace<core::WorldTransform>(_curParticle, registry->get<core::WorldTransform>(hit.entity));
							auto& furnitureWorld = registry->get<core::WorldTransform>(hit.entity);
							auto& particleSystem = registry->get<core::ParticleSystem>(_curParticle);
							uint32_t random = std::uniform_int_distribution<uint32_t>(1, 9)(*_generator);
							std::string textureString = "T_Wood_N_0" + std::to_string(random) + ".dds";
							particleSystem.renderData.baseColorTextureString = textureString;
							particleWorld.position = hit.point;
							particleWorld.rotation = furnitureWorld.rotation;
							_particleLifeTime = 0.0f;

							registry->patch<core::WorldTransform>(_curParticle);

							// ���⼭ ���� ������ ó��
							if (auto* dissolve = registry->try_get<mc::DissolveRenderer>(hit.entity))
							{
								dissolve->isPlaying = true;
							}
							//event.scene->DestroyEntity(hit.entity);
							continue;
						}

						if (relationship)
						{
							for (auto& child : relationship->children)
							{
								if (auto* particleSystem = registry->try_get<core::ParticleSystem>(child))
								{
									auto& particleWorld = registry->get<core::WorldTransform>(child);
									particleWorld.position = hit.point;

									uint32_t random = std::uniform_int_distribution<uint32_t>(1, 9)(*_generator);
									std::string textureString = "T_Wood_N_0" + std::to_string(random) + ".dds";
									particleSystem->renderData.baseColorTextureString = textureString;
									particleSystem->isOn = true;
									particleSystem->instanceData.isReset = true;

									registry->patch<core::WorldTransform>(child);
									_dispatcher->enqueue<core::OnParticleTransformUpdate>(child);
								}
							}
						}
					}
				}
			}
		}
	}


}
