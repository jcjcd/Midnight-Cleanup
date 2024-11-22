#include "pch.h"
#include "PhysicMaterialPanel.h"

#include <imgui_internal.h>

#include "ToolEvents.h"
#include "ToolProcess.h"

#include "../Animacore/PxResources.h"
#include "../Animacore/Scene.h"

tool::PhysicMaterialPanel::PhysicMaterialPanel(entt::dispatcher& dispatcher)
	: Panel(dispatcher)
{
	_dispatcher->sink<OnToolSelectFile>().connect<&PhysicMaterialPanel::selectFile>(this);
}

void tool::PhysicMaterialPanel::RenderPanel(float deltaTime)
{
	ImGui::Image(ToolProcess::icons["pmaterial"], {32, 32});
	ImGui::SameLine();
	ImGui::Text("%s (Physic Material)", _path.filename().replace_extension("").string().c_str());

	ImGui::Separator();

	ImGui::DragFloat("Dynamic Friction", &_physicMaterial.dynamicFriction);
	ImGui::DragFloat("Static Friction", &_physicMaterial.staticFriction);
	ImGui::DragFloat("Bounciness", &_physicMaterial.bounciness);

	if (ImGui::ButtonEx("Save"))
	{
		core::PxResources::SaveMaterial(_path, _physicMaterial);
	}
}

void tool::PhysicMaterialPanel::selectFile(const OnToolSelectFile& event)
{
	_path = event.path;

	if (_path.extension() == core::Scene::PHYSIC_MATERIAL_EXTENSION)
		_physicMaterial = core::PxResources::ReadMaterial(_path);
}
