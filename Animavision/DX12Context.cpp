#include "pch.h"
#include "DX12Context.h"
#include "DX12Helper.h"

DX12Context::DX12Context(HWND hWnd, UINT width, UINT height) :
	m_hWnd(hWnd),
	m_ScreenWidth(width),
	m_ScreenHeight(height)
{
//#if defined(_DEBUG)
	if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
	{
		LoadLibrary(GetLatestWinPixGpuCapturerPath_Cpp17().c_str());
	}
//#endif

	m_Viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) };
	m_ScissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));

	LoadPipeline();
	LoadAssets();

}

DX12Context::~DX12Context()
{
	// 모든 작업이 끝나기까지 기다렸다가 해제를 한다.
	m_CommandObjectManager->IdleGPU();
}

void DX12Context::Resize(uint32_t width, uint32_t height)
{
	if (m_ScreenWidth == width && m_ScreenHeight == height)
		return;

	if (!(m_ScreenWidth > 0 && m_ScreenHeight > 0))
		return;

	m_ScreenWidth = width;
	m_ScreenHeight = height;

	DXGI_SWAP_CHAIN_DESC1	desc;

	m_CommandObjectManager->IdleGPU();

	ThrowIfFailed(m_SwapChain->GetDesc1(&desc));
	for (UINT n = 0; n < kFrameCount; n++)
	{
		m_RenderTargets[n] = nullptr;
	}

	ThrowIfFailed(m_SwapChain->ResizeBuffers(kFrameCount, m_ScreenWidth, m_ScreenHeight, desc.Format, desc.Flags));
	m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT n = 0; n < kFrameCount; n++)
	{
		ThrowIfFailed(m_SwapChain->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n])));
		m_Device->CreateRenderTargetView(m_RenderTargets[n].get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_RTVDescriptorSize);
	}


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;


	// Create a SRV for each frame.
	for (UINT n = 0; n < kFrameCount; n++)
	{
		m_ShaderVisibleAllocator->Free(m_RTSRVHandle[n]);
		m_RTSRVHandle[n] = {};
		auto srvHandle = m_ShaderVisibleAllocator->Allocate();
		std::wstring name = L"Render Target Texture";
		name += std::to_wstring(n);
		m_RenderTargets[n]->SetName(name.c_str());
		m_Device->CreateShaderResourceView(m_RenderTargets[n].get(), &srvDesc, srvHandle);
		m_RTSRVHandle[n] = srvHandle;
	}

	m_Viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(m_ScreenWidth), static_cast<float>(m_ScreenHeight) };
	m_ScissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(m_ScreenWidth), static_cast<LONG>(m_ScreenHeight));

	// Create depth stencil buffer
	m_DepthStencil = nullptr;
	CreateDepthStencil();


	// copy초기화
	m_RenderTargetCopy = nullptr;
	m_RTCopyHandle = {};


}

void DX12Context::SetRenderTargets(uint32_t numRenderTargets, Texture* renderTargets[], Texture* depthStencil /*= nullptr*/, bool useDefaultDSV /*= true*/)
{
	m_CurrentRTVHandles.clear();
	if (numRenderTargets != 0 && renderTargets == nullptr)
	{
		m_CurrentRTVHandles.push_back(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), m_FrameIndex, m_RTVDescriptorSize));
		//rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), m_FrameIndex, m_RTVDescriptorSize);
	}
	else
	{
		for (uint32_t i = 0; i < numRenderTargets; i++)
		{
			DX12Texture* dx12Texture = static_cast<DX12Texture*>(renderTargets[i]);
			dx12Texture->ResourceBarrier(GetCommandList(), D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_CurrentRTVHandles.push_back(static_cast<DX12Texture*>(renderTargets[i])->GetCpuDescriptorHandleRTV());
		}
	}

	if (useDefaultDSV)
	{
		m_CurrentDSVHandle = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

		GetCommandList()->OMSetRenderTargets(numRenderTargets, m_CurrentRTVHandles.data(), FALSE, &m_CurrentDSVHandle);
	}
	else
	{
		if (depthStencil)
		{
			DX12Texture* dx12Texture = static_cast<DX12Texture*>(depthStencil);
			dx12Texture->ResourceBarrier(GetCommandList(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
			m_CurrentDSVHandle = dx12Texture->GetCpuDescriptorHandleDSV();
			GetCommandList()->OMSetRenderTargets(numRenderTargets, m_CurrentRTVHandles.data(), FALSE, &m_CurrentDSVHandle);
		}
		else
		{
			m_CurrentDSVHandle = {};
			GetCommandList()->OMSetRenderTargets(numRenderTargets, m_CurrentRTVHandles.data(), FALSE, nullptr);
		}
	}
}

void DX12Context::InitializeShaderResources(std::shared_ptr<SingleDescriptorAllocator> shaderVisibleAllocator)
{
	m_ShaderVisibleAllocator = shaderVisibleAllocator;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	// Create a SRV for each frame.
	for (UINT n = 0; n < kFrameCount; n++)
	{
		auto srvHandle = m_ShaderVisibleAllocator->Allocate();
		std::wstring name = L"DX12Context::RenderTargetTexture";
		name += std::to_wstring(n);
		m_RenderTargets[n]->SetName(name.c_str());
		m_Device->CreateShaderResourceView(m_RenderTargets[n].get(), &srvDesc, srvHandle);
		m_RTSRVHandle[n] = srvHandle;
	}
}

void DX12Context::Clear(const FLOAT* RGBA)
{
	for (auto& rtvHandle : m_CurrentRTVHandles)
	{
		GetCommandList()->ClearRenderTargetView(rtvHandle, RGBA, 0, nullptr);
	}
	if (m_CurrentDSVHandle.ptr != 0)
	{
		GetCommandList()->ClearDepthStencilView(m_CurrentDSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
}

void DX12Context::SetViewport(uint32_t width, uint32_t height)
{
	m_Viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) };
	m_ScissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));

	GetCommandList()->RSSetViewports(1, &m_Viewport);
	GetCommandList()->RSSetScissorRects(1, &m_ScissorRect);
}

void DX12Context::BeginRender()
{
	m_RenderCommandObject = m_CommandObjectManager->GetCommandObjectPool()->QueryCommandObject(D3D12_COMMAND_LIST_TYPE_DIRECT);

	GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
}

void DX12Context::EndRender()
{
	// 백 버퍼가 이제 프리젠트에 사용될 것임을 나타냅니다.
	GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	m_RenderCommandObject->Finish(true);
	m_RenderCommandObject = nullptr;

}

void DX12Context::Present()
{
	//
	// Back Buffer 화면을 Primary Buffer로 전송
	//
	//UINT m_SyncInterval = 1;	// VSync On
	UINT m_SyncInterval = 0;	// VSync Off

	UINT uiSyncInterval = m_SyncInterval;
	UINT uiPresentFlags = 0;

	if (!uiSyncInterval)
	{
		uiPresentFlags = DXGI_PRESENT_ALLOW_TEARING;
	}

	HRESULT hr = m_SwapChain->Present(uiSyncInterval, uiPresentFlags);

	if (DXGI_ERROR_DEVICE_REMOVED == hr)
	{
		auto result = GetDevice()->GetDeviceRemovedReason();
		__debugbreak();
	}

	m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
}

void DX12Context::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	GetCommandList()->IASetPrimitiveTopology(primitiveTopology);
}

void DX12Context::DrawIndexed(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
	// Draw the triangle.
	GetCommandList()->DrawIndexedInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation, 0);
}

void DX12Context::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
	GetCommandList()->Dispatch(x, y, z);
}

void DX12Context::DispatchRays(D3D12_DISPATCH_RAYS_DESC* dispatchDesc)
{
	ID3D12GraphicsCommandList4* commandList4;
	GetCommandList()->QueryInterface(IID_PPV_ARGS(&commandList4));
	commandList4->DispatchRays(dispatchDesc);
}

UINT64 DX12Context::GetRenderTarget()
{
	ID3D12Resource* resource = m_RenderTargets[m_FrameIndex].get();
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_COPY_SOURCE
	);
	GetCommandList()->ResourceBarrier(1, &barrier);

	if (m_RenderTargetCopy == nullptr)
	{
		// Create srv for rtv
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = m_ScreenWidth;
		textureDesc.Height = m_ScreenHeight;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;


		m_Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_RenderTargetCopy));

		auto srvHandle = m_ShaderVisibleAllocator->Allocate();
		std::wstring name = L"Render Target Copy";
		m_RenderTargetCopy->SetName(name.c_str());
		m_Device->CreateShaderResourceView(m_RenderTargetCopy.get(), nullptr, srvHandle);
		m_RTCopyHandle = srvHandle;
	}
	else
	{
		GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargetCopy.get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	}

	// Copy the render target to the texture
	GetCommandList()->CopyResource(m_RenderTargetCopy.get(), resource);

	GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource,
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

	GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargetCopy.get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	D3D12_GPU_DESCRIPTOR_HANDLE ret = m_ShaderVisibleAllocator->GetGPUHandleFromCPUHandle(m_RTCopyHandle);

	return ret.ptr;

}

void DX12Context::CheckRayTracingSupport()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
	ThrowIfFailed(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)));

	m_RayTracingSupported = featureSupportData.RaytracingTier < D3D12_RAYTRACING_TIER_1_0 ? false : true;
}

void DX12Context::LoadPipeline()
{
	DWORD dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	winrt::com_ptr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	winrt::com_ptr<ID3D12Debug5> debugController5 = nullptr;
	if (S_OK == debugController.as(IID_PPV_ARGS(&debugController5)))
	{
		debugController5->SetEnableGPUBasedValidation(TRUE);
		debugController5->SetEnableAutoName(TRUE);
		debugController5->Release();
	}

#endif

	winrt::com_ptr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	D3D_FEATURE_LEVEL	featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};


	winrt::com_ptr<IDXGIAdapter1> pAdapter;
	DXGI_ADAPTER_DESC1 AdapterDesc = {};

	DWORD	FeatureLevelNum = _countof(featureLevels);

	for (DWORD featerLevelIndex = 0; featerLevelIndex < FeatureLevelNum; featerLevelIndex++)
	{
		UINT adapterIndex = 0;
		while (DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, pAdapter.put()))
		{
			pAdapter->GetDesc1(&AdapterDesc);

			if (SUCCEEDED(D3D12CreateDevice(pAdapter.get(), featureLevels[featerLevelIndex], IID_PPV_ARGS(&m_Device))))
			{
				goto lb_exit;

			}
			adapterIndex++;
		}
	}
lb_exit:
	if (!m_Device)
	{
		__debugbreak();
	}

#if defined(_DEBUG)
	if (debugController)
	{
		SetDebugLayerInfo(m_Device.get());
	}
#endif

	// 레이트레이싱 체크
	CheckRayTracingSupport();

	m_CommandObjectManager = std::make_shared<CommandObjectManager>(m_Device);

	ID3D12CommandQueue* commandQueue = m_CommandObjectManager->GetCommandQueue().GetCommandQueue();

	// Describe and create the swap chain.
	// 여긴 좀더 알아보도록 해야겠다.
	{
		m_SwapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = m_ScreenWidth;
		swapChainDesc.Height = m_ScreenHeight;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//swapChainDesc.BufferDesc.RefreshRate.Numerator = m_uiRefreshRate;
		//swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = kFrameCount;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags = m_SwapChainFlags;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = TRUE;

		winrt::com_ptr<IDXGISwapChain1> swapChain1 = nullptr;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(
			commandQueue,
			m_hWnd,
			&swapChainDesc,
			&fsSwapChainDesc,
			nullptr,
			swapChain1.put()
		));

		swapChain1.as(m_SwapChain);
		m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
	}

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = kFrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap)));

		m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Describe and create a depth stencil view (DSV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		ThrowIfFailed(m_Device->CreateDescriptorHeap(
			&dsvHeapDesc, IID_PPV_ARGS(m_DSVHeap.put())));

		m_DSVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		// Create frame resources.
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		// Descriptor Table
		// |        0        |        1	       |
		// | Render Target 0 | Render Target 1 |
		for (UINT n = 0; n < kFrameCount; n++)
		{
			ThrowIfFailed(m_SwapChain->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n])));
			m_Device->CreateRenderTargetView(m_RenderTargets[n].get(), &rtvDesc, rtvHandle);
			rtvHandle.Offset(1, m_RTVDescriptorSize);
		}
	}
}

void DX12Context::CreateDepthStencil()
{
	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = m_ScreenWidth;
	depthStencilDesc.Height = m_ScreenHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;

	// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
	// the depth buffer.  Therefore, because we need to create two views to the same resource:
	//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
	//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
	// we need to create the depth buffer resource with a typeless format.  
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

	// 창 : 4xmsaaState 여기
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = m_DepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;


	ThrowIfFailed(m_Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optClear,
		IID_PPV_ARGS(m_DepthStencil.put())));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = m_DepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	m_Device->CreateDepthStencilView(m_DepthStencil.get(), &dsvDesc, m_DSVHeap->GetCPUDescriptorHandleForHeapStart());

	m_DepthStencil->SetName(L"Depth Stencil Resource");

}

void DX12Context::LoadAssets()
{
	CreateDepthStencil();
}