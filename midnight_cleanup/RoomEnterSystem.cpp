#include "pch.h"
#include "RoomEnterSystem.h"

#include <Animacore/Scene.h>

#include "McComponents.h"
#include "McEvents.h"
#include "McTagsAndLayers.h"
#include "Animacore/CoreComponents.h"
#include "Animacore/CorePhysicsComponents.h"
#include "Animacore/CoreTagsAndLayers.h"
#include "Animacore/PhysicsScene.h"
#include "Animacore/RenderComponents.h"
#include "Animacore/CoreComponentInlines.h"

mc::RoomEnterSystem::RoomEnterSystem(core::Scene& scene)
	: ISystem(scene)
	, _scene(&scene)
	, _dispatcher(scene.GetDispatcher())
{
	_dispatcher->sink<core::OnStartSystem>()
		.connect<&RoomEnterSystem::startSystem>(this);

	_dispatcher->sink<core::OnFinishSystem>()
		.connect<&RoomEnterSystem::finishSystem>(this);

	_dispatcher->sink<mc::OnOpenDoor>()
		.connect<&RoomEnterSystem::openDoor>(this);

	_dispatcher->sink<mc::OnTriggerGhostEvent>()
		.connect<&RoomEnterSystem::triggerGhostEvent>(this);
}

mc::RoomEnterSystem::~RoomEnterSystem()
{
	_dispatcher->disconnect(this);
}

void mc::RoomEnterSystem::operator()(core::Scene& scene)
{
	if (!scene.IsPlaying())
		return;

	auto registry = scene.GetRegistry();
	auto doorView = registry->view<Door, core::WorldTransform>();
	for (auto&& [entity, door, world] : doorView.each())
	{
		// 문이 열리는 중이 아닐경우
		if (!door.isMoving)
			continue;

		world.position += door.direction * FIXED_TIME_STEP * Door::SPEED_WEIGHT;

		auto displacement = door.destination - world.position;
		if (displacement.LengthSquared() < 0.001f)
		{
			switch (door.state)
			{
			case Door::OpenPositiveX:
				door.state = Door::ClosePositiveX;
				break;
			case Door::OpenPositiveZ:
				door.state = Door::ClosePositiveZ;
				break;
			case Door::OpenNegativeX:
				door.state = Door::CloseNegativeX;
				break;
			case Door::OpenNegativeZ:
				door.state = Door::CloseNegativeZ;
				break;
			case Door::ClosePositiveX:
				door.state = Door::OpenPositiveX;
				break;
			case Door::ClosePositiveZ:
				door.state = Door::OpenPositiveZ;
				break;
			case Door::CloseNegativeX:
				door.state = Door::OpenNegativeX;
				break;
			case Door::CloseNegativeZ:
				door.state = Door::OpenNegativeZ;
				break;
			default:
				break;
			}

			door.isMoving = false;
			world.position = door.destination;
		}

		registry->patch<core::WorldTransform>(entity);
	}

	auto& playerRoom = registry->get<mc::Room>(_player);

	if (!_isTriggered[playerRoom.type] && playerRoom.type != Room::Outdoor)
	{
		_enterElapsedTime[playerRoom.type] += FIXED_TIME_STEP;
		if (_enterElapsedTime[playerRoom.type] > 3.0f)
		{
			_dispatcher->enqueue<OnTriggerGhostEvent>(scene, playerRoom.type);
			_isTriggered[playerRoom.type] = true;
		}
	}


	constexpr float blinkInterval = 0.5f;
	static float blinkAccum = 0.f;

	blinkAccum += FIXED_TIME_STEP;

	if (blinkAccum > blinkInterval)
	{
		blinkAccum -= blinkInterval;

		for (auto&& [entity, swc, attr, room] : registry->view<mc::Switch, core::RenderAttributes, mc::Room>().each())
		{
			if (playerRoom.type == room.type && !swc.isOn)
			{
				attr.flags ^= core::RenderAttributes::Flag::OutLine;
			}
		}
	}
}

void mc::RoomEnterSystem::OnCollisionEnter(entt::entity self, entt::entity other, entt::registry& registry)
{
	if (other != _player)
		return;

	_enteredRoom = &registry.get<mc::Room>(self);
	_playerRoom = &registry.get<mc::Room>(other);
	auto& captionLog = registry.ctx().get<mc::CaptionLog>();

	//if (enteredRoom.type == Room::Outdoor && !captionLog.isShown[CaptionLog::Caption_NotTakeAnyTool])
	//{
	//	_dispatcher->enqueue<OnShowCaption>(*_scene, CaptionLog::Caption_NotTakeAnyTool, 15.0f);
	//	captionLog.isShown[CaptionLog::Caption_NotTakeAnyTool] = true;
	//}
	if (!captionLog.isShown[CaptionLog::Caption_HouseEnter])
	{
		if (_enteredRoom->type == Room::Porch or _enteredRoom->type == Room::LivingRoom or _enteredRoom->type == Room::GuestRoom)
		{
			_dispatcher->enqueue<OnShowCaption>(*_scene, CaptionLog::Caption_HouseEnter, 5.0f);
			captionLog.isShown[CaptionLog::Caption_HouseEnter] = true;
		}
	}
	else if (captionLog.isShown[CaptionLog::Caption_MopDirty] && !captionLog.isShown[CaptionLog::Caption_MopGetWater] && _enteredRoom->type == Room::Outdoor)
	{
		_dispatcher->enqueue<OnShowCaption>(*_scene, CaptionLog::Caption_MopGetWater, 3.0f);
		captionLog.isShown[CaptionLog::Caption_MopGetWater] = true;
	}


	if (other != _player)
		return;

	outlineMesses(*_scene, _playerRoom, _enteredRoom);


	_playerRoom->type = _enteredRoom->type;
}

void mc::RoomEnterSystem::OnCollisionStay(entt::entity self, entt::entity other, entt::registry& registry)
{
}

void mc::RoomEnterSystem::OnCollisionExit(entt::entity self, entt::entity other, entt::registry& registry)
{
}

void mc::RoomEnterSystem::startSystem(const core::OnStartSystem& event)
{
	auto registry = event.scene->GetRegistry();
	_dispatcher->trigger<core::OnRegisterCollisionHandler>({ entt::type_id<tag::RoomEnter>().hash(), this });

	auto h1 = entt::type_id<tag::RoomEnter>().hash();
	auto h2 = entt::type_id<tag::Tool>().hash();
	auto h3 = entt::type_id<tag::Mess>().hash();
	auto h4 = entt::type_id<tag::Trash>().hash();

	if (const auto it = registry->ctx().find<mc::Quest>())
		_quest = it;
	else
		_quest = &registry->ctx().emplace<mc::Quest>();

	for (auto&& [entity, door] : registry->view<Door>().each())
	{
		if (door.state == Door::None)
			LOG_ERROR(*event.scene, "{} : Set a mc::Door component Initial state", entity);
	}

	for (auto&& [entity, character] : registry->view<CharacterMovement>().each())
	{
		_player = entity;
	}

	for(auto&&[entity, tag] : registry->view<core::Tag>().each())
	{
		if(tag.id == tag::TriggerSound::id)
		{
			_triggerSound = entity;
			break;
		}
	}
}

void mc::RoomEnterSystem::finishSystem(const core::OnFinishSystem& event)
{
	_dispatcher->trigger<core::OnRemoveCollisionHandler>({ tag::RoomEnter::id, this });
}

void mc::RoomEnterSystem::openDoor(const mc::OnOpenDoor& event)
{
	auto registry = event.scene->GetRegistry();

	auto&& [door, sound, world] = registry->get<Door, core::Sound, core::WorldTransform>(event.entity);

	// 문 여는 중에 방향이 바뀌지 않도록
	if (!door.isMoving)
		door.isMoving = true;
	else
		return;

	sound.isPlaying = true;
	registry->patch<core::Sound>(event.entity);

	door.destination = world.position;

	// 문 열릴 방향(목표 위치) 설정
	switch (door.state)
	{
	case Door::OpenPositiveX:
	case Door::CloseNegativeX:
		door.destination.x += 1.5f;
		break;
	case Door::OpenPositiveZ:
	case Door::CloseNegativeZ:
		door.destination.z += 1.5f;
		break;
	case Door::OpenNegativeX:
	case Door::ClosePositiveX:
		door.destination.x -= 1.5f;
		break;
	case Door::OpenNegativeZ:
	case Door::ClosePositiveZ:
		door.destination.z -= 1.5f;
		break;
	default:
		break;
	}

	door.direction = door.destination - world.position;
}

void mc::RoomEnterSystem::triggerGhostEvent(const mc::OnTriggerGhostEvent& event)
{
	auto registry = event.scene->GetRegistry();
	auto physics = event.scene->GetPhysicsScene();
	auto gen = event.scene->GetGenerator();
	auto& sound = registry->get<core::Sound>(_triggerSound);
	std::string soundPath = "./Resources/Sound/ghost/Ghost_" + std::to_string(std::uniform_int_distribution(0,3)(*gen)) + ".wav";
	sound.path = soundPath;
	sound.isPlaying = true;
	registry->patch<core::Sound>(_triggerSound);

	for (auto&& [entity, rigid, room, snap] : registry->view<core::Rigidbody, Room, SnapAvailable>().each())
	{
		if (room.type != event.room)
			continue;

		if (snap.isSnapped)
			snap.isSnapped = false;

		rigid.constraints = core::Rigidbody::Constraints::None;
		rigid.useGravity = true;

		registry->ctx().get<Quest>().cleanedMessInRoom[room.type] -= 1;

		registry->patch<core::Rigidbody>(entity);

		Vector3 forceDir;
		forceDir.x = std::uniform_real_distribution(-8.f, 8.f)(*gen);
		forceDir.y = std::uniform_real_distribution(0.f, 8.f)(*gen);
		forceDir.z = std::uniform_real_distribution(-8.f, 8.f)(*gen);

		physics->AddForce(entity, forceDir, physics::ForceMode::Impulse);
	}

	const auto& playerPos = registry->get<core::WorldTransform>(_player).position;

	for (auto&& [entity, rigid, world, trash] : registry->view<core::Rigidbody, core::WorldTransform, DestroyableTrash>().each())
	{
		if ((playerPos - world.position).Length() > 3.0f)
			continue;

		Vector3 forceDir;
		forceDir.x = std::uniform_real_distribution(-8.f, 8.f)(*gen);
		forceDir.y = std::uniform_real_distribution(0.f, 8.f)(*gen);
		forceDir.z = std::uniform_real_distribution(-8.f, 8.f)(*gen);

		physics->AddForce(entity, forceDir, physics::ForceMode::Impulse);
	}

	outlineMesses(*event.scene, _playerRoom, _playerRoom);
}

void mc::RoomEnterSystem::outlineMesses(core::Scene& scene, Room* prevRoom, Room* nextRoom)
{
	auto& registry = *scene.GetRegistry();
	for (auto&& [entity, snap, room, attr] : registry.view<mc::SnapAvailable, mc::Room, core::RenderAttributes>().each())
	{
		if (room.type == prevRoom->type)
		{
			attr.flags &= ~core::RenderAttributes::Flag::OutLine;
		}

		if (room.type == nextRoom->type)
		{
			if (snap.isSnapped)
				continue;
			attr.flags |= core::RenderAttributes::Flag::OutLine;
		}
	}

}
