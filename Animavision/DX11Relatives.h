#pragma once

/// DX11
#include <DirectXMath.h>
#include <directxtk/SimpleMath.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <d3d11_4.h>
#include <winrt/windows.foundation.h>
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <directxtk/DDSTextureLoader.h>
#include <directxtk/WICTextureLoader.h>
#include <DirectXTex.h>
#include <DirectXTex.inl>
#include <DirectXColors.h>
#include <dxgidebug.h>
#include <directxtk/SpriteBatch.h>
#include <directxtk/Effects.h>

// Dwrite
#include <dwrite_3.h>
#include <d2d1_3.h>

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }

#pragma comment(lib, "dxguid.lib")

inline void ReportLiveDXGIObjects()
{
	IDXGIDebug* debug = nullptr;
	HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
	if (SUCCEEDED(hr))
	{
		hr = debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
		debug->Release();
	}
}

#include "DX12Helper.h"

inline bool operator==(const D3D11_SAMPLER_DESC& lhs, const D3D11_SAMPLER_DESC& rhs) {
	return
		lhs.Filter == rhs.Filter &&
		lhs.AddressU == rhs.AddressU &&
		lhs.AddressV == rhs.AddressV &&
		lhs.AddressW == rhs.AddressW &&
		lhs.MipLODBias == rhs.MipLODBias &&
		lhs.MaxAnisotropy == rhs.MaxAnisotropy &&
		lhs.ComparisonFunc == rhs.ComparisonFunc &&
		lhs.BorderColor[0] == rhs.BorderColor[0] &&
		lhs.BorderColor[1] == rhs.BorderColor[1] &&
		lhs.BorderColor[2] == rhs.BorderColor[2] &&
		lhs.BorderColor[3] == rhs.BorderColor[3] &&
		lhs.MinLOD == rhs.MinLOD &&
		lhs.MaxLOD == rhs.MaxLOD;
}

inline bool operator!=(const D3D11_SAMPLER_DESC& lhs, const D3D11_SAMPLER_DESC& rhs) {
	return !(lhs == rhs);
}

// Acquires an additional reference, if non-null.
template <typename InterfaceType>
inline InterfaceType* SafeAcquire(InterfaceType* newObject)
{
	if (newObject != NULL)
		newObject->AddRef();

	return newObject;
}

// Maps exceptions to equivalent HRESULTs,
inline HRESULT ExceptionToHResult() throw()
{
	try
	{
		throw;  // Rethrow previous exception.
	}
	catch (std::bad_alloc&)
	{
		return E_OUTOFMEMORY;
	}
	catch (...)
	{
		return E_FAIL;
	}
}

// Releases a COM object and nullifies pointer.
template <typename InterfaceType>
inline void SafeRelease(InterfaceType** currentObject)
{
	if (*currentObject != NULL)
	{
		(*currentObject)->Release();
		*currentObject = NULL;
	}
}