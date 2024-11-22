#include "pch.h"
#include "PickingSystem.h"

#include <Animacore/Scene.h>
#include <Animacore/RaycastHit.h>
#include <Animacore/PhysicsScene.h>
#include <Animacore/CoreComponents.h>
#include <Animacore/CoreSystemEvents.h>
#include <Animacore/CoreComponentInlines.h>

#include "McComponents.h"
#include "McEvents.h"
#include "McTagsAndLayers.h"
#include "Animacore/RenderComponents.h"

mc::PickingSystem::PickingSystem(core::Scene& scene)
	: ISystem(scene)
	, _physicsScene(scene.GetPhysicsScene())
	, _dispatcher(scene.GetDispatcher())
{
	_dispatcher->sink<core::OnStartSystem>().connect<&PickingSystem::startSystem>(this);
	_dispatcher->sink<core::OnFinishSystem>().connect<&PickingSystem::finishSystem>(this);
	_dispatcher->sink<mc::OnNotifyToolChanged>().connect<&PickingSystem::onNotifyToolChanged>(this);
}

mc::PickingSystem::~PickingSystem()
{
	_dispatcher->disconnect(this);
}

void mc::PickingSystem::operator()(core::Scene& scene, float tick)
{
	if (!scene.IsPlaying())
		return;

	pickingUpdate(scene, tick);
	snapUpdate(*scene.GetRegistry());
}

void mc::PickingSystem::operator()(core::Scene& scene)
{
	if (!scene.IsPlaying())
		return;

	auto registry = scene.GetRegistry();
	auto view = registry->view<Picking, core::WorldTransform>();

	for (auto&& [entity, picking, transform] : view.each())
	{
		if (picking.handSpot != entt::null && picking.pickingObject != entt::null)
		{
			auto& handWorld = registry->get<core::WorldTransform>(picking.handSpot);
			auto& objWorld = registry->get<core::WorldTransform>(picking.pickingObject);

			auto delta = handWorld.position - objWorld.position;
			float distance = delta.Length();

			// 최소 거리보다 멀면 속도를 적용
			if (distance > 0.01f)
			{
				delta.Normalize();
				constexpr float maxSpeed = 30.0f;
				float speed = (std::min)(maxSpeed, distance * 10.0f);
				auto velocity = delta * speed;
				_physicsScene->SetLinearVelocity(picking.pickingObject, velocity);
			}
			else
			{
				_physicsScene->SetLinearVelocity(picking.pickingObject, Vector3::Zero);
			}
		}
	}
}

void mc::PickingSystem::fillBucket(core::Scene& scene, entt::entity bucketEntity)
{
	auto registry = scene.GetRegistry();

	if (auto relationShip = registry->try_get<core::Relationship>(bucketEntity))
	{
		auto& children = relationShip->children;
		for (auto& child : children)
		{
			if (auto meshRenderer = registry->try_get<core::MeshRenderer>(child))
			{
				// isOn을 true로 바꾼다.
				meshRenderer->isOn = true;
				*meshRenderer->materialStrings.begin() = _waterMaterialName;
				meshRenderer->materials.clear();
			}
			//else if (auto* particleSystem = registry->try_get<core::ParticleSystem>(child))
			//{
			//	particleSystem->isOn = true;
			//	//particleSystem->instanceData.isReset = true;
			//	registry->patch<core::ParticleSystem>(child);
			//	_dispatcher->enqueue<core::OnParticleTransformUpdate>(child);
			//}			
		}
	}
	if (auto waterBucket = registry->try_get<mc::WaterBucket>(bucketEntity))
	{
		waterBucket->isFilled = true;
		auto& sound = registry->get<core::Sound>(bucketEntity);
		sound.isPlaying = true;
		sound.path = "Resources/Sound/tool/Bucket_Filled.wav";
		registry->patch<core::Sound>(bucketEntity);
	}
	if (auto durability = registry->try_get<mc::Durability>(bucketEntity))
	{
		durability->currentDurability = durability->maxDurability;
	}

	auto& log = scene.GetRegistry()->ctx().get<CaptionLog>();

	if (log.isShown[CaptionLog::Caption_MopGetWater] && !log.isShown[CaptionLog::Caption_MopMakeClean])
	{
		log.isShown[CaptionLog::Caption_MopMakeClean] = true;
		_dispatcher->enqueue<OnShowCaption>(scene, CaptionLog::Caption_MopMakeClean, 2.0f);
	}
}

void mc::PickingSystem::pickingUpdate(core::Scene& scene, float tick)
{
	using constraints = core::Rigidbody::Constraints;

	auto& registry = *scene.GetRegistry();
	const auto& input = registry.ctx().get<core::Input>();

	if (_player == entt::null)
		return;

	if (_isWorking)
		_timeElapsed += tick;

	std::vector<std::pair<entt::entity, core::Layer*>> hitObjects;

	for (const auto& hit : _ray->hits)
	{
		if (auto* layer = registry.try_get<core::Layer>(hit.entity))
			hitObjects.emplace_back(hit.entity, layer);
	}

	// 첫 번째 물건 OR 뭘 들고 있을 경우 그 뒤의 물건
	entt::entity firstHit = entt::null;

	for (const auto& hit : _ray->hits)
	{
		firstHit = hit.entity;
		if (_picking->pickingObject == entt::null)
		{
			if (auto* layer = registry.try_get<core::Layer>(firstHit))
			{
				if (layer->mask & layer::Hologram::mask)
				{
					continue;
				}
			}

		}
		if (_picking->pickingObject != firstHit)
			break;
	}

	if (firstHit != entt::null)
	{
		if (auto* layer = registry.try_get<core::Layer>(firstHit))
		{
			if (layer->mask & layer::Outline::mask)
			{
				auto& attributes = registry.get<core::RenderAttributes>(firstHit);
				attributes.flags |= core::RenderAttributes::Flag::OutLine;
			}
		}
	}


	// 잡기 or 정리하기
	if (input.keyStates[VK_LBUTTON] == core::Input::State::Down && firstHit != entt::null)
	{
		const auto* layer = registry.try_get<core::Layer>(firstHit);

		if (!layer or layer->mask & layer::Wall::mask)
		{
			for (auto&& [entity, layer] : hitObjects)
			{
				// 정리하기 
				if (layer->mask & layer::Hologram::mask)
				{
					// 스냅 포인트와의 거리 체크 후 물건 정리하기
					if (auto* snap = registry.try_get<SnapAvailable>(_picking->pickingObject))
					{
						if (snap->placedSpot == entity)
						{
							snap->isSnapped = true;

							auto* tag = registry.try_get<core::Tag>(_picking->pickingObject);
							if (tag && tag->id == tag::Mess::id)
							{
								auto room = registry.get<mc::Room>(_picking->pickingObject).type;
								registry.ctx().get<Quest>().cleanedMessInRoom[room] += 1;
							}

							// 홀로그램이 켜져 있으면 스냅 포인트에 붙이기
							const auto& snapSpot = registry.get<core::WorldTransform>(entity);
							auto& objWorld = registry.get<core::WorldTransform>(_picking->pickingObject);
							objWorld.position = snapSpot.position;
							objWorld.rotation = snapSpot.rotation;
							objWorld.scale = snapSpot.scale;
							registry.patch<core::WorldTransform>(_picking->pickingObject);


							auto& rigid = registry.get<core::Rigidbody>(_picking->pickingObject);
							rigid.constraints = constraints::FreezeAll;
							registry.patch<core::Rigidbody>(_picking->pickingObject);

							auto& renderer = registry.get<core::MeshRenderer>(snap->placedSpot);
							renderer.isOn = false;

							// 여기서 물 채우기 처리
							// 피킹 오브젝트에서 태그를 가져온다.
							if (auto tag = registry.try_get<core::Tag>(_picking->pickingObject))
							{
								if (tag->id == tag::WaterBucket::id)
								{
									fillBucket(scene, _picking->pickingObject);
								}
							}

							_picking->isPickingStart = false;
							_picking->pickingObject = entt::null;
						}
					}
				}
			}
		}
		// 문 닫기
		else if (layer->mask & layer::Door::mask)
		{
			scene.GetDispatcher()->enqueue<OnOpenDoor>(scene, firstHit);
		}

		// 전등 스위치
		else if (layer->mask & layer::Switch::mask)
		{
			scene.GetDispatcher()->enqueue<OnSwitchPushed>(scene, firstHit);
		}

		// Work Terminal
		else if (layer->mask & layer::WorkTerminal::mask)
		{
			auto& gameManager = registry.get<GameManager>(firstHit);

			if (gameManager.hasAttended && _timeElapsed > 5.0f)
				gameManager.isGameOver = true;
			else
			{
				gameManager.hasAttended = true;
				_isWorking = true;
				auto& log = registry.ctx().get<CaptionLog>();
				if (!log.isShown[CaptionLog::Caption_AfterClickTerminal])
				{
					_dispatcher->enqueue<OnShowCaption>(scene, CaptionLog::Caption_AfterClickTerminal, 3.0f);
					log.isShown[CaptionLog::Caption_AfterClickTerminal] = true;
				}

			}

			auto& sound = registry.get<core::Sound>(firstHit);
			sound.isPlaying = true;
			registry.patch<core::Sound>(firstHit);
		}

		// 장착한 장비가 없을 경우
		else if (_inventory->toolSlot.tools[_inventory->currentTool] == entt::null)
		{
			// 물체 집기
			if (_picking->isPickingStart)
			{
				// 정리하기 
				if (layer->mask & layer::Hologram::mask)
				{
					// 스냅 포인트와의 거리 체크 후 물건 정리하기
					if (auto* snap = registry.try_get<SnapAvailable>(_picking->pickingObject))
					{
						if (snap->placedSpot == firstHit)
						{
							snap->isSnapped = true;

							auto* tag = registry.try_get<core::Tag>(_picking->pickingObject);
							if (tag && tag->id == tag::Mess::id)
							{
								auto room = registry.get<mc::Room>(_picking->pickingObject).type;
								registry.ctx().get<Quest>().cleanedMessInRoom[room] += 1;
							}

							// 홀로그램이 켜져 있으면 스냅 포인트에 붙이기
							const auto& snapSpot = registry.get<core::WorldTransform>(firstHit);
							auto& objWorld = registry.get<core::WorldTransform>(_picking->pickingObject);
							objWorld.position = snapSpot.position;
							objWorld.rotation = snapSpot.rotation;
							objWorld.scale = snapSpot.scale;
							registry.patch<core::WorldTransform>(_picking->pickingObject);


							auto& rigid = registry.get<core::Rigidbody>(_picking->pickingObject);
							rigid.constraints = constraints::FreezeAll;
							registry.patch<core::Rigidbody>(_picking->pickingObject);

							auto& renderer = registry.get<core::MeshRenderer>(snap->placedSpot);
							renderer.isOn = false;

							// 여기서 물 채우기 처리
							// 피킹 오브젝트에서 태그를 가져온다.
							if (auto tag = registry.try_get<core::Tag>(_picking->pickingObject))
							{
								if (tag->id == tag::WaterBucket::id)
								{
									fillBucket(scene, _picking->pickingObject);
								}
							}

							_picking->isPickingStart = false;
							_picking->pickingObject = entt::null;
						}
					}
				}
			}
			else
			{
				// 잡기 
				if (layer->mask & layer::Pickable::mask)
				{
					_picking->pickingObject = firstHit;
					_picking->isPickingStart = true;

					auto& rigid = registry.get<core::Rigidbody>(_picking->pickingObject);
					rigid.useGravity = false;
					rigid.constraints = constraints::None;
					registry.patch<core::Rigidbody>(_picking->pickingObject);

					if (auto trash = registry.try_get<DestroyableTrash>(_picking->pickingObject))
						trash->isGrabbed = true;
					if (auto snap = registry.try_get<SnapAvailable>(_picking->pickingObject))
					{
						if (snap->isSnapped)
						{
							snap->isSnapped = false;

							auto* tag = registry.try_get<core::Tag>(firstHit);
							if (tag && tag->id == tag::Mess::id)
							{
								auto room = registry.get<mc::Room>(_picking->pickingObject).type;
								registry.ctx().get<Quest>().cleanedMessInRoom[room] -= 1;
							}
						}
					}
				}
			}
		}

	}
	// 놓기
	else if (input.keyStates[VK_RBUTTON] == core::Input::State::Down)
	{
		// 장착한 장비가 없을 경우
		if (_inventory->toolSlot.tools[_inventory->currentTool] == entt::null)
		{
			releaseItem(scene);
		}
		// 장비를 장착했을 경우
		else
		{
			// 장비 버리기
			scene.GetDispatcher()->enqueue<OnDiscardTool>(scene);
		}
	}

}

void mc::PickingSystem::snapUpdate(entt::registry& registry)
{
	const auto view = registry.view<SnapAvailable, core::WorldTransform>();

	for (auto&& [entity, snap, world] : view.each())
	{
		if (snap.placedSpot == entt::null)
			continue;

		const auto& spot = registry.get<core::WorldTransform>(snap.placedSpot);
		auto& renderer = registry.get<core::MeshRenderer>(snap.placedSpot);
		auto distance = world.position - spot.position;

		if (!snap.isSnapped && _picking->pickingObject == entity)
		{
			renderer.isOn = true;
		}
		else // 스냅 포인트에서 멀어진 경우 or 스냅 된 경우
		{
			renderer.isOn = false;
		}
	}
}

void mc::PickingSystem::startSystem(const core::OnStartSystem& event)
{
	auto registry = event.scene->GetRegistry();

	{
		auto view = registry->view<Picking, RayCastingInfo, Inventory>();

		for (auto&& [entity, picking, ray, inventory] : view.each())
		{
			_player = entity;
			_ray = &ray;
			_picking = &picking;
			_inventory = &inventory;
			break;
		}
	}

	{
		auto view = registry->view<SnapAvailable, core::Rigidbody>();

		for (auto&& [entity, snap, rigid] : view.each())
		{
			if (rigid.constraints == core::Rigidbody::Constraints::FreezeAll)
			{
				snap.isSnapped = true;
				auto& hologram = registry->get<core::Relationship>(entity).parent;
				auto& meshRenderer = registry->get<core::MeshRenderer>(hologram);
				meshRenderer.isOn = false;
			}
		}
	}
}

void mc::PickingSystem::finishSystem(const core::OnFinishSystem& event)
{
	_player = entt::null;
	_ray = nullptr;
	_picking = nullptr;
	_inventory = nullptr;
}

void mc::PickingSystem::onNotifyToolChanged(const OnNotifyToolChanged& event)
{
	releaseItem(*event.scene);
}

void mc::PickingSystem::releaseItem(core::Scene& scene)
{
	using constraints = core::Rigidbody::Constraints;

	auto& registry = *scene.GetRegistry();

	if (_picking->isPickingStart)
	{
		auto& rigid = registry.get<core::Rigidbody>(_picking->pickingObject);
		auto& attributes = registry.get<core::RenderAttributes>(_picking->pickingObject);

		if (auto trash = registry.try_get<DestroyableTrash>(_picking->pickingObject))
			trash->isGrabbed = false;

		rigid.useGravity = true;
		rigid.constraints = constraints::None;
		registry.patch<core::Rigidbody>(_picking->pickingObject);

		attributes.flags &= ~core::RenderAttributes::OutLine;

		_picking->pickingObject = entt::null;
		_picking->isPickingStart = false;
	}
}