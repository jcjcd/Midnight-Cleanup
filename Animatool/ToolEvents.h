#pragma once

namespace core
{
	class Scene;
}

namespace tool
{
	class AnimatorPanel;
	class Panel;
}

namespace tool
{
	enum class PanelType
	{
		None,
		Animator,
		Inspector,
		Project,
		Scene,
		Hierarchy,
		MenuBar,
		Console,
	};

	struct OnToolPlayScene {};

	struct OnToolStopScene {};

	struct OnToolPauseScene {};

	struct OnToolResumeScene {};

	struct OnToolModifyScene {};

	struct OnToolSaveTempScene {};

	struct OnToolLoadFromOtherEngine {};

	struct OnToolLoadScene
	{
		std::string path;
	};

	struct OnToolLoadPrefab
	{
		std::string path;
	};

	struct OnToolLoadFbx
	{
		std::string path;
	};

	struct OnToolCreateEntity
	{
		entt::entity entity = entt::null;
	};

	struct OnToolDestroyEntity
	{
		entt::entity entity = entt::null;
	};

	struct OnToolSelectEntity
	{
		std::vector<entt::entity> entities;
		core::Scene* scene = nullptr;
	};

	struct OnToolDoubleClickEntity
	{
		entt::entity entity = entt::null;
		core::Scene* scene = nullptr;
	};

	struct OnToolRemoveComponent
	{
		entt::meta_type meta;
		entt::registry* registry = nullptr;
		entt::entity entity = entt::null;
	};

	struct OnToolRemoveSystem
	{
		entt::meta_func func;
	};

	struct OnToolSelectFile
	{
		std::string path;
	};

	struct OnToolRequestAddPanel
	{
		PanelType panelType = PanelType::None;
		std::string path;
	};

	struct OnToolAddedPanel
	{
		PanelType panelType = PanelType::None;
		Panel* panel = nullptr;
	};

	struct OnToolRemovePanel
	{
		PanelType panelType = PanelType::None;
	};

	struct OnToolPostLoadScene
	{
		core::Scene* scene = nullptr;
	};

	struct OnToolPostLoadPrefab
	{
		std::string path;
		core::Scene* scene = nullptr;
	};

	struct OnToolModifyRenderer
	{
		entt::entity entity = entt::null;
	};

	struct OnToolApplyCamera {};

	struct OnToolShowFPS
	{
		bool show = true;
	};

	struct OnProcessSnapEntities
	{

	};
}
