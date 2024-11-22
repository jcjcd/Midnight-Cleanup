#pragma once
#include "Panel.h"

namespace tool
{
	class AnimatorPanel;

	class AnimatorDataPanel : public Panel
	{
	public:
		AnimatorDataPanel(entt::dispatcher& dispatcher);

		void RenderPanel(float deltaTime) override;
		PanelType GetType() override { return PanelType::Inspector; }

	private:
		void addedPanel(const OnToolAddedPanel& event);
		void removePanel(const OnToolRemovePanel& event);

		AnimatorPanel* _animatorPanel = nullptr;
	};
}
