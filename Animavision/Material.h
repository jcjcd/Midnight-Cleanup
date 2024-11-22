#pragma once
#include "RendererDLL.h"

#include "ShaderResource.h"

#define DEFAULT_TEXTURE2D_PATH "./Resources/Textures/Default/defaultmagentapng.dds"
#define DEFAULT_TEXTURECUBE_PATH "./Resources/Textures/Default/winterLake.dds"
#define DEFAULT_TEXTURE2DARRAY_PATH "./Resources/Textures/Default/defaultBlackArray.dds"

class Shader;
class Renderer;

// â : ���� ���� ����� ����
class ANIMAVISION_DLL Material
{
public:
	struct TextureSlot
	{
		Texture::Type Type;
		std::string Path;
		std::shared_ptr<Texture> Texture;
	};

	static std::shared_ptr<Material> Create(const std::shared_ptr<Shader>& shader);

	// Bind�� ���� ���� ������ϳ�?
	// �̾߱⸦ �غ����ҵ�

	void SetTexture(const std::string& name, const std::shared_ptr<Texture>& texture);
	void SetUAVTexture(const std::string& name, const std::shared_ptr<Texture>& texture);

	~Material()
	{
		m_Shader = nullptr;
		m_Textures.clear();
		m_UAVTextures.clear();
	}

	void SetName(const std::string& name)
	{
		m_Name = name;
	}
	void SetShader(const std::shared_ptr<Shader>& shader);
	std::shared_ptr<Shader> GetShader() const { return m_Shader; }
	robin_hood::unordered_map<std::string, TextureSlot>& GetTextures() { return m_Textures; }

	robin_hood::unordered_map<std::string, std::shared_ptr<Texture>>& GetUAVTextures() { return m_UAVTextures; }

	std::string m_Name;

	// ��Ʈ������ shader�� ã�� ����. serialize�� ���� �̰� ��
	std::string m_ShaderString;
	std::shared_ptr<Shader> m_Shader;
	robin_hood::unordered_map<std::string, TextureSlot> m_Textures;

	robin_hood::unordered_map<std::string, std::shared_ptr<Texture>> m_UAVTextures;
};


// â : �̰� dll ������ϳ�? ������ �������ΰǰ�?
class ANIMAVISION_DLL MaterialLibrary
{
public:
	std::shared_ptr<Material> LoadFromFile(const std::string& path);

	void LoadFromDirectory(const std::string& path);

	void AddMaterial(const std::shared_ptr<Material>& material) { m_Materials[material->m_Name] = material;	}

	// save material in map to file
	void SaveMaterial(const std::string& name);

	std::map<std::string, std::shared_ptr<Material>>& GetMaterials() { return m_Materials; }

	std::shared_ptr<Material> GetMaterial(const std::string& name)
	{
		auto it = m_Materials.find(name);
		if (it == m_Materials.end())
		{
			OutputDebugStringA(name.c_str());
			OutputDebugStringA(" Material is not found\n");
			return nullptr;
		}
		else
		{
			return it->second;
		}
	}

private:
	std::map<std::string, std::shared_ptr<Material>> m_Materials;

};