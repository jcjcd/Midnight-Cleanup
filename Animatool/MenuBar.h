#pragma once
#include "Panel.h"

class Renderer;

namespace tool
{
	class ToolProcess;

	class MenuBar : public Panel
	{
	public:
		explicit MenuBar(entt::dispatcher& dispatcher, Renderer* renderer, ToolProcess& toolProcess);

		void RenderPanel(float deltaTime) override;
		PanelType GetType() override { return PanelType::MenuBar; }

	private:
		void renderMenu();
		void checkSelected();
		void renderSavePopup();
		void renderSystemManager();
		void renderConfiguration();
		void processShortcut();

		void saveScene();
		void performNextAction();

		void modifyScene(const OnToolModifyScene&) { _needSave = true; }
		void loadScene(const OnToolLoadScene& event);
		void removeSystem(const OnToolRemoveSystem& event);

		enum class MenuType
		{
			None,
			NewScene,
			OpenScene,
			SaveScene,
			SaveAs,
			LoadNewFromFile,
			LoadContinuousFromFile,
		};

		Renderer* _renderer;
		ToolProcess* _toolProcess;

		ImGui::FileBrowser _fileDialog;
		std::filesystem::path _currentScenePath;
		MenuType _currentMenu = MenuType::None;
		MenuType _nextAction = MenuType::None;
		bool _showSavePopup = false;
		bool _needSave = false;

		bool _showSystemManager = false;
		int _systemToRemove = -1;
		std::vector<std::string> _availableSystems;

		bool _showConfig = false;
	};
}
