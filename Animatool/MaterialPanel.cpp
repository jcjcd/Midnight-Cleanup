#include "pch.h"
#include "MaterialPanel.h"

#include "ToolProcess.h"
#include "ToolEvents.h"

#include <Animacore/Scene.h>
#include <Animavision/Shader.h>
#include <Animavision/Material.h>
#include <Animavision/ShaderResource.h>

tool::MaterialPanel::MaterialPanel(entt::dispatcher& dispatcher, Renderer* renderer) :
	Panel(dispatcher),
	_renderer(renderer)
{
	_dispatcher->sink<OnToolSelectFile>().connect<&MaterialPanel::changeMaterial>(this);
}

void tool::MaterialPanel::changeMaterial(const OnToolSelectFile& event)
{
	std::filesystem::path path = event.path;

	if (!std::filesystem::is_regular_file(path) || path.extension() != ".material")
	{
		return;
	}

	std::string relativePath = std::filesystem::relative(event.path).string();
	std::ranges::replace(relativePath, '\\', '/');
	_selectedFile = "./" + relativePath;
}

void tool::MaterialPanel::RenderPanel(float deltaTime)
{
	ImGui::Image(ToolProcess::icons["material"], { 32, 32 });
	ImGui::SameLine();
	ImGui::Text("%s (Material)", _selectedFile.filename().replace_extension("").string().c_str());

	ImGui::Separator();

	auto& registry = *ToolProcess::scene->GetRegistry();

	ImGui::Text("Material Properties");

	if (auto material = _renderer->GetMaterial(_selectedFile.string()))
	{
		if (ImGui::BeginCombo("Shader", material->GetShader()->Name.c_str()))
		{
			for (auto&& [key, val] : *_renderer->GetShaders())
			{
				bool isSelected = material->GetShader()->Name == key;
				if (ImGui::Selectable(key.c_str()))
				{
					if (material->GetShader()->Name == key)
					{
						continue;
					}
					material->SetShader(val);
					for (auto&& [key, texture] : material->m_Textures)
					{
						switch (texture.Type)
						{
						case Texture::Type::Texture2D:
							texture.Path = DEFAULT_TEXTURE2D_PATH;
							texture.Texture = Texture::Create(_renderer, DEFAULT_TEXTURE2D_PATH, Texture::Type::Texture2D);
							break;
						case Texture::Type::TextureCube:
							texture.Path = DEFAULT_TEXTURECUBE_PATH;
							texture.Texture = Texture::Create(_renderer, DEFAULT_TEXTURECUBE_PATH, Texture::Type::TextureCube);
							break;
						case Texture::Type::Texture2DArray:
							texture.Path = DEFAULT_TEXTURE2DARRAY_PATH;
							texture.Texture = Texture::Create(_renderer, DEFAULT_TEXTURE2DARRAY_PATH, Texture::Type::Texture2DArray);
							break;
						default:
							break;
						}
					}
					_renderer->SaveMaterial(_selectedFile.string());
				}
				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		auto& properties = material->GetTextures();

		for (const auto& prop : properties)
		{
			std::filesystem::path PropPath = prop.second.Path;
			auto shortPropPath = PropPath.filename().replace_extension().string();
			if (ImGui::BeginCombo(prop.first.c_str(), shortPropPath.c_str()))
			{
				for (const auto& entry : std::filesystem::recursive_directory_iterator("./Resources/Textures"))
				{
					std::string extension = entry.path().filename().extension().string();
					std::string entryString = entry.path().string();

					// 확장자가 대문자인 경우 소문자로 변경
					std::ranges::transform(extension, extension.begin(), ::tolower);

					std::ranges::replace(entryString, '\\', '/');
					if (std::ranges::find(_textureExtensions, entry.path().extension().string()) != _textureExtensions.end())
					{
						bool isSelected = prop.second.Path == entryString;
						auto shortPath = entry.path().filename().replace_extension().string();
						if (ImGui::Selectable(shortPath.c_str()))
						{
							auto texture = Texture::Create(_renderer, entryString, prop.second.Type);
							material->SetTexture(prop.first, texture);
							_renderer->SaveMaterial(_selectedFile.string());
						}
						if (isSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
				}
				ImGui::EndCombo();
			}

			// 드래그 앤 드롭 타겟 설정
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PAYLOAD"))
				{
					std::string filePath = static_cast<const char*>(payload->Data);
					std::filesystem::path relativePath = std::filesystem::relative(filePath);

					// 앞에 "./"를 붙이고, "\\"를 "/"로 변경
					std::string relativePathStr = "./" + relativePath.string();
					std::ranges::replace(relativePathStr, '\\', '/');

					relativePath = std::filesystem::path(relativePathStr);

					// 확장자가 대문자인 경우 소문자로 변경
					auto extension = relativePath.extension().string();
					std::ranges::transform(extension, extension.begin(), ::tolower);


					// 확장자가 이미지 파일인지 확인
					if (extension == ".png" || extension == ".dds"
						|| extension == ".jpeg" || extension == ".jpg")
					{
						// 텍스처 생성
						auto texture = Texture::Create(_renderer, relativePath.string(), prop.second.Type);
						material->SetTexture(prop.first, texture);

						// Material 파일 저장
						_renderer->SaveMaterial(_selectedFile.string());
					}
				}
				ImGui::EndDragDropTarget();
			}
		}
	}
}
