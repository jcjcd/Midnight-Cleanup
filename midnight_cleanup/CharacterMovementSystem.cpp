#include "pch.h"
#include "CharacterMovementSystem.h"

#include "McComponents.h"

#include <Animacore/Scene.h>
#include <Animacore/PhysicsScene.h>
#include <Animacore/CoreComponents.h>
#include <Animacore/CoreSystemEvents.h>
#include <Animacore/RenderComponents.h>
#include <Animacore/CorePhysicsComponents.h>
#include <Animacore/InputSystem.h>

#include "McTagsAndLayers.h"

#include "McEvents.h"

mc::CharacterMovementSystem::CharacterMovementSystem(core::Scene& scene)
	: ISystem(scene)
{
	_physicsScene = scene.GetPhysicsScene();
	_dispatcher = scene.GetDispatcher();

	_dispatcher->sink<OnInputLock>().connect<&CharacterMovementSystem::cameraLock>(this);
	_dispatcher->sink<core::OnStartSystem>().connect<&CharacterMovementSystem::startSystem>(this);
	_dispatcher->sink<core::OnFinishSystem>().connect<&CharacterMovementSystem::finishSystem>(this);
	_dispatcher->sink<OnOpenSettings>().connect<&CharacterMovementSystem::openSettings>(this);
	_dispatcher->sink<OnCloseSettings>().connect<&CharacterMovementSystem::closeSettings>(this);
}

mc::CharacterMovementSystem::~CharacterMovementSystem()
{
	_dispatcher->disconnect(this);
}

void mc::CharacterMovementSystem::PreUpdate(core::Scene& scene, float tick)
{
	//���콺 �����Ÿ��°Ŷ������� ȸ���� ���⼭ ����
	auto* registry = scene.GetRegistry();
	auto&& setting = registry->ctx().emplace<GameSetting>();
	auto&& input = registry->ctx().get<core::Input>();
	core::WorldTransform& cameraTransform = scene.GetRegistry()->get<core::WorldTransform>(_camera);

	core::Camera* camera = registry->try_get<core::Camera>(_camera);
	camera->fieldOfView = setting.fov * DirectX::XM_PI / 360.0f;

	if (_isCameraLocked == false)
	{
		// ���콺 �̵��� ���� ȸ�� ó��
		if (input.mouseDeltaRotation != Vector2::Zero)
		{
			if (_skipFrameOnce)
			{
				return;
			}

			float yaw = input.mouseDeltaRotation.x * setting.mouseSensitivity * 0.02;   // �¿� ȸ��
			float pitch = input.mouseDeltaRotation.y * setting.mouseSensitivity * 0.02; // ���� ȸ��

			// �¿� ȸ�� (Yaw)
			Quaternion yawRotation = Quaternion::CreateFromAxisAngle(Vector3::Up, yaw);

			// ���� ȸ���� ������ ��ġ(����) ������ ����
			Vector3 eulerAngles = cameraTransform.rotation.ToEuler();
			float currentPitch = eulerAngles.x;

			// ��ġ ���� ����
			constexpr float maxPitch = DirectX::XMConvertToRadians(89.9f); // 89�� ���� (�ٶ󺸴� ������ ������ ����������� ����)
			constexpr float minPitch = DirectX::XMConvertToRadians(-59.9f);

			// ���ο� ��ġ ������ ����ϰ� ���� ���� ���� Ŭ����
			float newPitch = currentPitch + pitch;
			newPitch = std::clamp(newPitch, minPitch, maxPitch);

			// ���ѵ� ��ġ�� ������� ���ο� ȸ���� ����
			Quaternion pitchRotation = Quaternion::CreateFromAxisAngle(Vector3::Right, newPitch - currentPitch);

			// ���ο� ȸ�� ���
			Quaternion temp = pitchRotation * cameraTransform.rotation;
			Quaternion temp2 = temp * yawRotation;

			// ���� ȸ���� ����
			cameraTransform.rotation = temp2;
			scene.GetRegistry()->patch<core::WorldTransform>(_camera);
		}
		else
		{
			if (_skipFrameOnce)
			{
				_skipFrameOnce = false;
			}
		}
	}
}

void mc::CharacterMovementSystem::operator()(core::Scene& scene, float tick)
{
	if (_player == entt::null or _camera == entt::null)
		return;

	auto registry = scene.GetRegistry();
	auto& input = registry->ctx().get<core::Input>();
	auto& movement = registry->get<CharacterMovement>(_player);
	auto& controller = registry->get<core::CharacterController>(_player);
	auto& cameraWorld = scene.GetRegistry()->get<core::WorldTransform>(_camera);


	// ���� �̵� �ʱ�ȭ
	movement.hDir = Vector3::Zero;

	if (!_isMovementLocked)
	{
		if (input.keyStates[KEY('D')] == core::Input::State::Hold)
		{
			movement.hDir.x += 1.f;
		}
		else if (input.keyStates[KEY('A')] == core::Input::State::Hold)
		{
			movement.hDir.x -= 1.f;
		}

		if (input.keyStates[KEY('W')] == core::Input::State::Hold)
		{
			movement.hDir.z += 1.f;
		}
		else if (input.keyStates[KEY('S')] == core::Input::State::Hold)
		{
			movement.hDir.z -= 1.f;
		}

		// ���� �Է� ó��
		if (input.keyStates[VK_SPACE] == core::Input::State::Down && controller.isGrounded)
		{
			movement.currentJumpVelocity = movement.jump; // ���� ���� �� �ʱ� ���ӵ� ����
			_animator->parameters["IsGround"].value = false;

			if (_jumpSound)
			{
				_jumpSound->isPlaying = true;
				registry->patch<core::Sound>(_player);
			}
		}
	}

	movement.hDir.Normalize();

	// �̵� ������ ĳ������ ���� ���� �������� ��ȯ
	if (movement.hDir != Vector3::Zero)
	{
		Vector3 forward = cameraWorld.matrix.Backward();
		Vector3 right = cameraWorld.matrix.Right();

		// ���� �������θ� �̵��ϵ��� y ���� 0���� ����
		forward.y = 0.0f;
		right.y = 0.0f;

		forward.Normalize();
		right.Normalize();

		movement.hDir = right * movement.hDir.x + forward * movement.hDir.z;
		movement.hDir.Normalize();

		_animator->parameters["IsRunning"].value = true;

		// ���鿡 ���������
		if (controller.isGrounded)
		{
			_playSoundTImeAccum += tick;
		}
	}
	else
	{
		_animator->parameters["IsRunning"].value = false;
	}

	if (_playSoundTImeAccum >= _playSoundThreshold)
	{
		_playSoundTImeAccum -= _playSoundThreshold;
		playFootStepSound(scene);
	}

	auto& gameManager = registry->get<mc::GameManager>(_gameManager);

	if(!gameManager.isGameOver)
	{
		// esc ������ ����â ���������
		if (input.keyStates[VK_ESCAPE] == core::Input::State::Down && !_isSettingsOpen)
		{
			_isSettingsOpen = true;
			_dispatcher->trigger<OnOpenSettings>(scene);
		}
		else if (input.keyStates[VK_ESCAPE] == core::Input::State::Down && _isSettingsOpen)
		{
			_isSettingsOpen = false;
			_dispatcher->trigger<OnCloseSettings>(scene);
		}
	}
}

void mc::CharacterMovementSystem::operator()(core::Scene& scene)
{
	auto registry = scene.GetRegistry();
	auto& movement = registry->get<CharacterMovement>(_player);
	auto& controller = registry->get<core::CharacterController>(_player);
	auto& cameraWorld = scene.GetRegistry()->get<core::WorldTransform>(_camera);


	// ���� ���� �� ���� �ӵ� ������Ʈ
	movement.currentJumpVelocity -= movement.dropAcceleration * FIXED_TIME_STEP; // �߷¿� ���� ���� �ӵ� ����
	movement.vDir.y += movement.currentJumpVelocity * FIXED_TIME_STEP;

	// �̵� ���� ���
	Vector3 displacement = movement.hDir * movement.speed * FIXED_TIME_STEP;

	// �߷� ����
	displacement.y += movement.vDir.y;

	constexpr uint32_t ignoreLayer = layer::Stain::mask;
	constexpr uint32_t targetLayer = UINT32_MAX - ignoreLayer;

	_physicsScene->MoveCharacter(_player, controller, displacement, targetLayer, FIXED_TIME_STEP);

	// ���鿡 ����ִ��� Ȯ���Ͽ� ���� �ӵ� �ʱ�ȭ
	if (controller.isGrounded)
	{
		movement.vDir.y = 0.f;
		_animator->parameters["IsGround"].value = true;
	}
}

void mc::CharacterMovementSystem::cameraLock(const mc::OnInputLock& event)
{
	_isCameraLocked = event.isCameraLock;
	_isMovementLocked = event.isMovementLock;
}

void mc::CharacterMovementSystem::startSystem(const core::OnStartSystem& event)
{
	auto&& registry = *event.scene->GetRegistry();

	auto cameraView = registry.view<core::Camera>();
	for (auto&& [entity, camera] : cameraView.each())
	{
		if (camera.isActive)
		{
			_camera = entity;
			break;
		}
	}

	auto characterView = registry.view<core::CharacterController, mc::CharacterMovement>();
	for (auto&& [entity, controller, movement] : characterView.each())
	{
		_player = entity;
		_jumpSound = registry.try_get<core::Sound>(_player);
	}

	auto animview = registry.view<core::WorldTransform, core::Animator>();

	for (auto&& [entity, world, anim] : animview.each())
	{
		_hand = entity;
		_animator = &anim;
	}

	if (_player == entt::null)
		LOG_ERROR(*event.scene, "CharacterMovementSystem : System needs CharacterController & CharacterMovement");
	if (_camera == entt::null)
		LOG_ERROR(*event.scene, "CharacterMovementSystem : System needs at least one activated Camera");


	auto soundMaterialView = registry.view<core::Sound, mc::SoundMaterial>();
	for (auto&& [entity, sound, sMaterial] : soundMaterialView.each())
	{
		switch (sMaterial.type)
		{
		case SoundMaterial::Type::GROUND:
			_walkGroundSound = &sound;
			_walkGroundSoundEntity = entity;
			break;
		case SoundMaterial::Type::WOODEN:
			_walkWoodenSound = &sound;
			_walkWoodenSoundEntity = entity;
			break;
		case SoundMaterial::Type::TRUCK:
			_walkTruckSound = &sound;
			_walkTruckSoundEntity = entity;
			break;
		case SoundMaterial::Type::TATAMI:
			_walkTatamiSound = &sound;
			_walkTatamiSoundEntity = entity;
			break;
		default:
			break;
		}
	}

	for (auto&& [entity, gameManager] : registry.view<mc::GameManager>().each())
	{
		_gameManager = entity;
		break;
	}
}

void mc::CharacterMovementSystem::finishSystem(const core::OnFinishSystem& event)
{
	_camera = entt::null;
	_player = entt::null;
	_gameManager = entt::null;
}

void mc::CharacterMovementSystem::openSettings(const OnOpenSettings& event)
{
	auto& registry = *event.scene->GetRegistry();

	for (auto&& [entity, tag] : registry.view<core::Tag>().each())
	{
		if (tag.id == tag::AimUI::id)
		{
			registry.get<core::UICommon>(entity).isOn = false;
			continue;
		}

		if (tag.id == tag::TooltipUI::id)
		{
			registry.get<core::UICommon>(entity).isOn = false;
			continue;
		}

		if (tag.id == tag::TooltipAlphaUI::id)
		{
			registry.get<core::UICommon>(entity).isOn = false;
			continue;
		}

		if (tag.id == tag::SettingUI::id)
		{
			auto* common = registry.try_get<core::UICommon>(entity);
			auto* text = registry.try_get<core::Text>(entity);

			if (common && text)
			{
				common->isOn = true;
				text->isOn = true;
			}
			else
			{
				if (common)
					common->isOn = true;
				else if (text)
					text->isOn = true;
			}
		}
	}

	for (auto&& [entity, common, radialUI] : registry.view<core::UICommon, mc::RadialUI>().each())
	{
		if (common.isOn)
			common.isOn = false;
	}

	while (::ShowCursor(true) < 0) {}
	global::mouseMode = global::MouseMode::None;
	_dispatcher->trigger<mc::OnInputLock>(OnInputLock{ true, true });
	_dispatcher->trigger<core::OnSetTimeScale>(core::OnSetTimeScale{ 0.0f });
}

void mc::CharacterMovementSystem::closeSettings(const OnCloseSettings& event)
{
	auto& registry = *event.scene->GetRegistry();

	for (auto&& [entity, tag] : registry.view<core::Tag>().each())
	{
		if (tag.id == tag::AimUI::id)
		{
			registry.get<core::UICommon>(entity).isOn = true;
			continue;
		}

		if (tag.id == tag::TooltipUI::id)
		{
			registry.get<core::UICommon>(entity).isOn = true;
			continue;
		}

		if (tag.id == tag::TooltipAlphaUI::id)
		{
			registry.get<core::UICommon>(entity).isOn = true;
			continue;
		}

		if (tag.id == tag::SettingUI::id)
		{
			auto* common = registry.try_get<core::UICommon>(entity);
			auto* text = registry.try_get<core::Text>(entity);

			if (common && text)
			{
				common->isOn = false;
				text->isOn = false;
			}
			else
			{
				if (common)
					common->isOn = false;
				else if (text)
					text->isOn = false;
			}
		}
	}

	while (::ShowCursor(false) > 0) {}
	global::mouseMode = global::MouseMode::Lock;
	_dispatcher->trigger<mc::OnInputLock>(OnInputLock{ false, false });
	_dispatcher->trigger<core::OnSetTimeScale>(core::OnSetTimeScale{ 1.0f });
}

void mc::CharacterMovementSystem::playFootStepSound(core::Scene& scene)
{
	// ������ ���̸� ���.
	auto& registry = *scene.GetRegistry();
	auto& controller = registry.get<core::CharacterController>(_player);
	auto& transform = registry.get<core::WorldTransform>(_player);

	Vector3 origin = transform.position;

	Vector3 direction = Vector3::Down;

	std::vector<physics::RaycastHit> hits;

	if (!_physicsScene->Raycast(origin, direction, hits, 5, 3.f))
	{
		return;
	}

	// ����ĳ��Ʈ ����� ���� ���� ���
	// ����ĳ��Ʈ ����
	std::sort(hits.begin(), hits.end(), [](const physics::RaycastHit& a, const physics::RaycastHit& b)
		{
			return a.distance < b.distance;
		});

	// ������ ���鼭 ���� ����� soundMaterial�� ã�´�.
	for (auto& hit : hits)
	{
		if (auto* soundMaterial = registry.try_get<mc::SoundMaterial>(hit.entity))
		{
			switch (soundMaterial->type)
			{
			case SoundMaterial::Type::GROUND:
				_walkGroundSound->isPlaying = true;
				registry.patch<core::Sound>(_walkGroundSoundEntity);
				break;
			case SoundMaterial::Type::WOODEN:
				_walkWoodenSound->isPlaying = true;
				registry.patch<core::Sound>(_walkWoodenSoundEntity);
				break;
			case SoundMaterial::Type::TRUCK:
				_walkTruckSound->isPlaying = true;
				registry.patch<core::Sound>(_walkTruckSoundEntity);
				break;
			case SoundMaterial::Type::TATAMI:
				_walkTatamiSound->isPlaying = true;
				registry.patch<core::Sound>(_walkTatamiSoundEntity);
				break;
			default:
				_walkGroundSound->isPlaying = true;
				break;
			}
			break;
		}
	}

}

