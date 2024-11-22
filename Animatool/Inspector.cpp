#include "pch.h"
#include "Inspector.h"

#include "Hierarchy.h"
#include "ToolProcess.h"
#include "MaterialPanel.h"
#include "AnimatorDataPanel.h"
#include "ComponentDataPanel.h"
#include "PhysicMaterialPanel.h"

#include <Animacore/Scene.h>


tool::Inspector::Inspector(entt::dispatcher& dispatcher, Renderer& renderer)
	: Panel(dispatcher)
	, _renderer(&renderer)
{
	_dispatcher->sink<OnToolSelectEntity>().connect<&Inspector::selectEntity>(this);
	_dispatcher->sink<OnToolSelectFile>().connect<&Inspector::selectFile>(this);

	_materialPanel = std::make_shared<MaterialPanel>(*_dispatcher, _renderer);
	_animatorDataPanel = std::make_shared<AnimatorDataPanel>(*_dispatcher);
	_componentDataPanel = std::make_shared<ComponentDataPanel>(*_dispatcher, *_renderer);
	_physicMaterialPanel = std::make_shared<PhysicMaterialPanel>(*_dispatcher);
}

void tool::Inspector::RenderPanel(float deltaTime)
{
	if (ImGui::Begin("Inspector"))
	{
		switch (_currentMode)
		{
		case PanelMode::Entity:
			renderEntityMode(deltaTime);
			break;

		case PanelMode::Controller:
			renderControllerMode(deltaTime);
			break;

		case PanelMode::Material:
			renderMaterialMode(deltaTime);
			break;

		case PanelMode::PhysicMaterial:
			renderPhysicMaterialMode(deltaTime);
			break;

		default:
			break;
		}
	}

	ImGui::End();
}

void tool::Inspector::renderEntityMode(float deltaTime)
{
	_componentDataPanel->RenderPanel(deltaTime);
}

void tool::Inspector::renderControllerMode(float deltaTime)
{
	_animatorDataPanel->RenderPanel(deltaTime);
}

void tool::Inspector::renderMaterialMode(float deltaTime)
{
	_materialPanel->RenderPanel(deltaTime);
}

void tool::Inspector::renderPhysicMaterialMode(float deltaTime)
{
	_physicMaterialPanel->RenderPanel(deltaTime);
}

void tool::Inspector::selectEntity(const OnToolSelectEntity& event)
{
	_currentMode = PanelMode::Entity;
}

void tool::Inspector::selectFile(const OnToolSelectFile& event)
{
	_selectedFile = event.path;

	if (_selectedFile.extension() == core::Scene::PREFAB_EXTENSION)
	{
		//_currentMode = PanelMode::Prefab;
	}
	else if (_selectedFile.extension() == core::Scene::MATERIAL_EXTENSION)
	{
		_currentMode = PanelMode::Material;
	}
	else if (_selectedFile.extension() == core::Scene::PHYSIC_MATERIAL_EXTENSION)
	{
		_currentMode = PanelMode::PhysicMaterial;
	}
	else if (_selectedFile.extension() == core::Scene::CONTROLLER_EXTENSION)
	{
		_currentMode = PanelMode::Controller;
	}
	else
	{
		_currentMode = PanelMode::Entity;
	}
}
