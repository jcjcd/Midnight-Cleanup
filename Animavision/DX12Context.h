#pragma once
#include "RendererContext.h"

#include "DX12Texture.h"
#include "PipelineState.h"
#include "Descriptor.h"
#include "CommandObject.h"

class DX12Context : public RendererContext
{
public:
	static constexpr uint32_t kFrameCount = 2;

	DX12Context(HWND hWnd, UINT width, UINT height);

	virtual ~DX12Context();

	virtual void Resize(uint32_t width, uint32_t height) override;

	void SetRenderTargets(uint32_t numRenderTargets, Texture* renderTargets[], Texture* depthStencil = nullptr, bool useDefaultDSV = true);

	void InitializeShaderResources(std::shared_ptr<SingleDescriptorAllocator> shaderVisibleAllocator);

	void Clear(const FLOAT* RGBA);

	void SetViewport(uint32_t width, uint32_t height);

	void BeginRender();

	void EndRender();

	void Present();

	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);

	void DrawIndexed(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation);

	void Dispatch(uint32_t x, uint32_t y, uint32_t z);

	void DispatchRays(D3D12_DISPATCH_RAYS_DESC* dispatchDesc);

	void SetRenderCommandObject(std::shared_ptr<CommandObject> commandObject) { m_RenderCommandObject = commandObject; }

	ID3D12Device5* GetDevice() { return m_Device.get(); }

	ID3D12GraphicsCommandList* GetCommandList() { return m_RenderCommandObject->GetCommandList(); };

	std::shared_ptr<CommandObjectManager> GetCommandObjectManager() { return m_CommandObjectManager; }

	UINT64 GetRenderTarget();

	virtual uint32_t GetWidth() const override { return m_ScreenWidth; }
	virtual uint32_t GetHeight() const override { return m_ScreenHeight; }

	bool IsRayTracingSupported() const { return m_RayTracingSupported; }

	uint32_t GetCurrentFrameIndex() const { return m_FrameIndex; }

	uint32_t GetBackBufferCount() const { return kFrameCount; }


private:
	void CheckRayTracingSupport();
	void LoadPipeline();
	void CreateDepthStencil();
	void LoadAssets();

private:
	// 윈도우 관련 오브젝트
	HWND m_hWnd;
	UINT m_ScreenWidth;
	UINT m_ScreenHeight;

	bool m_RayTracingSupported = false;


	// D3D12 오브젝트
	CD3DX12_VIEWPORT m_Viewport;
	CD3DX12_RECT m_ScissorRect;

	bool m_4xMsaaState = true;

	winrt::com_ptr<IDXGISwapChain4> m_SwapChain;				// 스왑 체인
	winrt::com_ptr<ID3D12Device5> m_Device;					// 디바이스
	// 커맨드 관련 오브젝트
	std::shared_ptr<CommandObjectManager> m_CommandObjectManager;

	std::shared_ptr<CommandObject> m_RenderCommandObject;

	std::shared_ptr<SingleDescriptorAllocator> m_ShaderVisibleAllocator;

	// 렌더 타겟 관련 오브젝트
	winrt::com_ptr<ID3D12DescriptorHeap> m_RTVHeap;				// 렌더 타겟 뷰 힙
	winrt::com_ptr<ID3D12Resource> m_RenderTargets[kFrameCount];			// 렌더 타겟
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_RTSRVHandle[kFrameCount];
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_RTUAVHandle[kFrameCount];
	UINT m_RTVDescriptorSize;
	uint32_t m_SwapChainFlags = 0;
	DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_CurrentRTVHandles;
	D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentDSVHandle;

	// 렌더 타겟 복사본
	winrt::com_ptr<ID3D12Resource> m_RenderTargetCopy;
	D3D12_CPU_DESCRIPTOR_HANDLE m_RTCopyHandle;

	// 뎁스 스텐실 관련 오브젝트
	winrt::com_ptr<ID3D12DescriptorHeap> m_DSVHeap;				// 뎁스 스텐실 뷰 힙
	winrt::com_ptr<ID3D12Resource> m_DepthStencil;				// 뎁스 스텐실
	UINT m_DSVDescriptorSize;

	DXGI_FORMAT m_DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 디스크립션
	D3D_FEATURE_LEVEL	m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_ADAPTER_DESC1	m_AdapterDesc = {};

	// 동기화 오브젝트
	uint32_t m_FrameIndex = 0;

	friend class ChangDXII;

};
