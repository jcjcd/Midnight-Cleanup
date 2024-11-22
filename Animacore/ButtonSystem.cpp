#include "pch.h"
#include "ButtonSystem.h"

#include "Scene.h"
#include "CoreComponents.h"
#include "RenderComponents.h"
#include "CoreSystemEvents.h"

#include "MetaCtxs.h"

#include "../Animavision/Renderer.h"

core::ButtonSystem::ButtonSystem(core::Scene& scene) : ISystem(scene)
{
	scene.GetDispatcher()->sink<core::OnStartSystem>().connect<&ButtonSystem::startSystem>(this);
	//scene.GetDispatcher()->sink<core::OnGetTitleBarHeight>().connect<&ButtonSystem::getTitleBarHeight>(this);
	_dispatcher = scene.GetDispatcher();
}

//void core::ButtonSystem::getTitleBarHeight(const core::OnGetTitleBarHeight& event)
//{
//	//_titleBarHeight = event.titleBarHeight;
//}

core::ButtonSystem::~ButtonSystem()
{
	_dispatcher->disconnect(this);
}

void core::ButtonSystem::operator()(Scene& scene, float tick)
{
	auto&& registry = scene.GetRegistry();
	auto& input = scene.GetRegistry()->ctx().get<core::Input>();
	auto& config = scene.GetRegistry()->ctx().get<core::Configuration>();
	auto view = registry->view<core::Button, core::UI2D, core::UICommon>(entt::exclude_t<core::ComboBox>());
	auto meta = entt::resolve<global::CallBackFuncDummy>(global::callbackEventMetaCtx);

	float widthRatio = config.width / _defaultWidth;
	float heightRatio = config.height / _defaultHeight;


	Vector2 mousePos = input.mousePosition;

	mousePos.x -= config.width / 2;
	mousePos.y -= config.height / 2 - _titleBarHeight;
	mousePos.y *= -1;

	for (auto&& [entity, button, ui2d, uiCommon] : view.each())
	{
		if (!uiCommon.isOn)
			continue;

		DirectX::SimpleMath::Rectangle rect;
		rect.x = static_cast<long>((ui2d.position.x - ui2d.size.x / 2) * widthRatio);
		rect.y = static_cast<long>((ui2d.position.y - ui2d.size.y / 2) * heightRatio);
		rect.width = static_cast<long>(ui2d.size.x * widthRatio);
		rect.height = static_cast<long>(ui2d.size.y * heightRatio);

		if (rect.Contains(mousePos))
		{
			button.isHovered = true;
			if (input.keyStates[VK_LBUTTON] == core::Input::State::Down)
			{
				button.isPressed = true;
				if (button.pressedTexture)
				{
					// 텍스쳐 변경
					int a = 0;
				}
			}
			else if (input.keyStates[VK_LBUTTON] == core::Input::State::Up)
			{
				if (button.isPressed)
				{
					if (button.selectedTexture)
					{
						// 텍스쳐 변경
						int a = 0;
					}
					// 여기서 클릭 이벤트 발생, 전달
					for (auto&& onClick : button.onClickFunctions)
					{
						onClick.parameters[0] = &scene;
						meta.func(entt::hashed_string{ onClick.functionName.c_str() }).invoke({}, onClick.parameters.data(), onClick.parameters.size());
					}

					if (auto checkBox = registry->try_get<core::CheckBox>(entity))
					{
						checkBox->isChecked = !checkBox->isChecked;

						if (checkBox->isChecked)
							uiCommon.textureString = checkBox->checkedTextureString;
						else
							uiCommon.textureString = checkBox->uncheckedTextureString;
					}
				}
				button.isPressed = false;
			}
		}
		else
		{
			// 여기서도 텍스쳐 변경
			// 가능하면..
			if (input.keyStates[VK_LBUTTON] == core::Input::State::Up)
			{
				button.isPressed = false;
			}
			button.isHovered = false;
		}
	}

	for (auto&& [entity, button, text, comboBox] : registry->view<core::Button, core::Text, core::ComboBox>().each())
	{
		if (!text.isOn)
			continue;

		if (comboBox.isOn)
		{
			std::vector<DirectX::SimpleMath::Rectangle> rects;
			for (size_t i = 0; i < comboBox.comboBoxTexts.size(); i++)
			{
				DirectX::SimpleMath::Rectangle rect;
				rect.x = static_cast<long>((text.position.x - text.size.x / 2) * widthRatio);
				rect.y = static_cast<long>((-text.position.y - text.size.y / 2 - text.size.y * (i + 1)) * heightRatio);
				rect.width = static_cast<long>(text.size.x * widthRatio);
				rect.height = static_cast<long>(text.size.y * heightRatio);
				rects.push_back(rect);
			}

			for (size_t i = 0; i < rects.size(); i++)
			{
				if (rects[i].Contains(mousePos))
				{
					if (input.keyStates[VK_LBUTTON] == core::Input::State::Down)
						button.isPressed = true;
					else if (input.keyStates[VK_LBUTTON] == core::Input::State::Up)
					{
						if (button.isPressed)
						{
							comboBox.curIndex = static_cast<uint32_t>(i);
							comboBox.isOn = false;
							button.isPressed = false;
							break;
						}
					}
				}
			}
		}

		DirectX::SimpleMath::Rectangle rect;
		rect.x = static_cast<long>((text.position.x - text.size.x / 2) * widthRatio);
		rect.y = static_cast<long>((-text.position.y - text.size.y / 2) * heightRatio);
		rect.width = static_cast<long>(text.size.x * widthRatio);
		rect.height = static_cast<long>(text.size.y * heightRatio);

		if (rect.Contains(mousePos))
		{
			button.isHovered = true;
			if (input.keyStates[VK_LBUTTON] == core::Input::State::Down)
			{
				button.isPressed = true;
				if (button.pressedTexture)
				{
					// 텍스쳐 변경
					int a = 0;
				}
			}
			else if (input.keyStates[VK_LBUTTON] == core::Input::State::Up)
			{
				if (button.isPressed)
				{
					comboBox.isOn = !comboBox.isOn;

					if (button.selectedTexture)
					{
						// 텍스쳐 변경
						int a = 0;
					}
					// 여기서 클릭 이벤트 발생, 전달
					for (auto&& onClick : button.onClickFunctions)
					{
						onClick.parameters[0] = &scene;
						meta.func(entt::hashed_string{ onClick.functionName.c_str() }).invoke({}, onClick.parameters.data(), onClick.parameters.size());
					}
				}
				button.isPressed = false;
			}
		}
		else
		{
			// 여기서도 텍스쳐 변경
			// 가능하면..
			if (input.keyStates[VK_LBUTTON] == core::Input::State::Up)
			{
				button.isPressed = false;
			}
			button.isHovered = false;
		}
	}

	for (auto&& [entity, button, text] : registry->view<core::Button, core::Text>(entt::exclude_t<core::ComboBox>()).each())
	{
		if (!text.isOn)
			continue;

		DirectX::SimpleMath::Rectangle rect;
		rect.x = static_cast<long>((text.position.x - text.size.x / 2) * widthRatio);
		rect.y = static_cast<long>((-text.position.y - text.size.y / 2) * heightRatio);
		rect.width = static_cast<long>(text.size.x * widthRatio);
		rect.height = static_cast<long>(text.size.y * heightRatio);


		if (rect.Contains(mousePos))
		{
			button.isHovered = true;

			if (input.keyStates[VK_LBUTTON] == core::Input::State::Down)
			{
				button.isPressed = true;
				if (button.pressedTexture)
				{
					// 텍스쳐 변경
					int a = 0;
				}
			}
			else if (input.keyStates[VK_LBUTTON] == core::Input::State::Up)
			{
				if (button.isPressed)
				{
					if (button.selectedTexture)
					{
						// 텍스쳐 변경
						int a = 0;
					}
					// 여기서 클릭 이벤트 발생, 전달
					for (auto&& onClick : button.onClickFunctions)
					{
						onClick.parameters[0] = &scene;
						meta.func(entt::hashed_string{ onClick.functionName.c_str() }).invoke({}, onClick.parameters.data(), onClick.parameters.size());
					}
				}
				button.isPressed = false;
			}
		}
		else
		{
			// 여기서도 텍스쳐 변경
			// 가능하면..
			if (input.keyStates[VK_LBUTTON] == core::Input::State::Up)
			{
				button.isPressed = false;
			}
			button.isHovered = false;
		}
	}

	for (auto&& [entity, slider] : registry->view<core::Slider>().each())
	{
		if (!slider.isOn)
			continue;

		auto layout = registry->try_get<core::UI2D>(slider.sliderLayout);
		auto handleUI2D = registry->try_get<core::UI2D>(slider.sliderHandle);
		auto handleButton = registry->try_get<core::Button>(slider.sliderHandle);
		auto handleCommon = registry->try_get<core::UICommon>(slider.sliderHandle);

		if (!layout || !handleUI2D || !handleButton || !handleCommon->isOn)
			continue;

		float minMaxWeight = slider.minMax.y - slider.minMax.x;


		if (handleButton->isPressed)
		{
			handleCommon->color = DirectX::SimpleMath::Color(0.5f, 0.5f, 0.5f, 1.0f);

			if (slider.isVertical)
			{
				handleUI2D->position.y = mousePos.y;
				handleUI2D->position.y = std::clamp(handleUI2D->position.y, layout->position.y - layout->size.y / 2,
					layout->position.y + layout->size.y / 2);
				slider.curWeight = (handleUI2D->position.y - layout->position.y + layout->size.y / 2) / layout->size.y;
			}
			else
			{
				handleUI2D->position.x = mousePos.x / widthRatio;
				handleUI2D->position.x = std::clamp(handleUI2D->position.x, (layout->position.x - layout->size.x / 2),
					(layout->position.x + layout->size.x / 2));
				slider.curWeight = (handleUI2D->position.x - layout->position.x + layout->size.x / 2) / layout->size.x;
			}

			slider.curWeight *= minMaxWeight;
			slider.curWeight += slider.minMax.x;
		}
		else
		{
			handleCommon->color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);

			if (slider.isVertical)
				handleUI2D->position.y = layout->position.y - layout->size.y / 2 + layout->size.y * (slider.curWeight - slider.minMax.x) / minMaxWeight;
			else
				handleUI2D->position.x = layout->position.x - layout->size.x / 2 + layout->size.x * (slider.curWeight - slider.minMax.x) / minMaxWeight;
		}
	}
}

void core::ButtonSystem::startSystem(const core::OnStartSystem& event)
{
	auto registry = event.scene->GetRegistry();
	for (auto&& [entity, button] : registry->view<core::Button>().each())
	{
		if (!button.disabledTextureString.empty())
			button.disabledTexture = event.renderer->GetUITexture(button.disabledTextureString);

		if (!button.highlightTextureString.empty())
			button.highlightTexture = event.renderer->GetUITexture(button.highlightTextureString);

		if (!button.pressedTextureString.empty())
			button.pressedTexture = event.renderer->GetUITexture(button.pressedTextureString);

		if (!button.selectedTextureString.empty())
			button.selectedTexture = event.renderer->GetUITexture(button.selectedTextureString);

		if (core::UICommon* uiCommon = registry->try_get<core::UICommon>(entity))
			button.defaultTextureString = uiCommon->textureString;
	}
}
