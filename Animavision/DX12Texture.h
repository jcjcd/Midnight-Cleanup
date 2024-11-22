#pragma once
#include "ShaderResource.h"

class DX12Texture : public Texture
{
public:
	static std::shared_ptr<DX12Texture> Create(Renderer* renderer, const std::string& path);
	static std::shared_ptr<DX12Texture> CreateCube(Renderer* renderer, const std::string& path);

	DX12Texture() = default;


	ID3D12Resource* GetTexture() const { return m_Texture.get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle() const { return m_CpuDescriptorHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle() const { return m_GpuDescriptorHandle; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandleUAV() const { return m_CpuDescriptorHandleUAV; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandleUAV() const { return m_GpuDescriptorHandleUAV; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandleRTV() const { return m_CpuDescriptorHandleRTV; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandleDSV() const { return m_CpuDescriptorHandleDSV; }

	void ResourceBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES state);

	float* GetClearValue() override { return m_ClearValue; }

	void* GetShaderResourceView() override { return (void*)m_GpuDescriptorHandle.ptr; }

private:
	winrt::com_ptr<ID3D12Resource> m_Texture;
	D3D12_RESOURCE_STATES m_UsageState = D3D12_RESOURCE_STATE_COMMON;

	D3D12_CPU_DESCRIPTOR_HANDLE m_CpuDescriptorHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_GpuDescriptorHandle{};

	D3D12_CPU_DESCRIPTOR_HANDLE m_CpuDescriptorHandleUAV{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_GpuDescriptorHandleUAV{};

	D3D12_CPU_DESCRIPTOR_HANDLE m_CpuDescriptorHandleRTV{};

	D3D12_CPU_DESCRIPTOR_HANDLE m_CpuDescriptorHandleDSV{};

	float m_ClearValue[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	
	friend class DX12ResourceManager;
};

