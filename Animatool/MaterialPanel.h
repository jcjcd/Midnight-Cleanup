#pragma once
#include "Panel.h"

class Renderer;

namespace tool
{
	class MaterialPanel : public Panel
	{
	public:
		MaterialPanel(entt::dispatcher& dispatcher, Renderer* renderer);

		void RenderPanel(float deltaTime) override;
		PanelType GetType() override { return PanelType::Inspector; }

	private:
		void changeMaterial(const OnToolSelectFile& event);

	private:
		std::filesystem::path _selectedFile;
		Renderer* _renderer = nullptr;

		std::vector<std::string> _textureExtensions = { ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".dds", ".hdr"};
	};
};
