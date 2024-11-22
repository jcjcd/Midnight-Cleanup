#include "pch.h"
#include "McCallback.h"
#include "McEvents.h"
#include "Animacore/CoreComponents.h"
#include "Animacore/RenderComponents.h"

void mc::OnHitFurniture(core::Scene* scene)
{
	scene->GetDispatcher()->trigger<mc::OnProcessFurniture>(mc::OnProcessFurniture{ *scene });
}

void mc::OnWipeTools(core::Scene* scene)
{
	scene->GetDispatcher()->trigger<mc::OnProcessWipe>(mc::OnProcessWipe{ *scene });
}

void mc::OnPlayCurrentEntitySound(core::Scene* scene, entt::entity entity)
{
	auto&& registry = scene->GetRegistry();
	if (auto sound = registry->try_get<core::Sound>(entity))
	{
		sound->isPlaying = true;
		registry->patch<core::Sound>(entity);
	}
}

void mc::OnChangeTrigger(core::Scene* scene, std::string triggerName, bool isTrigger)
{
	// 애니메이터 가져오기
	auto&& registry = scene->GetRegistry();
	// view로 다가져오기
	auto view = registry->view<core::Animator>();
	// 애니메이터 가져오기
	for (auto&& [entity, animator] : view.each())
	{
		// 애니메이터의 트리거 이름 바꾸기
		auto iter = animator.parameters.find(triggerName);
		if (iter != animator.parameters.end())
		{
			iter->second.value = isTrigger;
		}
	}

}

void mc::OnApplySetting(core::Scene* scene)
{
	scene->GetDispatcher()->trigger<mc::OnApplyChange>(mc::OnApplyChange{});
}

void mc::OnDiscardSetting(core::Scene* scene)
{
	scene->GetDispatcher()->trigger<mc::OnDiscardChange>(mc::OnDiscardChange{});
}

void mc::OnCloseSetting(core::Scene* scene)
{
	scene->GetDispatcher()->trigger<mc::OnCloseSettings>(mc::OnCloseSettings{*scene});
}

void mc::OnRespawn(core::Scene* scene, entt::entity respawnPoint, entt::entity player, entt::entity playerCamera)
{
	// 리스폰 포인트 가져오기
	auto&& registry = scene->GetRegistry();

	// 리스폰 포인트의 위치 가져오기
	auto&& respawnTransform = registry->get<core::WorldTransform>(respawnPoint);

	// 플레이어의 위치 가져오기
	auto&& playerTransform = registry->get<core::WorldTransform>(player);

	// 플레이어의 위치를 리스폰 포인트로 이동
	playerTransform.position = respawnTransform.position;
	playerTransform.rotation = respawnTransform.rotation;

	// 플레이어의 위치를 업데이트
	registry->patch<core::WorldTransform>(player);

	// 플레이어 카메라의 위치 가져오기
	auto&& playerCameraTransform = registry->get<core::WorldTransform>(playerCamera);
	// 카메라의 로컬 위치 가져오기
	auto&& playerCameraLocalTransform = registry->get<core::LocalTransform>(playerCamera);


	// 플레이어 카메라의 회전을 리스폰 포인트로 이동
	playerCameraTransform.position = respawnTransform.position + playerCameraLocalTransform.position;
	playerCameraTransform.rotation = respawnTransform.rotation;

	// 플레이어 카메라의 위치를 업데이트
	registry->patch<core::WorldTransform>(playerCamera);

}

void mc::OnChangeBGMState(core::Scene* scene, bool isPlay)
{
	// BGM을 종료
	if (scene->GetRegistry()->ctx().contains<core::BGM>())
	{
		auto& bgm = scene->GetRegistry()->ctx().get<core::BGM>().sound;
		bgm.isPlaying = isPlay;
		bgm.isLoop = true;
		scene->GetDispatcher()->trigger<core::OnUpdateBGMState>({ *scene });
	}
}