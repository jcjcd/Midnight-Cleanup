#pragma once
#include "Panel.h"
#include "../Animacore/CorePhysicsComponents.h"

namespace tool
{
	struct OnToolSelectFile;
}

namespace tool
{
	class PhysicMaterialPanel : public Panel
	{
	public:
		PhysicMaterialPanel(entt::dispatcher& dispatcher);

		void RenderPanel(float deltaTime) override;
		PanelType GetType() override { return PanelType::Inspector; }

	private:
		void selectFile(const OnToolSelectFile& event);

		core::ColliderCommon::PhysicMaterial _physicMaterial;
		std::filesystem::path _path;
	};

}
