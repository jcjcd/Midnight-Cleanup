#include "pch.h"
#include "ComponentDataPanel.h"

#include "ToolProcess.h"

#include <Animacore/Scene.h>
#include <Animacore/AnimatorSystem.h>
#include <Animacore/CoreComponents.h>
#include <Animacore/RenderComponents.h>
#include <Animacore/CoreTagsAndLayers.h>
#include <Animacore/CorePhysicsComponents.h>
#include <midnight_cleanup/McComponents.h>

tool::ComponentDataPanel::ComponentDataPanel(entt::dispatcher& dispatcher, Renderer& renderer)
	: Panel(dispatcher), _renderer(&renderer)
{
	_dispatcher->sink<OnToolSelectEntity>().connect<&ComponentDataPanel::selectEntity>(this);
	_dispatcher->sink<OnToolRemoveComponent>().connect<&ComponentDataPanel::removeComponent>(this);
}

void tool::ComponentDataPanel::RenderPanel(float deltaTime)
{
	auto& registry = *ToolProcess::scene->GetRegistry();

	if (!_selectedEntities.empty())
	{
		// ID 텍스트 중앙 정렬
		{
			float windowWidth = ImGui::GetWindowSize().x;
			std::string entityText = "ID : " + std::to_string((uint32_t)_selectedEntities.back());
			float textWidth = ImGui::CalcTextSize(entityText.c_str()).x;
			ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
			ImGui::Text("%s", entityText.c_str());
			ImGui::Separator();
		}

		// 공통 컴포넌트 검색
		std::vector<entt::meta_type> commonComponents;

		for (auto&& [id, meta] : entt::resolve(global::componentMetaCtx))
		{
			bool common = true;
			for (auto entity : _selectedEntities)
			{
				if (!meta.func("HasComponent"_hs).invoke({}, &registry, &entity).cast<bool>())
				{
					common = false;
					break;
				}
			}

			if (common)
				commonComponents.push_back(meta);
		}

		for (auto& metaType : commonComponents)
		{
			auto meta = entt::resolve(global::componentMetaCtx, metaType.id());
			std::string componentName = meta.info().name().data();
			size_t startPos = componentName.rfind("::");
			std::string subName = (startPos == std::string::npos) ? componentName : componentName.substr(startPos + 2);
			size_t endPos = subName.find(">(void)");
			subName = (endPos == std::string::npos) ? subName : subName.substr(0, endPos);

			ImGui::PushID(componentName.c_str());
			ImGui::AlignTextToFramePadding();
			ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_DefaultOpen
				| ImGuiTreeNodeFlags_Framed
				| ImGuiTreeNodeFlags_SpanAvailWidth
				| ImGuiTreeNodeFlags_AllowItemOverlap;

			bool treeOpen = ImGui::TreeNodeEx(subName.c_str(), nodeFlags);
			ImGui::SameLine(ImGui::GetWindowWidth() - 30);
			if (ImGui::Button("+")) ImGui::OpenPopup("ComponentOptions");
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Component options");

			if (ImGui::BeginPopup("ComponentOptions"))
			{
				if (ImGui::MenuItem("Reset Component"))
				{
					for (auto entity : _selectedEntities)
						meta.func("ResetComponent"_hs).invoke({}, &registry, &entity);
				}
				if (ImGui::MenuItem("Remove Component"))
				{
					for (auto entity : _selectedEntities)
						_dispatcher->enqueue<OnToolRemoveComponent>(meta, &registry, entity);
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			if (treeOpen)
			{
				// 마지막으로 선택된 엔티티 기준 컴포넌트 값
				auto instance = meta.func("GetComponent"_hs).invoke({}, &registry, &_selectedEntities.back());

				// 변경 플래그
				bool isChanged = false;

				// 컴포넌트 필드 출력 및 수정
				for (auto&& [id, data] : meta.data())
				{
					renderComponentField(data, instance, isChanged);
				}

				// 특수화
				if (isChanged)
				{
					bool isCommon = true;

					if (subName == "MeshRenderer")
					{
						isCommon = false;

						// 마지막으로 선택한 엔티티의 변경된 인스턴스
						auto newRenderer = instance.cast<core::MeshRenderer>();

						// 마지막으로 선택한 엔티티의 변경 전 인스턴스
						auto& oldRenderer = registry.get<core::MeshRenderer>(_selectedEntities.back());

						for (auto entity : _selectedEntities)
						{
							// 선택된 엔티티의 인스턴스
							auto& selected = registry.get<core::MeshRenderer>(entity);

							if (oldRenderer.meshString != newRenderer.meshString)
							{
								selected.meshString = newRenderer.meshString;
								selected.mesh = _renderer->GetMesh(selected.meshString);
							}

							bool isMaterialChanged = false;

							if (oldRenderer.materialStrings.size() != newRenderer.materialStrings.size())
							{
								isMaterialChanged = true;
							}
							else
							{
								auto newiter = newRenderer.materialStrings.begin();
								auto olditer = oldRenderer.materialStrings.begin();
								for (int i = 0; i < newRenderer.materialStrings.size(); ++i, ++newiter, ++olditer)
								{
									if (*newiter != *olditer)
									{
										isMaterialChanged = true;
										break;
									}
								}
							}

							if (isMaterialChanged)
							{
								auto size = newRenderer.materialStrings.size();
								selected.materials.clear();
								selected.materialStrings.clear();
								selected.materials.reserve(size);

								for (auto iter = newRenderer.materialStrings.begin(); iter != newRenderer.materialStrings.end(); ++iter)
								{
									selected.materials.push_back(_renderer->GetMaterial(*iter));
									selected.materialStrings.push_back(*iter);
								}

							}

							bool isBooleanChanged = newRenderer.isSkinned != oldRenderer.isSkinned ||
								newRenderer.isCulling != oldRenderer.isCulling ||
								newRenderer.canReceivingDecal != oldRenderer.canReceivingDecal ||
								newRenderer.isOn != oldRenderer.isOn ||
								newRenderer.receiveShadow != oldRenderer.receiveShadow;

							if (isBooleanChanged)
							{
								selected.isSkinned = newRenderer.isSkinned;
								selected.isCulling = newRenderer.isCulling;
								selected.canReceivingDecal = newRenderer.canReceivingDecal;
								selected.isOn = newRenderer.isOn;
								selected.receiveShadow = newRenderer.receiveShadow;
							}

							_dispatcher->enqueue<OnToolModifyRenderer>(entity);
						}
					}

					if (isCommon)
						for (auto entity : _selectedEntities)
							meta.func("SetComponent"_hs).invoke({}, &registry, &entity, &instance);

					if (subName == "UICommon")
					{
						for (auto entity : _selectedEntities)
						{
							auto&& ui = core::Entity{ entity, *ToolProcess::scene->GetRegistry() }.Get<core::UICommon>();
							ui.texture = _renderer->GetUITexture(ui.textureString);
							_dispatcher->trigger<OnToolModifyRenderer>({ entity });
						}
					}
					else if (subName == "Text")
					{
						for (auto entity : _selectedEntities)
						{
							auto&& ui = core::Entity{ entity, *ToolProcess::scene->GetRegistry() }.Get<core::Text>();
							ui.font = _renderer->GetFont(ui.fontString);
							_dispatcher->trigger<OnToolModifyRenderer>({ entity });
						}
					}
					else if (subName == "Particle System")
					{
						for (auto entity : _selectedEntities)
						{
							auto&& particle = core::Entity{ entity, *ToolProcess::scene->GetRegistry() }.Get<core::ParticleSystem>();

							if (particle.renderData.useBaseColorTexture)
								particle.baseColorTexture = _renderer->GetTexture(particle.renderData.baseColorTextureString.c_str());

							if (particle.renderData.useEmissiveTexture)
								particle.emissiveTexture = _renderer->GetTexture(particle.renderData.emissiveTextureString.c_str());

							_dispatcher->trigger<OnToolModifyRenderer>({ entity });
						}
					}
					else if (subName == "Decal")
					{
						for (auto entity : _selectedEntities)
						{
							auto&& decal = core::Entity{ entity, *ToolProcess::scene->GetRegistry() }.Get<core::Decal>();

							decal.material = _renderer->GetMaterial(decal.materialString);

							_dispatcher->trigger<OnToolModifyRenderer>({ entity });
						}
					}

					BroadcastModifyScene();
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
			ImGui::Separator();
		}

		// Add Component 버튼을 가운데에 배치
		float windowWidth = ImGui::GetWindowSize().x;
		float buttonWidth = ImGui::CalcTextSize("Add Component").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponentPopup");

		showAddComponentPopup(registry);
	}
}

void tool::ComponentDataPanel::showAddComponentPopup(entt::registry& registry)
{
	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		// 검색 기능
		static char searchBuffer[128] = "";
		ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer));
		std::string searchString = searchBuffer;

		// 팝업 창의 가로 폭 조정
		ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_Always);

		// 스크롤 가능한 영역 시작
		if (ImGui::BeginChild("ComponentList", ImVec2(0, 300), true))
		{
			for (auto&& [id, meta] : entt::resolve(global::componentMetaCtx))
			{
				// 타 컴포넌트에 속해 있는 컴포넌트 제외
				if (meta.prop("is_hidden"_hs))
					continue;

				std::string componentName = meta.info().name().data();

				// 마지막 :: 이후 부분 추출
				size_t startPos = componentName.rfind("::");
				std::string subName = (startPos == std::string::npos) ? componentName : componentName.substr(startPos + 2);

				// 처음 등장하는 >(void) 전까지의 부분 추출
				size_t endPos = subName.find(">(void)");
				subName = (endPos == std::string::npos) ? subName : subName.substr(0, endPos);

				// 원본 이름을 소문자로 변환하여 검색
				std::string lower = subName;
				std::ranges::transform(lower, lower.begin(), ::tolower);

				// 검색어가 포함된 컴포넌트만 표시
				if (lower.find(searchString) == std::string::npos)
					continue;

				// 컴포넌트 이름을 가운데에 배치하고, 버튼의 가로 크기를 팝업 창의 가로 크기로 설정
				float availableWidth = ImGui::GetContentRegionAvail().x;

				if (ImGui::Button(subName.c_str(), ImVec2(availableWidth, 0)))
				{
					for (auto& entity : _selectedEntities)
					{
						// 컴포넌트 추가
						if (auto needs = meta.prop("need_components"_hs))
						{
							auto needCompIds = needs.value().cast<std::vector<entt::id_type>>();

							for (const auto& cId : needCompIds)
							{
								// 각 필요한 컴포넌트의 메타 정보 가져오기
								if (auto cMeta = entt::resolve(global::componentMetaCtx, cId))
								{
									// 필요한 컴포넌트가 해당 엔티티에 없을 경우에만 추가
									auto hasFunc = cMeta.func("HasComponent"_hs);
									auto addFunc = cMeta.func("AddComponent"_hs);

									if (!hasFunc.invoke({}, &registry, &entity).cast<bool>())
										addFunc.invoke({}, &registry, &entity);
								}
							}
						}
						else
						{
							// 이미 존재하는 컴포넌트일 경우
							if (meta.func("HasComponent"_hs).invoke({}, &registry, &entity).cast<bool>())
								continue;

							meta.func("AddComponent"_hs).invoke({}, &registry, &entity);
						}

						BroadcastModifyScene();
					}
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndChild(); // 스크롤 가능한 영역 종료
		}

		ImGui::EndPopup();
	}
}

std::vector<std::string> tool::ComponentDataPanel::getFilesWithExtension(const std::string& directory,
	const std::string& extension, bool isFullName)
{
	std::vector<std::string> files;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
	{
		if (entry.is_regular_file() && entry.path().extension() == extension)
		{
			if (isFullName)
				files.push_back(entry.path().string());
			else
				files.push_back(entry.path().filename().replace_extension("").string());
		}
	}

	return files;
}

void tool::ComponentDataPanel::renderComponentField(const entt::meta_data& data, entt::meta_any& instance,
	bool& isModified)
{
	// enum 일 경우
	if (data.type().is_enum())
	{
		renderEnumField(data, instance, isModified);
		return;
	}

	const auto propName = data.prop("name"_hs).value().cast<const char*>();
	const auto hash = data.type().info().hash();
	auto valueAny = data.get(entt::meta_handle{ instance });

	ImGui::PushID(hash);

	// min & max (존재 할 경우 적용)
	auto min = data.prop("min"_hs);
	auto max = data.prop("max"_hs);
	auto dragSpeed = data.prop("dragSpeed"_hs);

	// 툴팁 (존재 할 경우 출력)
	auto desc = data.prop("description"_hs);

	std::string propNameStr = propName;

	// 특수화
	if (propName == "Mesh")
	{
		auto val = valueAny.cast<std::string>();
		renderStringCombo(propName, val, *_renderer->GetMeshes(), data, instance, isModified);
	}

	else if (propName == "Materials")
	{
		renderMaterials(data, instance, isModified);
	}
	else if (propName == "Material")
	{
		std::filesystem::path path = valueAny.cast<std::string>();
		auto shortPath = path.filename().replace_extension();
		auto currentItem = shortPath.string();

		renderStringCombo(propName, currentItem, *_renderer->GetMaterials(), data, instance, isModified);
	}

	else if (propName == "Physic Material")
	{
		auto val = valueAny.cast<std::string>();

		renderStringCombo(propName, val, "./Resources/",
			core::Scene::PHYSIC_MATERIAL_EXTENSION, data, instance, isModified);
	}

	else if (propName == "Controller")
	{
		auto val = valueAny.cast<std::string>();
		auto& controllers = core::AnimatorSystem::controllerManager._controllers;
		renderStringCombo(propName, val, controllers, data, instance, isModified);
	}

	else if (propName == "UI Texture" || propName == "Highlight Texture" 
		|| propName == "Pressed Texture" || propName == "Disabled Texture" 
		|| propName == "Selected Texture" || propName == "Unchecked Texture"
		|| propName == "Checked Texture")
	{
		auto val = valueAny.cast<std::string>();
		renderStringCombo2(propName, val, *_renderer->GetUITextures(), data, instance, isModified);
	}

	else if (propName == "Particle Base Color Texture" || propName == "Particle Emissive Texture")
	{
		auto val = valueAny.cast<std::string>();
		renderStringCombo(propName, val, *_renderer->GetParticleTextures(), data, instance, isModified);
	}

	else if (propName == "Font")
	{
		auto val = valueAny.cast<std::string>();

		renderStringCombo2(propName, val, *_renderer->GetFonts(), data, instance, isModified);
	}

	else if (instance.type().id() == entt::type_id<core::Sound>().hash() && propName == "Path")
	{
		auto val = valueAny.cast<std::string>();

		renderStringCombo(propName, val, "./Resources/",
			core::Scene::SOUND_EXTENSION, data, instance, isModified);
	}

	else if (data.type().is_sequence_container())
	{
		if (propName == "Bursts")
			renderSequenceContainer<std::vector<core::ParticleSystem::EmissionModule::Burst>>(data, instance, isModified);
		else if (propName == "Alpha Ratios")
			renderSequenceContainer<std::vector<Vector2>>(data, instance, isModified);
		else if (propName == "Color Ratios")
			renderSequenceContainer<std::vector<Vector4>>(data, instance, isModified);
		else if (propName == "Texts")
			renderSequenceContainer<std::vector<core::ComboBox::StringWrapper>>(data, instance, isModified);
		else if (propName == "On Click Functions")
			renderButtonEvents(data, instance, isModified);
	}

	// 기본 타입
	else if (hash == entt::type_hash<float>())
	{
		auto val = valueAny.cast<float>();

		auto vMin = min ? min.value().cast<float>() : 0.f;
		auto vMax = max ? max.value().cast<float>() : 0.f;
		auto speed = dragSpeed ? dragSpeed.value().cast<float>() : 1.f;

		if (ImGui::DragFloat(propName, &val, speed, vMin, vMax))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<int>())
	{
		auto val = valueAny.cast<int>();

		auto vMin = min ? min.value().cast<int>() : 0;
		auto vMax = max ? max.value().cast<int>() : 0;
		auto speed = dragSpeed ? dragSpeed.value().cast<int>() : 1;

		if (ImGui::DragInt(propName, &val, (float)speed, vMin, vMax))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<uint32_t>())
	{
		auto val = valueAny.cast<uint32_t>();

		if (instance.type().info().hash() == entt::type_hash<core::Tag>())
		{
			renderStructCombo(global::tagMetaCtx, valueAny, data, instance, isModified);
		}
		else if (instance.type().info().hash() == entt::type_hash<core::Layer>())
		{
			// 현재 레이어 마스크를 가져옴
			uint32_t layerMask = valueAny.cast<uint32_t>();

			// global::layerMetaCtx에서 등록된 모든 레이어 정보를 순회
			for (auto&& [id, type] : entt::resolve(global::layerMetaCtx))
			{
				if (auto layerNameProp = type.prop("name"_hs))
				{
					const char* layerName = layerNameProp.value().cast<const char*>();

					// "layer::"을 제거하기 위한 코드
					std::string formattedLayerName = layerName;
					auto pos = formattedLayerName.find("layer::");
					if (pos != std::string::npos)
						formattedLayerName = formattedLayerName.substr(pos + 7);

					if (auto maskProp = type.prop("mask"_hs))
					{
						uint32_t layerBit = maskProp.value().cast<uint32_t>();

						// 가공된 레이어 이름을 사용하여 CheckboxFlags 표시
						ImGui::CheckboxFlags(formattedLayerName.c_str(), &layerMask, layerBit);
					}
				}
			}

			// 레이어 마스크가 수정되었음을 반영
			data.set(entt::meta_handle{ instance }, layerMask);
			isModified = true;
		}
		else if (ImGui::DragScalar(propName, ImGuiDataType_U32, &val))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<bool>())
	{
		auto val = valueAny.cast<bool>();

		if (ImGui::Checkbox(propName, &val))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<char>())
	{
		auto val = valueAny.cast<char>();

		if (ImGui::DragScalar(propName, ImGuiDataType_S8, &val))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<short>())
	{
		auto val = valueAny.cast<short>();

		if (ImGui::DragScalar(propName, ImGuiDataType_S16, &val))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<uint64_t>())
	{
		auto val = valueAny.cast<uint64_t>();

		if (ImGui::DragScalar(propName, ImGuiDataType_U64, &val))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<double>())
	{
		auto val = valueAny.cast<double>();

		auto vMin = min ? min.value().cast<float>() : 0.f;
		auto vMax = max ? max.value().cast<float>() : 0.f;
		auto speed = dragSpeed ? dragSpeed.value().cast<float>() : 1.f;

		if (ImGui::DragScalar(propName, ImGuiDataType_Double, &val))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<std::string>())
	{
		auto val = valueAny.cast<std::string>();

		if (ImGui::InputText(propName, &val))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<Vector2>())
	{
		auto val = valueAny.cast<Vector2>();

		auto vMin = min ? min.value().cast<float>() : 0.f;
		auto vMax = max ? max.value().cast<float>() : 0.f;
		auto speed = dragSpeed ? dragSpeed.value().cast<float>() : 1.f;

		if (ImGui::DragFloat2(propName, &val.x, speed, vMin, vMax))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<Vector3>())
	{
		auto val = valueAny.cast<Vector3>();

		auto vMin = min ? min.value().cast<float>() : 0.f;
		auto vMax = max ? max.value().cast<float>() : 0.f;
		auto speed = dragSpeed ? dragSpeed.value().cast<float>() : 1.f;

		if (ImGui::DragFloat3(propName, &val.x, speed, vMin, vMax))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<Vector4>())
	{
		auto val = valueAny.cast<Vector4>();

		auto vMin = min ? min.value().cast<float>() : 0.f;
		auto vMax = max ? max.value().cast<float>() : 0.f;
		auto speed = dragSpeed ? dragSpeed.value().cast<float>() : 1.f;

		if (ImGui::DragFloat4(propName, &val.x, speed, vMin, vMax))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<Quaternion>())
	{
		using namespace DirectX;

		auto val = valueAny.cast<Quaternion>();
		Vector3 euler = val.ToEuler() / (XM_PI / 180.f);

		auto vMin = min ? min.value().cast<float>() : 0.f;
		auto vMax = max ? max.value().cast<float>() : 0.f;
		auto speed = dragSpeed ? dragSpeed.value().cast<float>() : 1.f;

		if (ImGui::DragFloat3(propName, &euler.x, speed, vMin, vMax))
		{
			val = Quaternion::CreateFromYawPitchRoll(euler * (XM_PI / 180.f));
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else if (hash == entt::type_hash<DirectX::SimpleMath::Color>())
	{
		auto val = valueAny.cast<DirectX::SimpleMath::Color>();

		auto vMin = min ? min.value().cast<float>() : 0.f;
		auto vMax = max ? max.value().cast<float>() : 0.f;
		auto speed = dragSpeed ? dragSpeed.value().cast<float>() : 1.f;

		if (ImGui::DragFloat4(propName, &val.x, speed, vMin, vMax))
		{
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
	}

	else
	{
		renderStructField(data, instance, isModified);
	}

	if (desc && ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(desc.value().cast<const char*>());
	}

	ImGui::PopID();
}

void tool::ComponentDataPanel::renderEnumField(const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
{
	const auto hash = data.type().info().hash();

	// 참조 관계 임의 변경 불가
	if (hash == entt::type_hash<entt::entity>())
	{
		renderEntity(data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::Rigidbody::Interpolation>())
	{
		renderInterpolation(data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::Rigidbody::Constraints>())
	{
		renderConstraints(data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::MeshCollider::MeshColliderCookingOptions>())
	{
		renderCookingOptions(data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::CapsuleCollider::Axis>())
	{
		renderAxis(data, instance, isModified);
	}
	else if (hash == entt::type_hash<mc::RadialUI::Type>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<mc::RadialUI::Type>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<mc::Tool::Type>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<mc::Tool::Type>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::LightCommon::LightMode>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<core::LightCommon::LightMode>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::ParticleSystem::Option>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<core::ParticleSystem::Option>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::ParticleSystem::ShapeModule::Shape>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<core::ParticleSystem::ShapeModule::Shape>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::ParticleSystem::RenderModule::RenderMode>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<core::ParticleSystem::RenderModule::RenderMode>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::ParticleSystem::RenderModule::ColorMode>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<core::ParticleSystem::RenderModule::ColorMode>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::UICommon::MaskingOption>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<core::UICommon::MaskingOption>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::UI3D::Constrained>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<core::UI3D::Constrained>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::Text::TextAlign>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<core::Text::TextAlign>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::Text::TextBoxAlign>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<core::Text::TextBoxAlign>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::RenderAttributes::Flag>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumFlag<core::RenderAttributes::Flag>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<mc::Room::Type>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<mc::Room::Type>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<core::PostProcessingVolume::BloomCurve>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<core::PostProcessingVolume::BloomCurve>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<mc::Door::State>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<mc::Door::State>(valueAny, data, instance, isModified);
	}
	else if (hash == entt::type_hash<mc::SoundMaterial::Type>())
	{
		auto valueAny = data.get(entt::meta_handle{ instance });
		renderEnumCombo<mc::SoundMaterial::Type>(valueAny, data, instance, isModified);
	}
}

void tool::ComponentDataPanel::renderStructField(const entt::meta_data& data, entt::meta_any& instance,
	bool& isModified)
{
	const auto propName = data.prop("name"_hs).value().cast<const char*>();
	auto valueAny = data.get(entt::meta_handle{ instance });

	if (ImGui::TreeNode(propName))
	{
		const auto structType = entt::resolve(global::customStructMetaCtx, data.type().info());

		for (auto&& field : structType.data())
		{
			renderComponentField(field.second, valueAny, isModified);
		}

		if (isModified)
			data.set(entt::meta_handle{ instance }, valueAny);

		ImGui::TreePop();
	}
}

void tool::ComponentDataPanel::renderEntity(const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
{
	const auto propName = data.prop("name"_hs).value().cast<const char*>();
	auto valueAny = data.get(entt::meta_handle{ instance });

	auto val = valueAny.cast<entt::entity>();

	if (val != entt::null)
	{
		ImGui::InputScalar(propName, ImGuiDataType_U32, &val, NULL, NULL, "%u", ImGuiInputTextFlags_ReadOnly);
	}
	else
	{
		char text[] = "None";
		ImGui::InputText(propName, text, sizeof(text), ImGuiInputTextFlags_ReadOnly);
	}

	// 엔티티 드래그앤 드롭
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_PAYLOAD"))
		{
			auto payloadData = *(const entt::entity*)payload->Data;
			if (payloadData == _selectedEntities.back() && strcmp(propName, "Parent") == 0)
			{
				ImGui::OpenPopup("Invalid Parent Popup");
			}
			else
			{
				val = *(const entt::entity*)payload->Data;
				data.set(entt::meta_handle{ instance }, val);
				isModified = true;
			}
		}
		ImGui::EndDragDropTarget();
	}

	// 팝업을 모달로 열기 위해 ImGui::OpenPopupModal 사용
	if (ImGui::BeginPopupModal("Invalid Parent Popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::Text("Cannot set an entity as its own parent");
		ImGui::Separator();

		// OK 버튼을 팝업창의 글상자 아래쪽 가운데에 배치
		float windowWidth = ImGui::GetWindowSize().x;
		float buttonWidth = ImGui::CalcTextSize("OK").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

		if (ImGui::Button("OK"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void tool::ComponentDataPanel::renderInterpolation(const entt::meta_data& data, entt::meta_any& instance,
	bool& isModified)
{
	const auto propName = data.prop("name"_hs).value().cast<const char*>();
	auto valueAny = data.get(entt::meta_handle{ instance });

	renderEnumCombo<core::Rigidbody::Interpolation>(valueAny, data, instance, isModified);
}

void tool::ComponentDataPanel::renderConstraints(const entt::meta_data& data, entt::meta_any& instance,
	bool& isModified)
{
	const auto propName = data.prop("name"_hs).value().cast<const char*>();
	auto valueAny = data.get(entt::meta_handle{ instance });

	if (valueAny.allow_cast<uint32_t>()) {
		auto& val = valueAny.cast<uint32_t&>();

		if (ImGui::TreeNode(propName))
		{
			// 개별 플래그 체크박스
			bool freezePosX = val & static_cast<uint32_t>(core::Rigidbody::Constraints::FreezePositionX);
			bool freezePosY = val & static_cast<uint32_t>(core::Rigidbody::Constraints::FreezePositionY);
			bool freezePosZ = val & static_cast<uint32_t>(core::Rigidbody::Constraints::FreezePositionZ);
			bool freezeRotX = val & static_cast<uint32_t>(core::Rigidbody::Constraints::FreezeRotationX);
			bool freezeRotY = val & static_cast<uint32_t>(core::Rigidbody::Constraints::FreezeRotationY);
			bool freezeRotZ = val & static_cast<uint32_t>(core::Rigidbody::Constraints::FreezeRotationZ);

			// Position Constraints
			ImGui::PushID("Freeze Position");
			if (ImGui::Checkbox("X", &freezePosX)) {
				val = (freezePosX ? val | static_cast<uint32_t>(core::Rigidbody::Constraints::FreezePositionX)
					: val & ~static_cast<uint32_t>(core::Rigidbody::Constraints::FreezePositionX));
				isModified = true;
			}
			ImGui::SameLine();
			if (ImGui::Checkbox("Y", &freezePosY)) {
				val = (freezePosY ? val | static_cast<uint32_t>(core::Rigidbody::Constraints::FreezePositionY)
					: val & ~static_cast<uint32_t>(core::Rigidbody::Constraints::FreezePositionY));
				isModified = true;
			}
			ImGui::SameLine();
			if (ImGui::Checkbox("Z", &freezePosZ)) {
				val = (freezePosZ ? val | static_cast<uint32_t>(core::Rigidbody::Constraints::FreezePositionZ)
					: val & ~static_cast<uint32_t>(core::Rigidbody::Constraints::FreezePositionZ));
				isModified = true;
			}
			ImGui::SameLine();
			ImGui::Text("Freeze Position");
			ImGui::PopID();

			// Rotation Constraints
			ImGui::PushID("Freeze Rotation");
			if (ImGui::Checkbox("X", &freezeRotX)) {
				val = (freezeRotX ? val | static_cast<uint32_t>(core::Rigidbody::Constraints::FreezeRotationX)
					: val & ~static_cast<uint32_t>(core::Rigidbody::Constraints::FreezeRotationX));
				isModified = true;
			}
			ImGui::SameLine();
			if (ImGui::Checkbox("Y", &freezeRotY)) {
				val = (freezeRotY ? val | static_cast<uint32_t>(core::Rigidbody::Constraints::FreezeRotationY)
					: val & ~static_cast<uint32_t>(core::Rigidbody::Constraints::FreezeRotationY));
				isModified = true;
			}
			ImGui::SameLine();
			if (ImGui::Checkbox("Z", &freezeRotZ)) {
				val = (freezeRotZ ? val | static_cast<uint32_t>(core::Rigidbody::Constraints::FreezeRotationZ)
					: val & ~static_cast<uint32_t>(core::Rigidbody::Constraints::FreezeRotationZ));
				isModified = true;
			}
			ImGui::SameLine();
			ImGui::Text("Freeze Rotation");
			ImGui::PopID();

			// 상태 갱신
			if (isModified)
				data.set(entt::meta_handle{ instance }, val);

			ImGui::TreePop();
		}
	}
}

void tool::ComponentDataPanel::renderMaterials(const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
{
	if (ImGui::TreeNode("Materials"))
	{
		auto val = data.get(entt::meta_handle{ instance }).cast<std::list<std::string>>();

		if (val.size() == 0)
		{
			ImGui::Text("List is Empty");
		}

		auto iter = val.begin();
		int i = 0;
		for (iter; iter != val.end(); ++iter, ++i)
		{
			ImGui::PushID(iter->c_str());
			std::string elementIndexString = "element" + std::to_string(i);
			std::filesystem::path path = *iter;
			auto shortPath = path.filename().replace_extension();
			auto currentItem = shortPath.string();

			if (ImGui::BeginCombo(elementIndexString.c_str(), currentItem.c_str()))
			{
				for (auto&& [key, value] : *_renderer->GetMaterials())
				{
					bool isSelected = (*iter == key);
					std::filesystem::path path = key;
					auto shortPath = path.filename().replace_extension();
					if (ImGui::Selectable(shortPath.string().c_str(), isSelected))
					{
						*iter = key;
						data.set(entt::meta_handle{ instance }, val);
						isModified = true;
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			ImGui::Button(" = ");
			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("MATERIAL_DRAG_SWAP", &i, sizeof(int));
				ImGui::EndDragDropSource();
			}
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL_DRAG_SWAP"))
				{
					IM_ASSERT(payload->DataSize == sizeof(int));
					int payload_i = *(const int*)payload->Data;

					auto innerIter = val.begin();
					std::advance(innerIter, payload_i);

					std::swap(*iter, *innerIter);
					data.set(entt::meta_handle{ instance }, val);
					isModified = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::PopID();
		}

		if (ImGui::Button("+"))
		{
			if (val.size() == 0)
				val.push_back("");
			else
				val.push_back(val.back());
			data.set(entt::meta_handle{ instance }, val);
			isModified = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("-"))
		{
			if (val.size() > 0)
			{
				val.pop_back();
				data.set(entt::meta_handle{ instance }, val);
				isModified = true;
			}
		}

		ImGui::TreePop();
	}
}

void tool::ComponentDataPanel::renderCookingOptions(const entt::meta_data& data, entt::meta_any& instance,
	bool& isModified)
{
	const auto propName = data.prop("name"_hs).value().cast<const char*>();
	auto valueAny = data.get(entt::meta_handle{ instance });

	renderEnumFlag<core::MeshCollider::MeshColliderCookingOptions>(valueAny, data, instance, isModified);
}

void tool::ComponentDataPanel::renderAxis(const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
{
	const auto propName = data.prop("name"_hs).value().cast<const char*>();
	auto valueAny = data.get(entt::meta_handle{ instance });

	renderEnumCombo<core::CapsuleCollider::Axis>(valueAny, data, instance, isModified);
}

void tool::ComponentDataPanel::renderButtonEvents(const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
{
	auto meta = entt::resolve<global::CallBackFuncDummy>(global::callbackEventMetaCtx);
	auto funcs = meta.func();

	if (ImGui::TreeNode("On Click Functions"))
	{
		auto val = data.get(entt::meta_handle{ instance }).cast<std::vector<core::ButtonEvent>>();

		if (val.size() == 0)
		{
			ImGui::Text("List is Empty");
		}

		auto& events = val;
		for (int i = 0; i < events.size(); ++i)
		{
			ImGui::PushID(i);
			auto& event = events[i];
			std::string functionName = event.functionName;
			if (ImGui::BeginCombo("Event", functionName.c_str()))
			{
				for (auto&& func : funcs)
				{
					bool isSelected = functionName == func.second.prop("name"_hs).value().cast<const char*>();
					if (ImGui::Selectable(func.second.prop("name"_hs).value().cast<const char*>(), isSelected))
					{
						event.functionName = func.second.prop("name"_hs).value().cast<const char*>();
						event.parameters.resize(func.second.arity());
						for (int j = 0; j < func.second.arity(); ++j)
						{
							auto param = func.second.arg(j);
							if (param.info() == entt::resolve<int>().info())
							{
								event.parameters[j] = 0;
							}
							else if (param.info() == entt::resolve<float>().info())
							{
								event.parameters[j] = 0.0f;
							}
							else if (param.info() == entt::resolve<bool>().info())
							{
								event.parameters[j] = false;
							}
							else if (param.info() == entt::resolve<std::string>().info())
							{
								event.parameters[j] = std::string("");
							}
							else if (param.info() == entt::resolve<entt::entity>().info())
							{
								event.parameters[j] = entt::entity(entt::null);
							}
							else
							{
								event.parameters[j] = std::string("");
							}
						}
						isModified = true;
					}
					if (isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::SameLine();

			if (ImGui::Button("X"))
			{
				events.erase(events.begin() + i);
				isModified = true;
			}

			auto&& func = meta.func(entt::hashed_string{ event.functionName.c_str() });

			if (func)
			{
				auto size = func.arity();
				for (int i = 0; i < size; ++i)
				{
					ImGui::PushID(i);
					auto param = func.arg(i);
					// ImGui::Text(param.info().name().data());
					// 여기에 세부 수치를 조절 가능해야함
					if (param.info() == entt::resolve<int>().info())
					{
						ImGui::Text("int");
						ImGui::SameLine();
						int temp = event.parameters[i].cast<int>();
						if (ImGui::InputInt("##value", &temp))
						{
							event.parameters[i] = temp;
							isModified = true;
						}
					}
					else if (param.info() == entt::resolve<float>().info())
					{
						ImGui::Text("float");
						ImGui::SameLine();
						float temp = event.parameters[i].cast<float>();
						if (ImGui::InputFloat("##value", &temp))
						{
							event.parameters[i] = temp;
							isModified = true;
						}
					}
					else if (param.info() == entt::resolve<bool>().info())
					{
						ImGui::Text("bool");
						ImGui::SameLine();
						bool temp = event.parameters[i].cast<bool>();
						int intTemp = temp;
						if (ImGui::Combo("##value", &intTemp, "False\0True\0"))
						{
							event.parameters[i] = static_cast<bool>(intTemp);
							isModified = true;
						}
					}
					else if (param.info() == entt::resolve<std::string>().info())
					{
						ImGui::Text("string");
						ImGui::SameLine();
						std::string temp = event.parameters[i].cast<std::string>();

						bool isDirty = ImGui::InputText("##value", &temp);

						// 드래그 앤 드롭 동작 처리
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PAYLOAD"))
							{
								auto path = std::string(static_cast<const char*>(payload->Data));
								temp = path;
								isDirty = true;
							}
							ImGui::EndDragDropTarget();
						}

						if (isDirty)
						{
							event.parameters[i] = temp;
							isModified = true;
						}
					}
					else if (param.info() == entt::resolve<entt::entity>().info())
					{
						ImGui::Text("entity");
						ImGui::SameLine();
						entt::entity temp = event.parameters[i].cast<entt::entity>();
						bool isDirty = ImGui::InputScalar("##value", ImGuiDataType_U32, &temp);

						// 드래그 앤 드롭 동작 처리
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_PAYLOAD"))
							{
								auto entity = static_cast<entt::entity*>(payload->Data);
								temp = *entity;
								isDirty = true;
							}
							ImGui::EndDragDropTarget();
						}

						if (isDirty)
						{
							event.parameters[i] = temp;
							isModified = true;
						}
					}
					else
					{
						ImGui::Text(param.info().name().data());
					}

					ImGui::PopID();
				}

			}

			ImGui::PopID();

			ImGui::Separator();
		}

		if (ImGui::Button("+"))
		{
			events.push_back(core::ButtonEvent());
			isModified = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("-"))
		{
			if (events.size() > 0)
			{
				events.pop_back();
				isModified = true;
			}
		}

		if (isModified)
			data.set(entt::meta_handle{ instance }, events);

		ImGui::TreePop();
	}

}

void tool::ComponentDataPanel::renderStructCombo(entt::meta_ctx& ctx, entt::meta_any& valueAny, const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
{
	const auto dataName = data.prop("name"_hs).value().cast<const char*>();
	auto val = valueAny.cast<uint32_t>();

	std::vector<std::pair<uint32_t, const char*>> names;
	const char* currentName = nullptr;

	for (auto&& [id, type] : entt::resolve(ctx))
	{
		auto name = type.prop("name"_hs).value().cast<const char*>();
		names.emplace_back(id, name);

		if (id == val)
			currentName = name;
	}

	if (ImGui::BeginCombo(dataName, currentName))
	{
		for (const auto& tag : names)
		{
			bool isSelected = (tag.first == val);

			if (ImGui::Selectable(tag.second, isSelected))
			{
				val = tag.first;
				data.set(entt::meta_handle{ instance }, val);
				isModified = true;
			}
			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

void tool::ComponentDataPanel::selectEntity(const OnToolSelectEntity& event)
{
	_selectedEntities = event.entities;
}

void tool::ComponentDataPanel::removeComponent(OnToolRemoveComponent& event)
{
	if (auto needs = event.meta.prop("need_components"_hs))
	{
		auto needCompIds = needs.value().cast<std::vector<entt::id_type>>();

		for (auto& id : needCompIds)
		{
			auto meta = entt::resolve(global::componentMetaCtx, id);
			meta.func("RemoveComponent"_hs).invoke({}, event.registry, &event.entity);
		}
	}
	else
	{
		event.meta.func("RemoveComponent"_hs).invoke({}, event.registry, &event.entity);
	}
}
