#include "pch.h"
#include "InventorySystem.h"

#include "McComponents.h"
#include "McTagsAndLayers.h"

#include <Animacore/Scene.h>
#include <Animacore/CoreComponents.h>
#include <Animacore/RenderComponents.h>
#include <Animacore/CoreSystemEvents.h>

#include "../Animavision/Renderer.h"
#include <Animacore/InputSystem.h>

#include "McEvents.h"


mc::InventorySystem::InventorySystem(core::Scene& scene)
	: ISystem(scene)
{
	scene.GetDispatcher()->sink<core::OnStartSystem>().connect<&InventorySystem::startSystem>(this);
	scene.GetDispatcher()->sink<core::OnFinishSystem>().connect<&InventorySystem::finishSystem>(this);
	_dispatcher = scene.GetDispatcher();
}

mc::InventorySystem::~InventorySystem()
{
	_dispatcher->disconnect(this);
}

void mc::InventorySystem::operator()(core::Scene& scene, Renderer& renderer, float tick)
{
	if(!scene.IsPlaying())
		return;

	auto& input = scene.GetRegistry()->ctx().get<core::Input>();
	auto view = scene.GetRegistry()->view<core::UICommon, core::UI2D, mc::RadialUI>();
	static RadialUI::Type direction = RadialUI::Type::DEFAULT;
	static uint32_t currentTool = 0;
	auto& config = scene.GetRegistry()->ctx().get<core::Configuration>();

	if (_mainInventory == nullptr)
		return;

	for (int i = 0; i < 4; i++)
	{
		if (_mainInventory->toolSlot.tools[i] != entt::null)
			_mainInventory->toolType[i] = scene.GetRegistry()->get<Tool>(_mainInventory->toolSlot.tools[i]).type;
		else
			_mainInventory->toolType[i] = Tool::Type::DEFAULT;

		if (input.keyStates['1' + i] == core::Input::State::Down || input.keyStates[VK_NUMPAD1 + i] == core::Input::State::Down)
		{
			_mainInventory->currentTool = i;
			_isCurrentToolChanged = true;
		}
	}

	// ���콺 �ٷ� 1, 2, 3, 4�� ���� �� �־�� �Ѵ�.
	if (input.mouseWheel)
	{
		input.mouseWheel > 0 ? _mainInventory->currentTool++ : _mainInventory->currentTool--;
		_mainInventory->currentTool = _mainInventory->currentTool % 4;
		_isCurrentToolChanged = true;
	}


	for (auto&& [entity, common, ui, radialUI] : view.each())
	{
		// �� Ű�� ������ ����� ui�� ��ü�� ������
		// �� ��Ȳ���� ���콺�� ȣ�� �Ǿ��� �� �ش��ϴ� �����ۿ� �ٸ� ������ UI�� ��������
		// �װ� Ŭ���ϸ� ������ �Ǿ� �Ѵ�. �׸��� ui�� ������.
		// �׷��� ui�� ��ġ�� ���콺 ��ǥ��� ��ġ�ϴ� ��Ȯ�� ������ �ʿ��ϴ�.


		if (common.isOn)
		{
			global::mouseMode = global::MouseMode::None;

			if (radialUI.type & RadialUI::Type::INVENTORY)
			{
				if (radialUI.type & RadialUI::Type::LEFT)
				{
					if (_mainInventory->toolType[3] == Tool::DEFAULT)
						common.color.w = 0.0f;
					else
					{
						common.color.w = 1.0f;

						if (_mainInventory->toolType[3] == Tool::Type::AXE)
							common.textureString = "T_RadialIcon_Axe.dds";
						else if (_mainInventory->toolType[3] == Tool::Type::MOP)
							common.textureString = "T_RadialIcon_Mop.dds";
						else if (_mainInventory->toolType[3] == Tool::Type::SPONGE)
							common.textureString = "T_RadialIcon_Sponge.dds";
						else if (_mainInventory->toolType[3] == Tool::Type::FLASHLIGHT)
							common.textureString = "T_RadialIcon_Flashlight.dds";
					}
					common.texture = renderer.GetUITexture(common.textureString);
				}
				if (radialUI.type & RadialUI::Type::RIGHT)
				{
					if (_mainInventory->toolType[1] == Tool::DEFAULT)
						common.color.w = 0.0f;
					else
					{
						common.color.w = 1.0f;

						if (_mainInventory->toolType[1] == Tool::Type::AXE)
							common.textureString = "T_RadialIcon_Axe.dds";
						else if (_mainInventory->toolType[1] == Tool::Type::MOP)
							common.textureString = "T_RadialIcon_Mop.dds";
						else if (_mainInventory->toolType[1] == Tool::Type::SPONGE)
							common.textureString = "T_RadialIcon_Sponge.dds";
						else if (_mainInventory->toolType[1] == Tool::Type::FLASHLIGHT)
							common.textureString = "T_RadialIcon_Flashlight.dds";
					}
					common.texture = renderer.GetUITexture(common.textureString);
				}
				if (radialUI.type & RadialUI::Type::TOP)
				{
					if (_mainInventory->toolType[0] == Tool::DEFAULT)
						common.color.w = 0.0f;
					else
					{
						common.color.w = 1.0f;

						if (_mainInventory->toolType[0] == Tool::Type::AXE)
							common.textureString = "T_RadialIcon_Axe.dds";
						else if (_mainInventory->toolType[0] == Tool::Type::MOP)
							common.textureString = "T_RadialIcon_Mop.dds";
						else if (_mainInventory->toolType[0] == Tool::Type::SPONGE)
							common.textureString = "T_RadialIcon_Sponge.dds";
						else if (_mainInventory->toolType[0] == Tool::Type::FLASHLIGHT)
							common.textureString = "T_RadialIcon_Flashlight.dds";
					}
					common.texture = renderer.GetUITexture(common.textureString);
				}
				if (radialUI.type & RadialUI::Type::BOTTOM)
				{
					if (_mainInventory->toolType[2] == Tool::DEFAULT)
						common.color.w = 0.0f;
					else
					{
						common.color.w = 1.0f;

						if (_mainInventory->toolType[2] == Tool::Type::AXE)
							common.textureString = "T_RadialIcon_Axe.dds";
						else if (_mainInventory->toolType[2] == Tool::Type::MOP)
							common.textureString = "T_RadialIcon_Mop.dds";
						else if (_mainInventory->toolType[2] == Tool::Type::SPONGE)
							common.textureString = "T_RadialIcon_Sponge.dds";
						else if (_mainInventory->toolType[2] == Tool::Type::FLASHLIGHT)
							common.textureString = "T_RadialIcon_Flashlight.dds";
					}
					common.texture = renderer.GetUITexture(common.textureString);
				}
			}

			if (radialUI.type == RadialUI::Type::ARROW)
			{
				// WOO : editor viewport�� ���� ��ġ, ��ó�� ������ ���� �ʿ�
				Vector2 mousePosition = input.mousePosition - input.viewportPosition;
				//mousePosition.y -= 22.0f;

				// WOO : �̰� config ����� �̻��� �� ������� �ҵ�
				Vector2 centerPos = ui.toolVPSize / 2.0f;

#ifdef  _EDITOR
				if (ui.toolVPSize.x > config.width || ui.toolVPSize.y > config.height)
					centerPos = Vector2{ config.width / 2.0f, config.height / 2.0f };
#endif
				centerPos = Vector2{ config.width / 2.0f, config.height / 2.0f };

				Vector2 dir = mousePosition - centerPos;
				dir.Normalize();
				Vector2 originVec = Vector2(0.0f, -1.0f);

				float theta = std::acos(dir.Dot(originVec) / (dir.Length() * originVec.Length())) * 180.0f / DirectX::XM_PI;
				float crossProduct = dir.x * originVec.y - dir.y * originVec.x;

				if (crossProduct < 0)
					theta *= -1;

				ui.rotation = theta;

				// 4���� ������
				if (theta > -45.0f && theta <= 45.0f)
					direction = RadialUI::Type::TOP;
				else if (theta > 45.0f && theta <= 135.0f)
					direction = RadialUI::Type::LEFT;
				else if (theta > -135.0f && theta <= -45.0f)
					direction = RadialUI::Type::RIGHT;
				else
					direction = RadialUI::Type::BOTTOM;
			}

			if (radialUI.type & RadialUI::Type::DIRECTION)
			{
				if (radialUI.type & direction)
				{
					common.color.w = 1.0f;

					switch (direction)
					{
					case mc::RadialUI::LEFT:
						currentTool = 3;
						break;
					case mc::RadialUI::RIGHT:
						currentTool = 1;
						break;
					case mc::RadialUI::TOP:
						currentTool = 0;
						break;
					case mc::RadialUI::BOTTOM:
						currentTool = 2;
						break;
					default:
						OutputDebugStringA("wrong direction\n");
						break;
					}
				}
				else
					common.color.w = 0.0f;
			}

			if (radialUI.type & RadialUI::Type::TEXT)
			{
				auto type = _mainInventory->toolType[currentTool];

				if (type == Tool::Type::AXE)
					common.textureString = "T_RadialText_Axe.dds";
				else if (type == Tool::Type::MOP)
					common.textureString = "T_RadialText_Mop.dds";
				else if (type == Tool::Type::SPONGE)
					common.textureString = "T_RadialText_Sponge.dds";
				else if (type == Tool::Type::FLASHLIGHT)
					common.textureString = "T_RadialText_Flashlight.dds";
				else if (type == Tool::Type::DEFAULT)
					common.textureString = "T_RadialText_Hand.dds";
				common.texture = renderer.GetUITexture(common.textureString);
			}

			// ���콺 ���� ��ư�� ������ �ش��ϴ� �������� ���õǾ�� �Ѵ�.
			if (input.keyStates[VK_LBUTTON] == core::Input::State::Down ||
				input.keyStates[VK_TAB] == core::Input::State::Down)
			{
				common.isOn = false;

				global::mouseMode = global::MouseMode::Lock;

				if (_mainInventory)
				{
					_mainInventory->currentTool = currentTool;
					_isCurrentToolChanged = true;
				}

				_dispatcher->trigger<mc::OnInputLock>(OnInputLock{ false, false });

				// currentTool�� ���� uint32_t���� 
				// ���õǴ� �Ŵ� ���߿� �����̴� �׽�Ʈ�� �Բ� ����
			}
		}
		else
		{
			if (input.keyStates[VK_TAB] == core::Input::State::Down)
			{
				global::mouseMode = global::MouseMode::Clamp;
				_dispatcher->trigger<mc::OnInputLock>(OnInputLock{ true, false });
				common.isOn = !common.isOn;
			}
		}
	}

	if (_isCurrentToolChanged)
	{
		_animator->parameters["ActionTrigger"].value = false;
		_animator->parameters["IsAction"].value = false;
		// ���� �ٲ���� �� �ǹ��� �پ��ִ� ���� �ٲ���� �Ѵ�.
		if (_pivotEntity != entt::null)
		{
			if (auto* pivotSlot = scene.GetRegistry()->try_get<PivotSlot>(_pivotEntity))
			{
				for (int i = 0; i < 4; i++)
				{
					if (_mainInventory->toolSlot.tools[i] != entt::null)
					{
						if (i == _mainInventory->currentTool)
						{
							_dispatcher->trigger<mc::OnNotifyToolChanged>(mc::OnNotifyToolChanged{ scene, entt::null });
							core::Entity(_mainInventory->toolSlot.tools[i], *scene.GetRegistry()).SetParent(_pivotEntity);
							// �ش� Ʈ�������� 0���� �����.
							if (auto transform = scene.GetRegistry()->try_get<core::LocalTransform>(_mainInventory->toolSlot.tools[i]))
							{
								Quaternion q = Quaternion::Identity;
								Vector3 p = Vector3::Zero;
								Vector3 euler = Vector3::Zero;
								// �ش� Ʈ�������� �� ������ �°� �ٲ��ش�.
								switch (scene.GetRegistry()->get<Tool>(_mainInventory->toolSlot.tools[i]).type)
								{
								case Tool::Type::AXE:
									p = Vector3{ -0.01f, -0.01f, -0.07f };
									euler = Vector3{ 39.5f, 100.1f, -80.00f };
									break;
								case Tool::Type::MOP:
									p = Vector3{ 0.02f, -0.04f, -0.23f };
									euler = Vector3{ -4.1f, -10.4f, 174.504f };
									break;
								case Tool::Type::SPONGE:
									p = Vector3{ -0.05f, -0.04f, 0.04f };
									euler = Vector3{ 0.0f, 102.800f, -98.000f };
									break;
								case Tool::Type::FLASHLIGHT:
									p = Vector3{ -0.01f, -0.035f, 0.03f };
									euler = Vector3{ -81.90f, 0.0f, 175.2f };
									break;
								default:
									break;
								}
								transform->position = p;
								transform->rotation = Quaternion::CreateFromYawPitchRoll(euler * (DirectX::XM_PI / 180.f));
								scene.GetRegistry()->patch<core::LocalTransform>(_mainInventory->toolSlot.tools[i]);
							}
						}
						else
						{
							core::Entity(_mainInventory->toolSlot.tools[i], *scene.GetRegistry()).SetParent(entt::null);
							auto& transform = scene.GetRegistry()->get<core::WorldTransform>(_mainInventory->toolSlot.tools[i]);
							transform.position = Vector3{ 0.f, -100000.f, 0.f };
							transform.rotation = Quaternion::Identity;
							scene.GetRegistry()->patch<core::WorldTransform>(_mainInventory->toolSlot.tools[i]);
						}
					}
				}

				pivotSlot->isUsing = true;
			}
		}

		_isCurrentToolChanged = false;
	}
}

void mc::InventorySystem::startSystem(const core::OnStartSystem& event)
{
	for (auto&& [entity, inventory] : event.scene->GetRegistry()->view<Inventory>().each())
	{
		_mainInventory = &inventory;

		_mainInventory->toolSlot.tool1 = entt::null;	
		_mainInventory->toolSlot.tool2 = entt::null;
		_mainInventory->toolSlot.tool3 = entt::null;
		_mainInventory->toolSlot.tool4 = entt::null;
	}

	{
		auto view = event.scene->GetRegistry()->view<mc::PivotSlot>();
		auto&& registry = event.scene->GetRegistry();

		for (auto&& [entity, pivotSlot] : view.each())
		{
			if (pivotSlot.isUsing)
			{
				_pivotEntity = entity;
				break;
			}
		}
	}

	{
		auto view = event.scene->GetRegistry()->view<core::Animator>();
		auto&& registry = event.scene->GetRegistry();

		for (auto&& [entity, animator] : view.each())
		{
			_animator = &animator;
			break;
		}
	}
}

void mc::InventorySystem::finishSystem(const core::OnFinishSystem& event)
{

}
