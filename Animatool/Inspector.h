#pragma once
#include "Panel.h"
#include "ComponentDataPanel.h"

class Renderer;

namespace tool
{
	class MaterialPanel;
	class AnimatorPanel;
	class AnimatorDataPanel;
	class ComponentDataPanel;
	class PhysicMaterialPanel;

	class Inspector : public Panel
	{
	public:
		explicit Inspector(entt::dispatcher& dispatcher, Renderer& renderer);

		void RenderPanel(float deltaTime) override;
		PanelType GetType() override { return PanelType::Inspector; }

	private:
		void renderEntityMode(float deltaTime);
		void renderControllerMode(float deltaTime);
		void renderMaterialMode(float deltaTime);
		void renderPhysicMaterialMode(float deltaTime);

		// 이벤트
		void selectEntity(const OnToolSelectEntity& event);
		void selectFile(const OnToolSelectFile& event);

		enum class PanelMode
		{
			Entity,
			Controller,
			Material,
			PhysicMaterial,
			Prefab, // todo 필요하다면
		};

		std::vector<entt::entity> _selectedEntities;
		std::filesystem::path _selectedFile;
		entt::meta_any _copiedComponent;

		Renderer* _renderer = nullptr;
		PanelMode _currentMode = PanelMode::Entity;

		std::shared_ptr<MaterialPanel> _materialPanel;
		std::shared_ptr<AnimatorDataPanel> _animatorDataPanel;
		std::shared_ptr<ComponentDataPanel> _componentDataPanel;
		std::shared_ptr<PhysicMaterialPanel> _physicMaterialPanel;
	};
}
