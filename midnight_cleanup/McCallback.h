#pragma once

#include <Animacore/Scene.h>

namespace mc
{
	inline void OnPlaySound(core::Scene* scene, std::string soundName, int time)
	{
		int a = 0;
	}

	inline void OnTest(core::Scene* scene, std::string str, int i, entt::entity e)
	{
		int a = 0;
		std::string output = str + std::to_string(i) + std::to_string((uint32_t)e);
		OutputDebugStringA(output.c_str());
		OutputDebugStringA("\n");
	}

	inline void OnTestSceneChange(core::Scene* scene, std::string str, int i)
	{
		std::filesystem::path path = "./Resources/Scenes/" + str + ".scene";
		scene->GetDispatcher()->enqueue<core::OnChangeScene>(path);
	}

	inline void OnQuitGame(core::Scene* scene)
	{
		PostQuitMessage(0);
	}

	inline void OnChangeResolution(core::Scene* scene, bool isFullScreen, bool isWindowedFullScreen, int width, int height)
	{
		scene->GetDispatcher()->trigger<core::OnChangeResolution>({isFullScreen, isWindowedFullScreen, width, height});
	}

	void OnHitFurniture(core::Scene* scene);

	void OnWipeTools(core::Scene* scene);

	void OnPlayCurrentEntitySound(core::Scene* scene, entt::entity entity);

	void OnChangeTrigger(core::Scene* scene, std::string triggerName, bool isTrigger);

	void OnApplySetting(core::Scene* scene);

	void OnDiscardSetting(core::Scene* scene);

	void OnCloseSetting(core::Scene* scene);

	void OnRespawn(core::Scene* scene, entt::entity respawnPoint, entt::entity player, entt::entity playerCamera);

	void OnChangeBGMState(core::Scene* scene, bool isPlay);
}
