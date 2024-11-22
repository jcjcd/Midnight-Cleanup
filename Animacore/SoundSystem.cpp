#include "pch.h"
#include "SoundSystem.h"

#include "Scene.h"
#include "CoreComponents.h"
#include "CoreSystemEvents.h"
#include "CorePhysicsComponents.h"

#include <fmod/fmod_errors.h>

#ifdef _DEBUG
#pragma comment(lib, "fmodL_vc.lib")
#else
#pragma comment(lib, "fmod_vc.lib")
#endif


core::SoundSystem::SoundSystem(Scene& scene)
	: ISystem(scene)
{
	// FMOD 시스템 생성 및 초기화
	FMOD::System_Create(&_fmodSystem);

	_fmodSystem->init(512, FMOD_INIT_NORMAL, 0);

	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<OnStartSystem>().connect<&SoundSystem::startSystem>(this);
	_dispatcher->sink<OnFinishSystem>().connect<&SoundSystem::finishSystem>(this);
	_dispatcher->sink<OnSetVolume>().connect<&SoundSystem::setVolume>(this);
	_dispatcher->sink<OnUpdateBGMState>().connect<&SoundSystem::updateBGMState>(this);
}

core::SoundSystem::~SoundSystem()
{
	// 사운드 해제
	for (auto&& fmodSound : std::views::values(_soundMap))
	{
		fmodSound->release();
	}
	_soundMap.clear();

	// 시스템 해제
	if (_fmodSystem)
	{
		_fmodSystem->close();
		_fmodSystem->release();
		_fmodSystem = nullptr;
	}
	_dispatcher->disconnect(this);
}

void core::SoundSystem::operator()(Scene& scene, float tick)
{
	auto& registry = *scene.GetRegistry();
	auto soundView = registry.view<Sound>();
	auto listenerView = registry.view<SoundListener, WorldTransform>();

	for (auto&& [entity, sound] : soundView.each())
	{
		if (bool isPlaying = false; sound._channel->isPlaying(&isPlaying))
			sound.isPlaying = isPlaying;

		if (sound.is3D)
		{
			if (auto world = registry.try_get<WorldTransform>(entity))
			{
				FMOD_VECTOR pos = { world->position.x, world->position.y, world->position.z };
				sound._channel->set3DAttributes(&pos, nullptr);
			}
		}
	}

	for (auto&& [entity, listener, world] : listenerView.each())
	{
		const auto& dxPos = world.position;
		const auto& dxForward = world.matrix.Backward();
		const auto& dxUp = world.matrix.Up();

		FMOD_VECTOR position = { dxPos.x, dxPos.y, dxPos.z };
		FMOD_VECTOR forward = { dxForward.x, dxForward.y, dxForward.z };
		FMOD_VECTOR up = { dxUp.x, dxUp.y, dxUp.z };
		FMOD_VECTOR velocity = { 0.0f, 0.0f, 0.0f };

		if (listener.applyDoppler)
		{
			if (auto rigidbody = registry.try_get<Rigidbody>(entity))
			{
				velocity.x = rigidbody->velocity.x;
				velocity.y = rigidbody->velocity.y;
				velocity.z = rigidbody->velocity.z;
			}
		}

		// 사운드 위치 업데이트
		_fmodSystem->set3DListenerAttributes(listener._index, &position, &velocity, &forward, &up);
	}

	_fmodSystem->update();
}

FMOD::Sound* core::SoundSystem::loadSound(const Sound& sound, FMOD_RESULT& result)
{
	if (_soundMap.contains(sound.path))
		return _soundMap[sound.path];

	FMOD::Sound* newSound = nullptr;
	FMOD_MODE mode = FMOD_DEFAULT;
	mode |= sound.is3D ? FMOD_3D : FMOD_2D;
	mode |= sound.isLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
	result = _fmodSystem->createSound(sound.path.c_str(), mode, nullptr, &newSound);

	if (result != FMOD_OK)
		return nullptr;

	_soundMap[sound.path] = newSound;
	return newSound;
}

void core::SoundSystem::updateSound(entt::registry& registry, entt::entity entity)
{
	auto& sound = registry.get<Sound>(entity);

	// 사운드가 아직 로드되지 않은 경우 로드
	if (!_soundMap.contains(sound.path))
		if (FMOD_RESULT result; !loadSound(sound, result))
			return;

	// 사운드 재생 여부 및 사운드 교체
	bool isCurrentlyPlaying = false;
	if (sound._channel)
	{
		FMOD::Sound* currentSound = nullptr;
		sound._channel->getCurrentSound(&currentSound);
		if (currentSound != _soundMap[sound.path])
		{
			sound._channel->stop();
			sound._channel = nullptr;
		}

		sound._channel->isPlaying(&isCurrentlyPlaying);
	}

	// 재생 요청 처리
	if (sound.isPlaying)
	{
		// 사운드가 재생 중이 아니면 새롭게 재생
		if (!isCurrentlyPlaying)
		{
			FMOD_RESULT result = _fmodSystem->playSound(_soundMap[sound.path], nullptr, false, &sound._channel);
			if (result != FMOD_OK)
				return;

			// 3D 사운드일 경우 위치 설정
			if (sound.is3D && registry.all_of<WorldTransform>(entity))
			{
				auto& world = registry.get<WorldTransform>(entity);
				FMOD_VECTOR pos = { world.position.x, world.position.y, world.position.z };
				sound._channel->set3DAttributes(&pos, nullptr);
				sound._channel->set3DMinMaxDistance(sound.minDistance, sound.maxDistance);
			}

			// 개별 볼륨에 _masterVolume 곱해서 최종 볼륨 설정
			sound._channel->setVolume(sound.volume * _masterVolume);
			sound._channel->setPitch(sound.pitch);
		}
		else
		{
			// 이미 재생 중인 경우 볼륨만 업데이트
			FMOD_RESULT result = _fmodSystem->playSound(_soundMap[sound.path], nullptr, false, &sound._channel);
			if (result != FMOD_OK)
				return;

			sound._channel->setVolume(sound.volume * _masterVolume);
		}
	}
	else if (!sound.isPlaying && isCurrentlyPlaying)
	{
		// 사운드가 재생 중인데 isPlaying이 false인 경우 정지
		sound._channel->stop();
		sound._channel = nullptr; // 채널 초기화
	}

	// 속성 업데이트 (사운드, 볼륨, 피치)
	if (sound._channel && isCurrentlyPlaying)
	{
		// 볼륨 업데이트
		float currentVolume;
		sound._channel->getVolume(&currentVolume);
		float targetVolume = sound.volume * _masterVolume;
		if (std::fabsf(currentVolume - targetVolume) > DBL_EPSILON)
			sound._channel->setVolume(targetVolume);

		// 피치 업데이트
		float currentPitch;
		sound._channel->getPitch(&currentPitch);
		if (std::fabsf(currentPitch - sound.pitch) > DBL_EPSILON)
			sound._channel->setPitch(sound.pitch);

		// 감쇠 거리 업데이트
		if (sound.is3D)
		{
			float min, max;
			sound._channel->get3DMinMaxDistance(&min, &max);
			if (std::fabs(min - sound.minDistance) + std::fabs(max - sound.maxDistance) > DBL_EPSILON)
				sound._channel->set3DMinMaxDistance(sound.minDistance, sound.maxDistance);
		}

		// 모드 업데이트
		FMOD_MODE currentMode;
		sound._channel->getMode(&currentMode);
		FMOD_MODE newMode = (sound.is3D ? FMOD_3D : FMOD_2D) | (sound.isLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
		if (currentMode != newMode)
			sound._channel->setMode(newMode);
	}
}

void core::SoundSystem::updateListener(entt::registry& registry, entt::entity entity)
{
	auto&& [listener, world] = registry.get<SoundListener, WorldTransform>(entity);

	const auto& dxPos = world.position;
	const auto& dxForward = world.matrix.Backward();
	const auto& dxUp = world.matrix.Up();

	FMOD_VECTOR position = { dxPos.x, dxPos.y, dxPos.z };
	FMOD_VECTOR forward = { dxForward.x, dxForward.y, dxForward.z };
	FMOD_VECTOR up = { dxUp.x, dxUp.y, dxUp.z };
	FMOD_VECTOR velocity = { 0.0f, 0.0f, 0.0f };

	int numListeners;
	_fmodSystem->get3DNumListeners(&numListeners);
	if (listener._index < numListeners)
	{
		// 새로 생성하는 경우 인덱스 할당
		if (listener._index == -1)
			listener._index = numListeners - 1;

		auto result = _fmodSystem->set3DListenerAttributes(listener._index, &position, &velocity, &forward, &up);
		if (result != FMOD_OK)
			LOG_ERROR_D(*_dispatcher, "Entity {}, Update Listener Error: {}", entity, FMOD_ErrorString(result));
	}
}

void core::SoundSystem::destroyListener(entt::registry& registry, entt::entity entity)
{
	auto&& listener = registry.get<SoundListener>(entity);

	if (listener._index > 0)
	{
		auto result = _fmodSystem->set3DNumListeners(listener._index);
		if (result != FMOD_OK)
			LOG_ERROR_D(*_dispatcher, "Entity {}, Destroy Listener Error: {}", entity, FMOD_ErrorString(result));
	}
}

void core::SoundSystem::setVolume(const OnSetVolume& event)
{
	_masterVolume = event.volume;
}

void core::SoundSystem::startSystem(const OnStartSystem& event)
{
	auto& registry = *event.scene->GetRegistry();
	auto soundView = registry.view<Sound>();
	auto listenerView = registry.view<SoundListener>();

	// 리스너 생성
	for (auto&& [entity, listener] : listenerView.each())
	{
		updateListener(registry, entity);
	}

	// 사운드 로드
	for (auto&& [entity, sound] : soundView.each())
	{
		if (FMOD_RESULT result; !loadSound(sound, result))
			LOG_ERROR(*event.scene, "Loading Sound Error {} : {}, {}", entity, FMOD_ErrorString(result), sound.path);

		updateSound(registry, entity);

		if (sound.is3D && !registry.any_of<WorldTransform>(entity))
			LOG_ERROR(*event.scene, "{}: 3D Sound need a Transform Component", entity);
	}

	event.scene->GetRegistry()->on_update<Sound>().connect<&SoundSystem::updateSound>(this);
	event.scene->GetRegistry()->on_update<SoundListener>().connect<&SoundSystem::updateListener>(this);
	event.scene->GetRegistry()->on_destroy<SoundListener>().connect<&SoundSystem::destroyListener>(this);
}

void core::SoundSystem::finishSystem(const OnFinishSystem& event)
{
	auto view = event.scene->GetRegistry()->view<Sound>();

	// 재생중인 사운드 정지
	for (auto&& [entity, sound] : view.each())
	{
		if (sound._channel && sound.isLoop)
		{
			sound._channel->stop();
			sound._channel = nullptr;
		}
	}

	_soundMap.clear();
	event.scene->GetRegistry()->on_update<Sound>().disconnect(this);
	event.scene->GetRegistry()->on_update<SoundListener>().disconnect(this);
}

void core::SoundSystem::updateBGMState(const OnUpdateBGMState& event)
{
	// registry
	auto& registry = *event.scene->GetRegistry();
	// BGM ctx
	auto& bgm = registry.ctx().get<BGM>();
	auto& sound = bgm.sound;

	if (sound.is3D)
	{
		LOG_ERROR(*event.scene, "BGM Sound should not be 3D Sound");
		return;
	}

	// 사운드가 아직 로드되지 않은 경우 로드
	if (!_soundMap.contains(sound.path))
		if (FMOD_RESULT result; !loadSound(sound, result))
			return;

	// 사운드 재생 여부 및 사운드 교체
	bool isCurrentlyPlaying = false;
	if (sound._channel)
	{
		FMOD::Sound* currentSound = nullptr;
		sound._channel->getCurrentSound(&currentSound);
		if (currentSound != _soundMap[sound.path])
		{
			sound._channel->stop();
			sound._channel = nullptr;
		}

		sound._channel->isPlaying(&isCurrentlyPlaying);
	}

	// 재생 요청 처리
	if (sound.isPlaying)
	{
		// 사운드가 재생 중이 아니면 새롭게 재생
		if (!isCurrentlyPlaying)
		{
			FMOD_RESULT result = _fmodSystem->playSound(_soundMap[sound.path], nullptr, false, &sound._channel);
			if (result != FMOD_OK)
				return;

			sound._channel->setVolume(sound.volume);
			sound._channel->setPitch(sound.pitch);
		}
		// 사운드 다시 재생
		else
		{
			FMOD_RESULT result = _fmodSystem->playSound(_soundMap[sound.path], nullptr, false, &sound._channel);

			if (result != FMOD_OK)
				return;
		}
	}
	// 사운드 정지 처리
	else if (!sound.isPlaying && isCurrentlyPlaying)
	{
		// 사운드가 재생 중인데 isPlaying이 false인 경우 정지
		sound._channel->stop();
		sound._channel = nullptr; // 채널 초기화
	}

	// 속성 업데이트 (사운드, 볼륨, 피치)
	if (sound._channel && isCurrentlyPlaying)
	{
		// 볼륨 업데이트
		float currentVolume; sound._channel->getVolume(&currentVolume);
		if (std::fabsf(currentVolume - sound.volume) > DBL_EPSILON)
			sound._channel->setVolume(sound.volume);

		// 피치 업데이트
		float currentPitch; sound._channel->getPitch(&currentPitch);
		if (std::fabsf(currentPitch - sound.pitch) > DBL_EPSILON)
			sound._channel->setPitch(sound.pitch);

		// 감쇠 거리 업데이트
		if (sound.is3D)
		{
			float min, max; sound._channel->get3DMinMaxDistance(&min, &max);
			if (std::fabs(min - sound.minDistance) + std::fabs(max - sound.maxDistance) > DBL_EPSILON)
				sound._channel->set3DMinMaxDistance(sound.minDistance, sound.maxDistance);
		}

		// 모드 업데이트
		FMOD_MODE currentMode; sound._channel->getMode(&currentMode);
		FMOD_MODE newMode = (sound.is3D ? FMOD_3D : FMOD_2D) | (sound.isLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
		if (currentMode != newMode)
			sound._channel->setMode(newMode);
	}

}
