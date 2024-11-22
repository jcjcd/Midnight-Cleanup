#include "pch.h"
#include "ShaderResource.h"
#include "Renderer.h"
#include "DX11Texture.h"
#include "DX11Buffer.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"

std::shared_ptr<Texture> Texture::Create(Renderer* renderer, const std::string& path, const Type& type/* = Type::Texture2D*/)
{
	switch (Renderer::s_Api)
	{
	case Renderer::API::DirectX11:
		return DX11Texture::Create(renderer, path, type);
		break;
	case Renderer::API::DirectX12:
	{
		switch (type)
		{
		case Type::Texture2D:
		{
			return DX12Texture::Create(renderer, path);
			break;
		}
		case Type::TextureCube:
		{
			return DX12Texture::CreateCube(renderer, path);
			break;
		}
		default:
			break;
		}
	}
		return DX12Texture::Create(renderer, path);
		break;
	}

	assert(false && "Invalid API");
	return nullptr;
}

std::shared_ptr<Texture> Texture::Create(RendererContext* context, const std::string& path, const Type& type/* = Type::Texture2D*/)
{
	switch (Renderer::s_Api)
	{
	case Renderer::API::DirectX11:
		return std::make_shared<DX11Texture>(context, path, type);
		break;
	case Renderer::API::DirectX12:
		OutputDebugStringA("DX12Texture::Create(RendererContext*, const std::string&, const Type&) not implemented\n");
		return nullptr;
		break;
	}

	assert(false && "Invalid API");
	return nullptr;
}

std::shared_ptr<Buffer> Buffer::Create(Renderer* renderer, Mesh* mesh, Usage usage)
{
	switch (Renderer::s_Api)
	{
	case Renderer::API::DirectX11:
		return DX11Buffer::Create(renderer->GetContext(), mesh, usage);
		break;
	case Renderer::API::DirectX12:
		return DX12Buffer::Create(renderer, mesh, usage);
		break;
	}

	assert(false && "Invalid API");
	return nullptr;

}
