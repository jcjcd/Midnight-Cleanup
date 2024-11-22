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
	// �ִϸ����� ��������
	auto&& registry = scene->GetRegistry();
	// view�� �ٰ�������
	auto view = registry->view<core::Animator>();
	// �ִϸ����� ��������
	for (auto&& [entity, animator] : view.each())
	{
		// �ִϸ������� Ʈ���� �̸� �ٲٱ�
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
	// ������ ����Ʈ ��������
	auto&& registry = scene->GetRegistry();

	// ������ ����Ʈ�� ��ġ ��������
	auto&& respawnTransform = registry->get<core::WorldTransform>(respawnPoint);

	// �÷��̾��� ��ġ ��������
	auto&& playerTransform = registry->get<core::WorldTransform>(player);

	// �÷��̾��� ��ġ�� ������ ����Ʈ�� �̵�
	playerTransform.position = respawnTransform.position;
	playerTransform.rotation = respawnTransform.rotation;

	// �÷��̾��� ��ġ�� ������Ʈ
	registry->patch<core::WorldTransform>(player);

	// �÷��̾� ī�޶��� ��ġ ��������
	auto&& playerCameraTransform = registry->get<core::WorldTransform>(playerCamera);
	// ī�޶��� ���� ��ġ ��������
	auto&& playerCameraLocalTransform = registry->get<core::LocalTransform>(playerCamera);


	// �÷��̾� ī�޶��� ȸ���� ������ ����Ʈ�� �̵�
	playerCameraTransform.position = respawnTransform.position + playerCameraLocalTransform.position;
	playerCameraTransform.rotation = respawnTransform.rotation;

	// �÷��̾� ī�޶��� ��ġ�� ������Ʈ
	registry->patch<core::WorldTransform>(playerCamera);

}

void mc::OnChangeBGMState(core::Scene* scene, bool isPlay)
{
	// BGM�� ����
	if (scene->GetRegistry()->ctx().contains<core::BGM>())
	{
		auto& bgm = scene->GetRegistry()->ctx().get<core::BGM>().sound;
		bgm.isPlaying = isPlay;
		bgm.isLoop = true;
		scene->GetDispatcher()->trigger<core::OnUpdateBGMState>({ *scene });
	}
}