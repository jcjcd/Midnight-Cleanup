#include "pch.h"
#include "Material.h"
#include "Renderer.h"
#include "DX12Shader.h"
#include "DX11Shader.h"

#include "ShaderResource.h"
#include <fstream>

#include <cereal/types/unordered_map.hpp>

namespace cereal
{
	template <class Archive>
	void CEREAL_SAVE_FUNCTION_NAME(Archive& ar, robin_hood::unordered_map<std::string, Material::TextureSlot> const& map)
	{
		ar(make_size_tag(static_cast<size_type>(map.size())));

		for (const auto& i : map)
			ar(make_map_item(i.first, i.second));
	}

	//! Loading for std-like pair associative containers
	template <class Archive>
	void CEREAL_LOAD_FUNCTION_NAME(Archive& ar, robin_hood::unordered_map<std::string, Material::TextureSlot>& map)
	{
		size_type size;
		ar(make_size_tag(size));

		map.clear();

		auto hint = map.begin();
		for (size_t i = 0; i < size; ++i)
		{
			std::string key;
			Material::TextureSlot value;

			ar(make_map_item(key, value));
#ifdef CEREAL_OLDER_GCC
			hint = map.insert(hint, std::make_pair(std::move(key), std::move(value)));
#else // NOT CEREAL_OLDER_GCC
			hint = map.emplace_hint(hint, std::move(key), std::move(value));
#endif // NOT CEREAL_OLDER_GCC
		}
	}

	template <class Archive>
	void serialize(Archive& archive, Material& material)
	{
		archive(
			CEREAL_NVP(material.m_Name),
			CEREAL_NVP(material.m_ShaderString),
			CEREAL_NVP(material.m_Textures)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, Material::TextureSlot& texturePair)
	{
		archive(
			CEREAL_NVP(texturePair.Type),
			CEREAL_NVP(texturePair.Path)
		);
	}

}


std::shared_ptr<Material> Material::Create(const std::shared_ptr<Shader>& shader)
{
	auto material = std::make_shared<Material>();

	material->m_ShaderString = shader->Name;
	material->m_Shader = shader;

	switch (Renderer::s_Api)
	{
	case Renderer::API::NONE:
		return material;
	case Renderer::API::DirectX11:
	{
		auto dx11Shader = std::static_pointer_cast<DX11Shader>(shader);
		for (auto& texture : dx11Shader->m_TextureBindings)
		{
			material->m_Textures.emplace(texture.Name, TextureSlot{ (Texture::Type)texture.Dimension,"", nullptr });
		}
		for (auto& texture : dx11Shader->m_UAVBindings)
		{
			material->m_UAVTextures.emplace(texture.Name, nullptr);
		}
		return material;
	}
	return material;
	case Renderer::API::DirectX12:
	{
		auto dx12Shader = std::static_pointer_cast<DX12Shader>(shader);
		for (auto& texture : dx12Shader->m_TextureBindings[DX12Shader::ROOT_GLOBAL])
		{
			material->m_Textures.emplace(texture.Name, TextureSlot{ texture.TextureDimension, "", nullptr });
		}
		for (auto& texture : dx12Shader->m_UAVBindings[DX12Shader::ROOT_GLOBAL])
		{
			material->m_UAVTextures.emplace(texture.Name, nullptr);
		}
		return material;
	}
	default:
		return material;
	}
}

void Material::SetTexture(const std::string& name, const std::shared_ptr<Texture>& texture)
{
	auto it = m_Textures.find(name);
	if (it == m_Textures.end())
	{
		OutputDebugStringA(this->m_Shader->Name.c_str());
		OutputDebugStringA(m_Name.c_str());
		OutputDebugStringA("Material's ");
		OutputDebugStringA(name.c_str());
		OutputDebugStringA(" : Setting Texture is not found\n");
		return;
	}
	else
	{
		if (it->second.Type != texture->m_Type)
		{
			OutputDebugStringA("Texture type is not matched\n");
			return;
		}

		it->second.Path = texture->m_Path;
		it->second.Texture = texture;
		return;
	}
}

void Material::SetUAVTexture(const std::string& name, const std::shared_ptr<Texture>& texture)
{
	auto it = m_UAVTextures.find(name);
	if (it == m_UAVTextures.end())
	{
		OutputDebugStringA(name.c_str());
		OutputDebugStringA(" : Texture is not found\n");
		return;
	}
	else
	{
		it->second = texture;
		return;
	}
}

void Material::SetShader(const std::shared_ptr<Shader>& shader)
{
	this->m_Shader = shader;
	this->m_ShaderString = shader->Name;

	this->m_Textures.clear();

	switch (Renderer::s_Api)
	{
	case Renderer::API::DirectX11:
	{
		auto dx11Shader = std::static_pointer_cast<DX11Shader>(shader);
		for (auto& texture : dx11Shader->m_TextureBindings)
		{
			m_Textures.emplace(texture.Name, TextureSlot{ (Texture::Type)texture.Dimension, "", nullptr });
		}
		for (auto& texture : dx11Shader->m_UAVBindings)
		{
			m_UAVTextures.emplace(texture.Name, nullptr);
		}
	}
	break;
	case Renderer::API::DirectX12:
	{
		auto dx12Shader = std::static_pointer_cast<DX12Shader>(shader);
		for (auto& texture : dx12Shader->m_TextureBindings[DX12Shader::ROOT_GLOBAL])
		{
			m_Textures.emplace(texture.Name, TextureSlot{ texture.TextureDimension, "", nullptr });
		}
		for (auto& texture : dx12Shader->m_UAVBindings[DX12Shader::ROOT_GLOBAL])
		{
			m_UAVTextures.emplace(texture.Name, nullptr);
		}
	}
	case Renderer::API::NONE:
	default:
		break;

	}
}

std::shared_ptr<Material> MaterialLibrary::LoadFromFile(const std::string& path)
{
	std::string pathString = path;
	std::replace(pathString.begin(), pathString.end(), '\\', '/');

	std::filesystem::path filePath = std::filesystem::current_path();
	filePath /= pathString;

	std::ifstream file(filePath);
	if (!file.is_open())
	{
		OutputDebugStringA("File is not found\n");
		return nullptr;
	}
	cereal::JSONInputArchive archive(file);

	std::shared_ptr<Material> material = std::make_shared<Material>();
	archive(*material);

	if (material->m_Name != pathString)
	{
		material->m_Name = pathString;
	}
	m_Materials[pathString] = material;

	return material;
}

void MaterialLibrary::LoadFromDirectory(const std::string& path)
{
	std::filesystem::path directory = path;
	if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
	{
		OutputDebugStringA("Directory is not found\n");
		return;
	}

	// recursive iterator로 만들어보자
	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".material")
		{
			std::filesystem::path curPath = entry.path();

			auto absPath = relative(entry);
			std::string pathString = curPath.string();
			//std::replace(pathString.begin(), pathString.end(), '\\', '/');
			LoadFromFile(pathString);
		}
	}
}

void MaterialLibrary::SaveMaterial(const std::string& name)
{
	std::ofstream file(name);
	cereal::JSONOutputArchive archive(file);

	auto material = m_Materials.find(name);
	if (material != m_Materials.end())
	{
		archive(*material->second);
	}
	else
	{
		OutputDebugStringA("Material is not found\n");
	}
}
