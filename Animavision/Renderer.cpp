#include "pch.h"
#include "Renderer.h"
#include "ChangDXII.h"
#include "NeoWooDXI.h"


std::unique_ptr<Renderer> Renderer::Create(HWND hwnd, uint32_t width, uint32_t height, API api, bool isRaytracing /*= false*/)
{
	s_Api = api;

	switch (api)
	{
	case API::DirectX11:
	{
		if (isRaytracing)
		{
			assert(false && "Raytracing is not supported in DirectX11");
		}
		//return std::make_unique<WooDXI>(hwnd, width, height);
		return std::make_unique<NeoWooDXI>(hwnd, width, height);
		break;
	}
	case API::DirectX12:
	{
		return std::make_unique<ChangDXII>(hwnd, width, height, isRaytracing);
		break;
	}
	}

	assert(false && "Invalid API");
	return nullptr;
}
