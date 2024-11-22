#include "pch.h"
#include "Shader.h"
#include "Renderer.h"
#include "DX11Shader.h"
#include "DX12Shader.h"

std::shared_ptr<Shader> Shader::Create(Renderer* renderer, const std::string& srcPath)
{
	switch (Renderer::s_Api)
	{
	case Renderer::API::DirectX11:
		return std::make_shared<DX11Shader>(renderer->GetContext(), srcPath);
		break;
	case Renderer::API::DirectX12:
		return std::make_shared<DX12Shader>(renderer, srcPath.data());
		break;
	}

	assert(false && "Invalid API");
	return nullptr;
}

void ShaderLibrary::AddShader(const std::shared_ptr<Shader>& shader)
{
	auto check = m_Shaders.find(shader->Name);
	if (check != m_Shaders.end())
	{
		OutputDebugStringA("Shader with the same name already exists in the library\n");
		return;
	}

	m_Shaders.emplace(shader->Name, shader);

}

std::shared_ptr<Shader> ShaderLibrary::LoadShader(Renderer* renderer, const std::string& srcPath)
{
	std::string path = srcPath.data();
	std::replace(path.begin(), path.end(), '\\', '/');
	auto found = m_Shaders.find(path.data());
	if (found != m_Shaders.end())
	{
		return found->second;
	}
	else
	{
		auto shader = Shader::Create(renderer, path);
		OutputDebugStringA(path.data());
		if (shader && shader->IsValid)
		{
			OutputDebugStringA(" Loaded shader\n");
			m_Shaders.emplace(path, shader);
			return shader;
		}
		else
		{
			OutputDebugStringA(" Failed to load shader\n");
			return nullptr;
		}
	}
}


std::shared_ptr<Shader> ShaderLibrary::GetShader(const std::string& name) const
{
	auto found = m_Shaders.find(name.data());
	if (found != m_Shaders.end())
	{
		return found->second;
	}

	return nullptr;
}

void ShaderLibrary::ResetAllocateState4DX12()
{
	for (auto& shader : m_Shaders)
	{
		auto shader12 = std::static_pointer_cast<DX12Shader>(shader.second);
		shader12->ResetAllocateStates();
	}
}

