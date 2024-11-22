#include "pch.h"
#include "DX11GraphicsResource.h"
#include "NeoDX11Context.h"

ID3D11Device* DX11GraphicsResource::GetDevice(NeoDX11Context& context)
{
	return context.GetDevice();
}

ID3D11DeviceContext* DX11GraphicsResource::GetDeviceContext(NeoDX11Context& context)
{
	return context.GetDeviceContext();
}
