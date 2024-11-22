#pragma once

#include "DX11Relatives.h"

class DX11Context;
class NeoDX11Context;

class DX11GraphicsResource
{
public:
	DX11GraphicsResource() = default;
	virtual ~DX11GraphicsResource() = default;

protected:
	static ID3D11Device* GetDevice(DX11Context& context);
	static ID3D11DeviceContext* GetDeviceContext(DX11Context& context);
	static ID3D11Device* GetDevice(NeoDX11Context& context);
	static ID3D11DeviceContext* GetDeviceContext(NeoDX11Context& context);
};

