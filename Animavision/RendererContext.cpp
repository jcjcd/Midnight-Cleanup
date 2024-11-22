#include "pch.h"
#include "RendererContext.h"
#include "Renderer.h"

std::unique_ptr<RendererContext> RendererContext::Create(HWND hwnd, uint32_t width, uint32_t height)
{
	switch (Renderer::s_Api)
	{
	case Renderer::API::DirectX11:
	{
		//return std::make_unique<DX11Context>(hwnd, width, height);
		break;
	}
	case Renderer::API::DirectX12:
	{
		//return std::make_unique<DX12Context>(hwnd, width, height);
		break;
	}
	}

	assert(false && "Invalid API");
	return nullptr;
}

